#ifndef DownloadManager_H
#define DownloadManager_H

#include <pthread.h>
#include <string>
#include <queue>

#define REQUEST_STARTED 	"Request Started\r\n"
#define REQUEST_QUEUED 		"Request Queued\r\n"

class User; // Forward declaration
class FileDownloader;

typedef struct Request
{
	std::string ip;
	int port;
	std::string filename;
} Request;

// The DownloadManager class handles commands from the User to download a file. If a FileDownloader is available DownloadManager lets one handle the download, otherwise it queues the request. 
class DownloadManager
{
	public:
		DownloadManager(int threads); // Thread pooling and new thread for the manager is done here. 
		~DownloadManager();
		bool isReady();
		
		std::string fileList(std::string ip, int port); // Gets the file list from the ip/port. Delivers message to user on completion
		std::string download(std::string ip, int port, std::string filename); // Attempts to download the given file from the desired location. Stores file in directory and notifies user once done. 
		void reclaim(FileDownloader* fileDownloader, int threadIndex, std::string userMessage); // Adds the downloader back to the queue, if the request queue is not empty it immediately takes the next request.
		
		std::string details(); // Called on the User thread, returns total number of download threads, and how many in use + the files they are getting. 
		
		void setUser(User* user);
		
		pthread_mutex_t* getReadyMutex();
		pthread_cond_t* getReadyCond();
		
	private:
		bool flag_ready = false;
		pthread_mutex_t readyMutex;
		pthread_cond_t readyCond;
	
		pthread_mutex_t requestMutex;
		
		FileDownloader* inProgress; // Array which is the same size as the number of threads. When a thread is created it is given a unique index corresponding to the position it is stored in. 
		std::queue<Request> requests; // A request is only queued if it cannot be handled immediately.
		std::queue<FileDownloader*> downloaders;
	
		User* user;
		
		static void* startDownloadManagerThread(void* downloadManager); // Called from this classes ctor when creating its own pthread. This is only called once for downloadManager
};

#endif