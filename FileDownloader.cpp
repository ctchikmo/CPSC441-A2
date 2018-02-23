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
#include <unistd.h>

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

int FileDownloader::getServSocket()
{
	return servSocket;
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
		servSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		
		struct sockaddr_in address;
		address.sin_family = AF_INET; // IPV4 byte ordering
		if(inet_pton(AF_INET, request.ip.c_str(), &address.sin_addr) == 1) // Convert the ip to byte form. (1 means good)
		{
			address.sin_port = htons(request.port);
		
			if(connect(servSocket, (struct sockaddr*)&address, sizeof(address)) < 0) // UDP connect just sets default sendto 
				request.port = -3;
			
			else // we good
			{
				if(request.type == RequestType::DOWNLOAD)
					handleDownload();
				else
					fetchFileList();	
			}
		}
		else
			request.port = -3;
		
		close(servSocket);
	}
}

void FileDownloader::fetchFileList()
{
	// Connect to the server and let it know we are asking for the file list. 
	char opener[OPENER_SIZE]; // Opener just sends the requst type and the file desired. Here it is the list command, so no file.
	opener[OPENER_POS] = FILE_LIST;
	if(send(servSocket, opener, OPENER_SIZE, MSG_NOSIGNAL) == -1)
	{
		request.port = -4;
		return; // The server will clean itself up after timeout. 
	}
	
	// Get the file size so we can run the Octoblock algorithm here, then start waiting for recvs. 
	char fileSize[8];
	int recBytes = recv(servSocket, fileSize, sizeof(fileSize), 0);
	if(recBytes < 0)
	{
		request.port = -4;
		return; // The server will clean itself up after timeout. 
	}
	std::string fSize(fileSize, recBytes);
	int fileS = std::stoi(fSize);
	
	std::vector<Octoblock*> blocks;
	Octoblock::getOctoblocks(&blocks, fileS, this);
	int current = 0;
	
	if(blocks.size() == 0)
	{
		std::cout << "No Files Hosted" << std::endl;
		return;
	}
	
	while(flag_running)
	{
		if(blocks[current]->complete())
		{
			current++;
			if(current > (int)blocks.size())
				break;
		}
		else
		{
			char buffer[OCTOLEG_MAX_SIZE];
			int bytes = recv(servSocket, buffer, sizeof(buffer), 0);
			if(recBytes < 0)
			{
				request.port = -4;
				return; // The server will clean itself up after timeout. 
			}
			else
			{
				if(!(blocks[current]->recvClient(buffer, bytes)))
				{
					request.port = -4;
					return; // The server will clean itself up after timeout. 
				}	
			}
		}
	}
	
	// We got all the data.
	// Note, for filenames I appended the ends with \n, so it will be printable.
	char* data = new char[fileS + 1]; // +1 to add the \0
	int pos = 0;
	for(int i = 0; i < (int)blocks.size(); i++)
	{
		char* blockData = NULL;
		int dataSize;
		blocks[i]->getData(blockData, &dataSize);
		
		for(int j = 0; j < dataSize; j++)
		{
			data[pos + j] = blockData[j];
		}
		
		delete[] blockData;
		pos += dataSize;
	}	
	
	data[fileS] = '\0';
	std::cout << data << std::endl;
}

void FileDownloader::handleDownload()
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





