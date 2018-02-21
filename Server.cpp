#include "Server.h"
#include "FileSender.h"
#include "User.h"

#include <iostream>
#include <stdlib.h>
#include <dirent.h>

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

// The accessor of this method must account for the mutexes and conditions (the method itself does not as it is used as a check from within this mutex, which would cause a deadlock if both did)
bool Server::isReady()
{
	return flag_ready;
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
	std::cout << "Host port: " << port << std::endl;
	
	std::cout << "Hosted files: (items with no extension are folders)" << std::endl;
	std::vector<std::string> filenames = listFilenames();
	for(int i = 0; i < (int)filenames.size(); i++)
		std::cout << " - " << filenames[i] << std::endl;
}

// Thanks @ https://stackoverflow.com/questions/306533/how-do-i-get-a-list-of-files-in-a-directory-in-c answer by Chris Kloberdanz
std::vector<std::string> Server::listFilenames()
{
	DIR *dpdf;
	struct dirent *epdf;
	std::vector<std::string> rv;

	dpdf = opendir((user->getDirectory()).c_str());
	int count = 0; // We skip the first 2 files as they are the directory ones: . and ..
	
	if(dpdf != NULL)
	{
		while((epdf = readdir(dpdf)))
		{
			if(count == 2)
				rv.push_back(epdf->d_name);
			else
				count++;
		}
		
		closedir(dpdf);
	}
	else
		std::cout << "Check the directory you entered, it may be invalid." << std::endl;
	
	return rv;
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
	
	return NULL;
}

void Server::quit()
{
	flag_running = false;
	for(int i = 0; i < numThreads; i++)
		if(inProgress[i] != NULL)
			inProgress[i]->quit();
}




