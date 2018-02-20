#ifndef DownloadManager_H
#define DownloadManager_H

#include <string>

class User; // Forward declaration

// The DownloadManager class handles commands from the User to download a file. If a FileDownloader is available DownloadManager lets one handle the download, otherwise it queues the request. 
class DownloadManager
{
	public:
		DownloadManager(int threads); // Thread pooling and new thread for the manager is done here. 
		~DownloadManager();
		
		void setUser(*User user);
		void fileList(std::string ip, int port) // Gets the file list from the ip/port. Delivers message to user
		void download(std::string ip, int port, std::string filename); // Attempts to download the given file from the desired location. Stores file in directory and notifies user once done. 
		
	private:
		User* user;
};

#endif