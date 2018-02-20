#ifndef DownloadManager_H
#define DownloadManager_H

#include <pthread.h>
#include <string>
#include <queue>

#define REQUEST_STARTED "Request Started\r\n"
#define REQUEST_QUEUED "Request Queued\r\n"

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
		
		std::string fileList(std::string ip, int port) // Gets the file list from the ip/port. Delivers message to user on completion
		std::string download(std::string ip, int port, std::string filename); // Attempts to download the given file from the desired location. Stores file in directory and notifies user once done. 
		void reclaim(FileDownloader* fileDownloader); // Adds the downloader back to the queue, if the request queue is not empty it immediately takes the next request.
		
		void setUser(*User user);
		*User getUser();
		
		std::string details(); // Called on the User thread, returns total number of download threads, and how many in use + the files they are getting. 
		
	private:
		pthread_mutex_t requestMutex;
		
		std::queue<Request> requests; // A request is only queued if it cannot be handled immediately.
		std::queue<FileDownloader*> downloaders;
	
		User* user;
};

#endif