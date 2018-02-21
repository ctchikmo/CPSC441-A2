#ifndef FileSender_H
#define FileSender_H

#include <pthread.h>
#include <string>

class Server; // Forward Declaration

// The FileSender class handles a connections request for a download. If something goes wrong the message is sent over the socket to a FileDownloader, which handles getting it to the user. 
class FileSender
{
	public:
		FileSender(Server* server, int threadIndex);
		~FileSender();
		
		int getThreadIndex();
		
		void beginRequest(int clientSocket);
		
		void quit(); // Called from the user thread
		
	private:
		pthread_t thread;
		pthread_mutex_t requestMutex;
		pthread_cond_t requestCond;
	
		Server* server;
		int threadIndex = -1;
		
		bool flag_running = true;
		int clientSocket = -1;
		
		void awaitRequest();
		void handleRequest();
	
		static void* startFileSenderThread(void* fileSender); // Called from this classes ctor when creating its own pthread.
};

#endif