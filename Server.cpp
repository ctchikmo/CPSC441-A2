#include "Server.h"
#include "FileSender.h"
#include "User.h"

#include <iostream>
#include <stdlib.h>

Server::Server(int port, int threads):
port(port),
numThreads(threads)
{
	pthread_mutex_init(&readyMutex, NULL);
	pthread_cond_init(&readyCond, NULL);
	
	pthread_mutex_init(&senderMutex, NULL);
	pthread_cond_init(&senderCond, NULL);
	
	inProgress = new FileSender *[numThreads];
	for(int i = 0; i < numThreads; i++)
		inProgress[i] = NULL;
	
	for(int i = 0; i < numThreads; i++)
		new FileSender(this, i);
	
	pthread_mutex_lock(&senderMutex); // User thread waits on this. 
	{
		while(senders.size() == 0)
			pthread_cond_wait(&senderCond, &senderMutex);
	}
	pthread_mutex_unlock(&senderMutex);
	
	// At least one FileSender is ready, so we proceed to start the Server thread.
	int rv = pthread_create(&thread, NULL, startServerThread, this);
	if (rv != 0)
	{
		std::cout << "Error creating Server thread, exiting program" << std::endl;
		exit(-1);
	}
}

Server::~Server()
{
	pthread_mutex_destroy(&readyMutex);
	pthread_cond_destroy(&readyCond);
	
	pthread_mutex_destroy(&senderMutex);
	pthread_cond_destroy(&senderCond);
	
	// No for loop to delete pointers inside of inProgress because when the queue has its dtor called they are deleted there. 
	delete[] inProgress;
}

bool Server::isReady()
{
	bool rv = false;
	
	pthread_mutex_lock(&readyMutex);
	{
		rv = flag_ready;
	}
	pthread_mutex_unlock(&readyMutex);
	
	return rv;
}

void Server::setUser(User* user)
{
	this->user = user;
}

void Server::reclaim(FileSender* fileSender, int threadIndex)
{
	pthread_mutex_lock(&senderMutex);
	{
		inProgress[threadIndex] = NULL; // Remove the thread from the inProgress array. 
		
		senders.push(fileSender); // Add the downloader back to the pool.
		pthread_cond_signal(&senderCond); // Ctor and connection acceptor wait on this, however they do so exclusivly (ctor must be signalled before acceptor starts).
	}
	pthread_mutex_unlock(&senderMutex);
}

void Server::details()
{
	std::cout << "Total file senders: " << numThreads << std::endl;
	std::cout << "Host ip: " << port << std::endl;
	std::cout << "Host port: " << port << std::endl;
	
	std::cout << "Hosted files: ";
	std::cout << std::endl;
}

pthread_mutex_t* Server::getReadyMutex()
{
	return &readyMutex;
}

pthread_cond_t* Server::getReadyCond()
{
	return &readyCond;
}

void Server::startupServer()
{
	pthread_mutex_lock(&readyMutex);
	{
		flag_ready = true;
		pthread_cond_signal(&readyCond);
	}
	pthread_mutex_unlock(&readyMutex);
}

void* Server::startServerThread(void* server)
{
	Server* s = (Server*)server;
	s->startupServer();
	
	delete s;
	return NULL;
}

void Server::quit()
{
	flag_running = false;
	for(int i = 0; i < numThreads; i++)
		if(inProgress[i] != NULL)
			inProgress[i]->quit();
}




