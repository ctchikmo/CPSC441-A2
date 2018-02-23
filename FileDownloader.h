#ifndef FileDownloader_H
#define FileDownloader_H

#include "Request.h"

#include <pthread.h>
#include <string>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

/*
	std::ofstream outfile(directory.append("/test.txt"));
	outfile << "testing" << std::endl;
	outfile.close();
	*/

class DownloadManager;

// The FileDownloader class handles the user commanded download. If something goes wrong the FileDownloader notifies the DownloadManager which will handle the user. 
class FileDownloader
{
	public:
		FileDownloader(DownloadManager* downloadManager, int threadIndex);
		~FileDownloader();
		
		void beginRequest(Request requst);
		int getServSocket();
		struct sockaddr_in* getServAddress();
		
		int getThreadIndex();
		void details(); // Called via the user thread
		
		void quit(); // Called from the user thread
		
	private:
		pthread_t thread;
		pthread_mutex_t requestMutex;
		pthread_cond_t requestCond;
	
		DownloadManager* downloadManager;
		int threadIndex = -1;
		
		bool flag_running = true;
		Request request; // A port value of >= 0 means good, -1 means startup, -2 means finished request.
		int servSocket = -1;
		struct sockaddr_in address;
		
		void awaitRequest();
		void fetchFileList(int recvPort);
		void handleDownload(int recvPort);
		void generalHandler(int recvPort, char* opener, char** data, int* dataSize);
		
		static void* startFileDownloaderThread(void* fileDownloader); // Called from this classes ctor when creating its own pthread.
};

#endif