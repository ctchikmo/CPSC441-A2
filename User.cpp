#include "User.h"
#include "DownloadManager.h"
#include "Server.h"

#include <iostream>
#include <stdlib.h>

User::User(std::string directory):
directory(directory)
{
	pthread_mutex_init(&messageMutex, NULL);
}

User::~User()
{
	pthread_mutex_destroy(&messageMutex);
}

void User::setDownloadManager(DownloadManager* dm)
{
	downloadManager = dm;
}

void User::setServer(Server* s)
{
	server = s;
}

void User::beginInputLoop()
{
	// Wait for server to be ready. 
	pthread_mutex_lock(server->getReadyMutex());
	{
		while(!server->isReady())
			pthread_cond_wait(server->getReadyCond(), server->getReadyMutex());
	}
	pthread_mutex_unlock(server->getReadyMutex());
	
	std::cout << "You can begin entering commands now, type 'Help' to bring up the list of commands." << std::endl;
}

void User::bufferMessage(std::string message)
{
	
}

std::string& User::getDirectory()
{
	return directory;
}

void User::handleCommand(std::string input)
{
	
}

void User::help()
{
	
}

void User::messageMode()
{
	
}

void User::viewMessages()
{
	
}

void User::details()
{
	std::cout << "Client downloading details:" << std::endl;
	downloadManager->details();
	std::cout << std::endl;
}

void User::serverDetails()
{
	std::cout << "Server hosting details:" << std::endl;
	server->details();
	std::cout << std::endl;
}

void User::fileList(std::string ip, std::string port)
{
	
}

void User::download(std::string ip, std::string port, std::string filename)
{
	
}

void User::quit()
{
	
}











