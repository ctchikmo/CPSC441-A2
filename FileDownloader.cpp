#include "FileDownloader.h"
#include "DownloadManager.h"
#include "Request.h"

#include <iostream>
#include <stdlib.h>

FileDownloader::FileDownloader(DownloadManager* downloadManager, int threadIndex):
downloadManager(downloadManager),
threadIndex(threadIndex),
request(RequestType::DOWNLOAD, "ip", -1, "file")
{
	pthread_mutex_init(&requestMutex, NULL);
	pthread_cond_init(&requestCond, NULL);
	
	int rv = pthread_create(&thread, NULL, startFileDownloaderThread, this);
	if (rv != 0)
	{
		std::cout << "Error creating FileDownloader thread, exiting program" << std::endl;
		exit(-1);
	}
}

FileDownloader::~FileDownloader()
{
	pthread_mutex_destroy(&requestMutex);
	pthread_cond_destroy(&requestCond);
}

void FileDownloader::beginRequest(Request req)
{
	request = req;
	
	pthread_mutex_lock(&requestMutex);
	{
		pthread_cond_signal(&requestCond);
	}
	pthread_mutex_unlock(&requestMutex);
}

void FileDownloader::fetchFileList()
{
	
}

void FileDownloader::handleDownload()
{
	
}

int FileDownloader::getThreadIndex()
{
	return threadIndex;
}

void FileDownloader::details()
{
	std::cout << "Currently Downloading: " << request.filename << ", from: " << request.ip << ", on port: " << request.port << std::endl;
}

void FileDownloader::awaitRequest()
{
	while(flag_running)
	{
		pthread_mutex_lock(&requestMutex);
		{
			if(request.port != -1)
				downloadManager->reclaim(this, threadIndex, "Done download for " + request.filename + ", from: " + request.ip + ", on port: " + std::to_string(request.port));
			
			while(request.port == -1 || request.port == -2)
				pthread_cond_wait(&requestCond, &requestMutex);
		}
		pthread_mutex_unlock(&requestMutex);
		
		// At this point we have a new request to handle
		if(request.type == RequestType::DOWNLOAD)
			handleDownload();
		else
			fetchFileList();
		
		request.port = -2; // We are done with this request. 
	}
}

void* FileDownloader::startFileDownloaderThread(void* fileDownloader)
{
	FileDownloader* fd = (FileDownloader*)fileDownloader;
	fd->awaitRequest();
	
	delete fd;
	return NULL;
}

void FileDownloader::quit()
{
	flag_running = false;
}





