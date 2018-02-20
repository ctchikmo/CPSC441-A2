#ifndef Server_H
#define Server_H

#include <pthread.h>
#include <string>
#include <queue>

class User; //Forward declaration
class FileSender; //Forward declaration

// The Server class handles incomming connections and after a successful connection hands them off to a FileSender
class Server
{
	public:
		Server(int threads);
		~Server();
		bool isReady();
		
		void reclaim(FileSender* fileSender, int threadIndex);
		
		std::string details(); // Called on user thread, returns server details: Host port, Host ip, Hosted files, Thread count, threads in use & the file they are sending. 
		
		void setUser(User* user);
		
		pthread_mutex_t* getReadyMutex();
		pthread_cond_t* getReadyCond();
		
	private:
		bool flag_ready = false;
		pthread_mutex_t readyMutex;
		pthread_cond_t readyCond;
	
		pthread_mutex_t senderMutex;
		pthread_cond_t senderCond;
		
		int port = -1;
		FileSender* inProgress; // Array which is the same size as the number of threads. When a thread is created it is given a unique index corresponding to the position it is stored in.
		std::queue<FileSender*> senders;
	
		User* user;
		
		static void* startServerThread(void* server); // Called from this classes ctor when creating its own pthread. This is only called once for Server
};

#endif