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

void FileSender::beginRequest(int clientSocket)
{
	
}

void FileSender::awaitRequest()
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
	
}






