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
		
		void handleRequest(int clientSocket);
		
		std::string details(); // Called via the user thread
		
	private:
		Server* server;
		int threadIndex -1;
		
		void awaitRequest();
	
		static void* startFileSenderThread(void* fileSender); // Called from this classes ctor when creating its own pthread.
};

#endif