#ifndef FileDownloader_H
#define FileDownloader_H

#include <pthread.h>
#include <string>

#define CMD_LIST 		0
#define CMD_DOWNLOAD 	1

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
		
		void fetchFileList(std::string ip, int port);
		void handleDownload(std::string ip, int port, std::string filename);
		
		std::string details(); // Called via the user thread
		
	private:
		DownloadManager* downloadManager;
		int threadIndex -1;
		
		void awaitRequest();
		
		static void* startFileDownloaderThread(void* fileDownloader); // Called from this classes ctor when creating its own pthread.
};

#endif