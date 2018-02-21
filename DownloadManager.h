#ifndef DownloadManager_H
#define DownloadManager_H

#include "Request.h"

#include <pthread.h>
#include <string>
#include <queue>

class User; // Forward declaration
class FileDownloader;

// The DownloadManager class handles commands from the User to download a file. If a FileDownloader is available DownloadManager lets one handle the download, otherwise it queues the request. 
class DownloadManager
{
	public:
		DownloadManager(int threads); // Thread pooling and new thread for the manager is done here. 
		~DownloadManager();
		
		void reclaim(FileDownloader* fileDownloader, int threadIndex, std::string userMessage); // Adds the downloader back to the queue, if the request queue is not empty it immediately takes the next request.
		std::string fileList(std::string ip, int port); // Gets the file list from the ip/port. Delivers message to user on completion. RETURNS IF QUEUED OR NOT
		std::string download(std::string ip, int port, std::string filename); // Attempts to download the given file from the desired location. Stores file in directory and notifies user once done. RETURNS IF QUEUED OR NOT
		
		void details(); // Called on the User thread, returns total number of download threads, and how many in use + the files they are getting. 
		
		void setUser(User* user);
		void quit(); // Called from the user thread
		
	private:	
		bool flag_running = true;
		pthread_t thread;
		pthread_mutex_t requestMutex;
		pthread_cond_t requestCond; // This is used only for startup checking. 
		
		FileDownloader** inProgress; // Array which is the same size as the number of threads. When a thread is created it is given a unique index corresponding to the position it is stored in. 
		int numThreads = -1;
		std::queue<Request> requests; // A request is only queued if it cannot be handled immediately.
		std::queue<FileDownloader*> downloaders;
		
		void requestConsumer();
		std::string requestAdder(RequestType type, std::string ip, int port, std::string filename);
		
		static void* startDownloadManagerThread(void* downloadManager);
	
		User* user;
};

#endif