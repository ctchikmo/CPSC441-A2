#include "FileSender.h"
#include "Server.h"

#include <iostream>
#include <stdlib.h>

FileSender::FileSender(Server* server, int threadIndex)
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

void FileSender::beginRequest(int cls)
{
	clientSocket = cls;
	
	pthread_mutex_lock(&requestMutex);
	{
		pthread_cond_signal(&requestCond);
	}
	pthread_mutex_unlock(&requestMutex);
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
		
		handleRequest();
		clientSocket = -1;
	}
}

void FileSender::handleRequest()
{
	
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






