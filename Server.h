#ifndef Server_H
#define Server_H

#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <string>
#include <queue>
#include <vector>

#define SERVER_MAX_BACKLOG 128

class User;
class FileSender; //Forward declaration

// The Server class handles incomming connections and after a successful connection hands them off to a FileSender
class Server
{
	public:
		Server(int port, int threads);
		~Server();
		bool isReady();
		void setUser(User* user);
		
		void reclaim(FileSender* fileSender, int threadIndex);
		
		void details(); // Called on user thread, returns server details: Host port, Host ip, Hosted files, Thread count, threads in use & the file they are sending. 
		std::vector<std::string> listFilenames();
		
		void quit(); // Called from the user thread
		
		pthread_mutex_t* getReadyMutex();
		pthread_cond_t* getReadyCond();
		
		User* user; // User is used for accessing the current directory (where the files are hosted)
		
	private:
		bool flag_ready = false;
		int port = -1;
		std::string ip;
		pthread_t thread;
		pthread_mutex_t readyMutex; // The ready mutex and cond are used to indicate that the server has completed startup (at least one FileSender up, and Server is about to open for connections.)
		pthread_cond_t readyCond;
	
		pthread_mutex_t senderMutex;
		pthread_cond_t senderCond;
		
		int serverSocket = -1;
		struct sockaddr_in serverAddressing;
		
		int numThreads = -1;
		FileSender** inProgress; // Array which is the same size as the number of threads. When a thread is created it is given a unique index corresponding to the position it is stored in. Required for quit(). 
		std::queue<FileSender*> senders;
		
		bool flag_running = true;
		
		bool isDir(const char* path);
		
		void startupServer();
		void reciveData();
		static void* startServerThread(void* server); // Called from this classes ctor when creating its own pthread. This is only called once for Server
};

#endif