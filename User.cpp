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
	
	std::cout << "You can begin entering commands now, type 'help' to bring up the list of commands.\r\n(No caps or anything fancy for the commands)" << std::endl;
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
	char first = input[0];
	char sec = input[1];
	
	if(first == 'm' && sec == 'm')
		messageMode();
	
	else if(first == 'v' && sec == 'm')
		viewMessages();
	
	else if(first == 'd' && sec == 'd')
		details();
	
	else if(first == 's' && sec == 'd')
		serverDetails();
	
	else if(first == 'f' && sec == 'l')
		fileList(input);
	
	else if(first == 'd' && sec == 'f')
		download(input);
	
	else if(first == 'h' && sec == 'e' && input[2] == 'l' && input[3] == 'p')
		help();
	
	else if(first == 'q' && sec == 'q')
		quit();
	
	else
		std::cout << "No command recognized." << std::endl;
}

void User::help()
{
	std::cout
	<< "Type in 'mm' to turn on message mode. In this mode you can not enter commands, but all incoming messages are displayed immediately." << std::endl
	<< "Type in 'vm' to view currently buffered messages." << std::endl
	<< "Type in 'dd' to view the Download Manager details (thread count and ongoing downloads)." << std::endl
	<< "Type in 'sd' to view the Server details (thread count, your ip, port, files hosted)." << std::endl
	<< "Type in 'fl <host ip> <host port>' to view the list of files at the address." << std::endl
	<< "Type in 'df <host ip> <host port> <filename>' to download 'filename' from the address." << std::endl
	<< "Type in 'qq' to quit" << std::endl
	<< std::endl;
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

void User::fileList(std::string input)
{
	
}

void User::download(std::string input)
{
	
}

void User::quit()
{
	downloadManager->quit();
	server->quit();
}











