#include "FileSender.h"
#include "Server.h"
#include "Communication.h"

#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <vector>

//!!!!! IMPORTANT: File sender does not actually recv any data, instead it gets it all from the server from handleData. This is because UDP does not support connecitons, so at the OS level it has no idea where to send. 
FileSender::FileSender(Server* server, int threadIndex):
server(server),
threadIndex(threadIndex)
{
	pthread_mutex_init(&requestMutex, NULL);
	pthread_cond_init(&requestCond, NULL);
	
	int rv = pthread_create(&thread, NULL, startFileSenderThread, this);
	if (rv != 0)
	{
		std::cout << "Error creating FileSender thread, exiting program" << std::endl;
		exit(-1);
	}
}

FileSender::~FileSender()
{
	pthread_mutex_destroy(&requestMutex);
	pthread_cond_destroy(&requestCond);
}

int FileSender::getThreadIndex()
{
	return threadIndex;
}

void FileSender::beginRequest(int cls, int port, std::string req)
{
	clientSocket = cls;
	clientPort = port;
	request = req;
	
	pthread_mutex_lock(&requestMutex);
	{
		pthread_cond_signal(&requestCond);
	}
	pthread_mutex_unlock(&requestMutex);
}

void FileSender::handleData(char* data, int recBytes)
{
	pthread_mutex_lock(&requestMutex);
	{
		dataQueue.push(std::string(data, recBytes));
		pthread_cond_signal(&requestCond);
	}
	pthread_mutex_unlock(&requestMutex);
}

int FileSender::getClientSocket()
{
	return clientSocket;
}

int FileSender::getClientPort()
{
	return clientPort;
}

void FileSender::awaitRequest()
{
	while(flag_running)
	{
		pthread_mutex_lock(&requestMutex);
		{
			server->reclaim(this, threadIndex);
			
			while(clientSocket == -1)
				pthread_cond_wait(&requestCond, &requestMutex);
		}
		pthread_mutex_unlock(&requestMutex);
		
		if(request[0] == FILE_LIST)
			handleFileList();
		else
			handleFile();

		close(clientSocket);
		clientSocket = -1;
	}
}

void FileSender::handleFileList()
{
	// Inital comunication requires sending file size, in this case that is the bytes required to send all file names. (Handle file gets a filename to send the size for here.)
	std::vector<std::string> filenames = server->listFilenames();
	int size = 0;
	for(int i = 0; i < (int)filenames.size(); i++)
		size += filenames[i].size() + 1; // +1 for the \0 which will be included in its printout.
	
	while(flag_running)
	{
		std::string handleThis;
	
		// Figure out how to transmission timer. If we go over this after a few attempts we return to the Sever loop.
		pthread_mutex_lock(&requestMutex);
		{
			while(dataQueue.size() == 0)
				pthread_cond_wait(&requestCond, &requestMutex);
			
			handleThis = dataQueue.front();
			dataQueue.pop();
		}
		pthread_mutex_unlock(&requestMutex);
		
		// At this point handleThis is safe to use. 
		
		// Final ack detected, so we return back to Server pool.
	}
}

void FileSender::handleFile()
{
	std::cout << 1 << std::endl;
}
	
void* FileSender::startFileSenderThread(void* fileSender)
{
	FileSender* fs = (FileSender*)fileSender;
	fs->awaitRequest();
	
	delete fs;
	return NULL;
}

void FileSender::quit()
{
	flag_running = false;
}






