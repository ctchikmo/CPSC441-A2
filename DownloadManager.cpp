#include "User.h"
#include "DownloadManager.h"
#include "FileDownloader.h"

#include <iostream>
#include <stdlib.h>

DownloadManager::DownloadManager(int threads):
numThreads(threads)
{
	pthread_mutex_init(&requestMutex, NULL);
	pthread_cond_init(&requestCond, NULL);
	
	inProgress = new FileDownloader *[numThreads];
	for(int i = 0; i < numThreads; i++)
		inProgress[i] = NULL;
	
	for(int i = 0; i < numThreads; i++)
		new FileDownloader(this, i); // These add themselves once rady via the reclaim method. 
	
	// The DownloadManager is ready once at least one FileDownloader is up and running. This holds back the user thread as it calls the DownloadManager ctor.
	pthread_mutex_lock(&requestMutex);
	{
		while(downloaders.size() == 0)
			pthread_cond_wait(&requestCond, &requestMutex);
	}
	pthread_mutex_unlock(&requestMutex);
	
	// At least one FileDownloader is ready, so we proceed to start the DownloadManager thread.
	int rv = pthread_create(&thread, NULL, startDownloadManagerThread, this);
	if (rv != 0)
	{
		std::cout << "Error creating DownloadManager thread, exiting program" << std::endl;
		exit(-1);
	}
	
	std::cout << "Downloaders usable." << std::endl;
}

DownloadManager::~DownloadManager()
{
	pthread_mutex_destroy(&requestMutex);
	pthread_cond_destroy(&requestCond);
	
	// No for loop to delete pointers inside of inProgress because when the queue has its dtor called they are deleted there. 
	delete[] inProgress;
}

void DownloadManager::reclaim(FileDownloader* fileDownloader, int threadIndex, std::string userMessage)
{
	user->bufferMessage(userMessage); // Let the user display the message
	
	pthread_mutex_lock(&requestMutex);
	{
		inProgress[threadIndex] = NULL; // Remove the thread from the inProgress array. 
		downloaders.push(fileDownloader); // Add the downloader back to the pool.
		pthread_cond_signal(&requestCond);
	}
	pthread_mutex_unlock(&requestMutex);
}

void DownloadManager::requestConsumer()
{
	while(flag_running)
	{
		pthread_mutex_lock(&requestMutex);
		{
			while(downloaders.size() == 0 || requests.size() == 0)
				pthread_cond_wait(&requestCond, &requestMutex);
			
			// At this point there is at least on downloader and one request. 
			Request reqeust = requests.front();
			requests.pop();
			
			FileDownloader* fileDownloader = downloaders.front();
			downloaders.pop();
			
			fileDownloader->beginRequest(reqeust);
			inProgress[fileDownloader->getThreadIndex()] = fileDownloader;
			
			user->bufferMessage("Request for: " + reqeust.filename + " from: " + reqeust.ip + " on port: " + std::to_string(reqeust.port) + " started.");
		}
		pthread_mutex_unlock(&requestMutex);
	}
}

std::string DownloadManager::fileList(std::string ip, int port)
{
	return requestAdder(RequestType::LIST, ip, port, "File List");
}

std::string DownloadManager::download(std::string ip, int port, std::string filename)
{
	return requestAdder(RequestType::DOWNLOAD, ip, port, filename);
}

std::string DownloadManager::requestAdder(RequestType type, std::string ip, int port, std::string filename)
{
	pthread_mutex_lock(&requestMutex);
	{
		requests.push(Request(type, ip, port, filename));
		pthread_cond_signal(&requestCond);
	}
	pthread_mutex_unlock(&requestMutex);
	
	return "Request queued.";
}

void DownloadManager::details()
{
	std::cout << "Total file downloaders: " << numThreads << std::endl;
	for(int i = 0; i <numThreads; i++)
		if(inProgress[i] != NULL)
			inProgress[i]->details();
}

void DownloadManager::setUser(User* user)
{
	this->user = user;
}

void* DownloadManager::startDownloadManagerThread(void* downloadManager)
{
	DownloadManager* dm = (DownloadManager*)downloadManager;
	dm->requestConsumer();
	
	delete dm;
	return NULL;
}

void DownloadManager::quit()
{
	flag_running = false;
	for(int i = 0; i < numThreads; i++)
		if(inProgress[i] != NULL)
			inProgress[i]->quit();
}




