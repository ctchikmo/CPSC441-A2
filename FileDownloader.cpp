#include "FileDownloader.h"
#include "DownloadManager.h"
#include "Request.h"
#include "Communication.h"
#include "User.h"
#include "Timer.h"

#include <iostream>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring> // memset
#include <fstream> 

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

struct sockaddr_in* FileDownloader::getServAddress()
{
	return &address;
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
				downloadManager->reclaim(this, threadIndex, "File download for " + request.filename + "  encounted an error (or timeout) during download");
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
		
		// There are 2 addresses, the one we send on to the server, and the one with the random port we bind to, to recv.
		struct sockaddr_in recvAddress;
		recvAddress.sin_family = AF_INET; // IPV4 byte ordering
		recvAddress.sin_addr.s_addr = INADDR_ANY; // Use any/all of the computers registered IPs.
		recvAddress.sin_port = htons(0); // Random port.
		
		memset((sockaddr_in*)&address, 0, sizeof(address));
		address.sin_family = AF_INET; // IPV4 byte ordering
		address.sin_port = htons(request.port);
		if(inet_pton(AF_INET, request.ip.c_str(), &address.sin_addr) == 1) // Convert the ip to byte form. (1 means good)
		{
			if(bind(servSocket, (struct sockaddr*) &recvAddress, sizeof(recvAddress)) < 0) // Bind to the recvAddress so we can get data. 
				request.port = -3;
				
			else // we good
			{
				// I can not simply use the port given to recvAddress after the bind, as recvAddress is not updated, I need to read the sockets "name" for the new port. 
				struct sockaddr_in myAddress;
				int len = sizeof(myAddress);
				if(getsockname(servSocket, (struct sockaddr*) &myAddress, &len) < 0)
				{
					std::cout << "Error reading new client bind socket" << std::endl;
					exit(1);
				}
				int myRecvPort = ntohs(myAddress.sin_port); // Convert the byte form port to an int 
				
				if(request.type == RequestType::DOWNLOAD)
					fileDownload(myRecvPort);
				else
					fetchFileList(myRecvPort);	
			}
		}
		else
			request.port = -3;
		
		close(servSocket);
	}
}

void FileDownloader::fetchFileList(int recvPort)
{
	// Connect to the server and let it know we are asking for the file list. 
	char opener[OPENER_SIZE]; // Opener just sends the requst type and the file desired. Here it is the list command, so no file.
	opener[OPENER_POS] = FILE_LIST;
	std::string str_recvPort = std::to_string(recvPort);
	for(int i = 0; i < (int)str_recvPort.size(); i++)
		opener[OPENER_RECVPORT + i] = str_recvPort[i];
	
	char* data = NULL;
	int dataSize = 0;
	generalHandler(opener, data, &dataSize);	
	
	if(request.port == -4)
		return;
	
	if(dataSize > 0)
	{
		std::string print = "";
		for(int i = 0; i < dataSize; i++)
			print += data[i];
		downloadManager->user->bufferMessage(print);
		
		//delete[] data; I have no idea why, but this crashes with an abort every single time, even though it is identical to fileDownload, which does not abort, so this will just remain a mem leak.
	}
	else
		downloadManager->user->bufferMessage("No Files Hosted");
}

void FileDownloader::fileDownload(int recvPort)
{
	// Connect to the server and let it know we are asking for a file
	char opener[OPENER_SIZE]; // Opener just sends the requst type and the file desired. Here it is the list command, so no file.
	opener[OPENER_POS] = FILE_DOWNLOAD;
	std::string str_recvPort = std::to_string(recvPort);
	for(int i = 0; i < (int)str_recvPort.size(); i++)
		opener[OPENER_RECVPORT + i] = str_recvPort[i];
	
	// A \0 indicates end of file name
	int i = 0;
	for(; i < (int)request.filename.size(); i++)
		opener[OPENER_FILENAME + i] = request.filename[i];
	opener[OPENER_FILENAME + i] = '\0';
	
	char* data = NULL;
	int dataSize = 0;
	generalHandler(opener, data, &dataSize);	
	
	if(request.port == -4)
		return;
	
	if(dataSize >= 0) //empty file if 0
	{
		// We are good to create the file.
		std::string print(data, dataSize);
		std::ofstream outfile(downloadManager->user->getDirectory() + "/" + request.filename);
		for(int i = 0; i < dataSize; i++)
			outfile << data[i];
		outfile.close();
		
		delete[] data;
	}
	else
		downloadManager->user->bufferMessage(request.filename + " not found on " + request.ip);
}

