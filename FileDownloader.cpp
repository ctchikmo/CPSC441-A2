#include "FileDownloader.h"
#include "DownloadManager.h"
#include "Request.h"
#include "Communication.h"

#include <iostream>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

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
		// I use request.port to flag stuff here, which is probably bad practice, but w/e. 
		pthread_mutex_lock(&requestMutex);
		{
			if(request.port == -1)
				downloadManager->reclaim(this, threadIndex, "Downloader Thread Started");
			else if(request.port == -3)
			{
				downloadManager->reclaim(this, threadIndex, "File download for " + request.filename + " encounted an error during socket setup.");
				request.port = -2; // We are done with this request. 
			}
			else if(request.port == -4)
			{
				downloadManager->reclaim(this, threadIndex, "File download for " + request.filename + "  encounted an error during download");
				request.port = -2; // We are done with this request. 
			}
			else
			{
				downloadManager->reclaim(this, threadIndex, "Done download for " + request.filename + ", from: " + request.ip + ", on port: " + std::to_string(request.port));
				request.port = -2; // We are done with this request. 
			}
		
			while(request.port == -1 || request.port == -2)
				pthread_cond_wait(&requestCond, &requestMutex);
		}
		pthread_mutex_unlock(&requestMutex);
		
		// At this point we have a new request to handle
		
		// Get the socket to use and set it up. 
		int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		
		struct sockaddr_in address;
		address.sin_family = AF_INET; // IPV4 byte ordering
		if(inet_pton(AF_INET, request.ip.c_str(), &address.sin_addr) == 1) // Convert the ip to byte form. (1 means good)
		{
			address.sin_port = htons(request.port);
		
			if(connect(sock, (struct sockaddr*)&address, sizeof(address)) < 0)
				request.port = -3;
			
			else // we good
			{
				if(request.type == RequestType::DOWNLOAD)
					handleDownload(sock);
				else
					fetchFileList(sock);	
			}
		}
		else
			request.port = -3;
	}
}

void FileDownloader::fetchFileList(int socket)
{
	// Connect to the server and let it know we are asking for the file list. 
	char opener[OPENER_SIZE]; // Opener just sends the requst type and the file desired. Here it is the list command, so no file.
	opener[OPENER_POS] = FILE_LIST;
	if(send(socket, opener, OPENER_SIZE, MSG_NOSIGNAL) == -1)
	{
		request.port = -4;
		return;
	}
}

void FileDownloader::handleDownload(int socket)
{
	
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





