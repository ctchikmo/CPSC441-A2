#ifndef FileSender_H
#define FileSender_H

#include <pthread.h>
#include <string>
#include <queue>
#include <sys/socket.h>

class Server; // Forward Declaration

// The FileSender class handles a connections request for a download. If something goes wrong the message is sent over the socket to a FileDownloader, which handles getting it to the user. 
class FileSender
{
	public:
		FileSender(Server* server, int threadIndex);
		~FileSender();
		
		int getThreadIndex();
		
		void beginRequest(int clientSocket, int port, std::string request);
		void handleData(char* data, int recBytes);
		int getClientSocket();
		int getClientPort(); // Port is used by server to direct recv traffic.
		
		bool flag_continue = false;
		pthread_mutex_t* getMutex();
		pthread_cond_t* getCond();
		
		void quit(); // Called from the user thread
		
		Server* server;
		
	private:
		pthread_t thread;
		pthread_mutex_t requestMutex;
		pthread_cond_t requestCond;
	
		int threadIndex = -1;
		
		bool flag_running = true;
		int clientSocket = -1;
		int clientPort = -1;
		std::string request;
		
		void awaitRequest();
		
		// The handlers are acutally just dataQueue consumers, which quit after a too many timeout retransmissions, or a final ack is sent. 
		// RequestMutex is used on these as that mutex is safe to use at this time. It is only enabled while waiting for a connection. 
		void handleFileList(); 
		void handleFile();
		bool generalHandler(int size, char* toSend);
		std::queue<std::string> dataQueue;
		
		bool fileSizeTimeout();
	
		static void* startFileSenderThread(void* fileSender); // Called from this classes ctor when creating its own pthread.
};

#endif