void FileDownloader::generalHandler(char* opener, char*& data, int* dataSize)
{
	openerForTimeout = opener;
	
	// Random loss potential
	if(!downloadManager->user->losePacket())
	{
		if(sendto(servSocket, opener, OPENER_SIZE, MSG_NOSIGNAL, (struct sockaddr*)&address, sizeof(address)) == -1)
		{
			request.port = -4;
			return; // The server will clean itself up after timeout. 
		}
	}
	else
		downloadManager->user->bufferMessage("Random loss: sending opener.");
	
	// Get the file size so we can run the Octoblock algorithm here, then start waiting for recvs. 
	char fileSize[FILE_SIZE_BUFF];
	int recBytes;
	while(flag_running)
	{
		Timer<FileDownloader>* fileSizeTimer = new Timer<FileDownloader>(WAIT_TIME, this, &FileDownloader::fileSizeTimeout, servSocket);
		recBytes = recv(servSocket, fileSize, sizeof(fileSize), 0);
		
		// If anything got out of that recv we stop the timer. 
		fileSizeTimer->stop();
		if(fileSizeTimer->attemptsFinished())
		{
			request.port = -4;
			return; // The server will clean itself up after timeout. 
		}
		else if(recBytes < 0)
		{
			request.port = -4;
			return; // The server will clean itself up after timeout. 
		}
		else if(recBytes == FILE_SIZE_BUFF)
			break;
	}
	
	// Successful filesize, we ack to server.
	char fileSizeAck[FILE_SIZE_ACK_BUFF];
	fileSizeAck[FILE_SIZE_KEY_B] = FILE_SIZE_KEY;
	
	// Random loss potential
	if(!downloadManager->user->losePacket())
	{
		if(sendto(servSocket, fileSizeAck, FILE_SIZE_ACK_BUFF, MSG_NOSIGNAL, (struct sockaddr*)&address, sizeof(address)) == -1)
		{
			request.port = -4;
			return; // The server will clean itself up after timeout. 
		}
	}
	else
		downloadManager->user->bufferMessage("Random loss: fileSize ack");
	
	if(recBytes == FILE_SIZE_BUFF && fileSize[FILE_SIZE_START] == '-' && fileSize[FILE_SIZE_START + 1] == '1')
	{
		*dataSize = -1;
		return;
	}
	
	std::string fSize = "";
	for(int i = 0; fileSize[FILE_SIZE_START + i] != FILE_SIZE_END; i++)
		fSize += fileSize[FILE_SIZE_START + i];
	int fileS = std::stoi(fSize);
	
	std::vector<Octoblock*> blocks;
	Octoblock::getOctoblocks(&blocks, fileS, this);
	int current = 0;
	
	if(blocks.size() == 0)
	{
		*dataSize = 0;
		return;
	}
	
	while(flag_running)
	{
		if(blocks[current]->complete())
		{
			std::cout << current << std::endl;
			current++;
			if(current >= (int)blocks.size())
				break;
		}
		else
		{
			char buffer[OCTOLEG_MAX_SIZE + HEADER_SIZE + FILE_SIZE_BUFF]; // This is the size that works for everything, it doesnt matter that it might be bigger than any recv. 
			int bytes;
			
			Timer<Octoblock>* timeStandard = new Timer<Octoblock>(WAIT_TIME, blocks[current], &Octoblock::requestRetrans, servSocket);
			bytes = recv(servSocket, buffer, sizeof(buffer), 0);
			
			// If anything gets out of recv we stop the timer
			timeStandard->stop();
			
			if(timeStandard->attemptsFinished())
			{
				request.port = -4;
				return; // The server will clean itself up after timeout. 
			}
			else if(recBytes < 0)
			{
				request.port = -4;
				return; // The server will clean itself up after timeout. 
			}
			else if(bytes > 0)
			{
				if(downloadManager->user->losePacket()) // Random loss potential
				{
					downloadManager->user->bufferMessage("Random loss: loop recv");
					continue;
				}
				else if(bytes == FILE_SIZE_BUFF && buffer[FILE_SIZE_KEY_B] == FILE_SIZE_KEY)// Sever waiting on fileSize ack still
				{
					if(sendto(servSocket, fileSizeAck, FILE_SIZE_BUFF, MSG_NOSIGNAL, (struct sockaddr*)&address, sizeof(address)) == -1)
					{
						request.port = -4;
						return; // The server will clean itself up after timeout. 
					}
				}
				else if(!(blocks[current]->recvClient(buffer, bytes)))
				{
					request.port = -4;
					return; // The server will clean itself up after timeout. 
				}			
			}
			// If bytes are 0 that means the timer timed out and stopped the recv so it could send. In this case we want to keep looping. 
		}
	}
	
	// We got all the data.
	*dataSize = fileS;
	data = new char[fileS];
	int pos = 0;
	for(int i = 0; i < (int)blocks.size(); i++)
	{
		blocks[i]->getData(data + pos);
		pos += blocks[i]->getSize();
	}
}

bool FileDownloader::fileSizeTimeout()
{
	// Random loss potential
	if(!downloadManager->user->losePacket())
	{
		if(sendto(servSocket, openerForTimeout, OPENER_SIZE, MSG_NOSIGNAL, (struct sockaddr*)&address, sizeof(address)) == -1)
		{
			request.port = -4;
			return false;
		}
	}
	else
		downloadManager->user->bufferMessage("Random loss: resend fileSize due to timeout");
	
	return true;
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
	close(servSocket);
}





