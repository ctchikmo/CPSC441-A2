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
	std::cout << std::endl << "Server up and running" << std::endl << std::endl;
	
	std::cout << "You can begin entering commands now, type 'help' to bring up the list of commands." << std::endl << "(No caps or anything fancy for the commands)" << std::endl;
}

void User::bufferMessage(std::string message)
{
	if(flag_messageMode)
		std::cout << message << std::endl;
	else
	{
		pthread_mutex_lock(&messageMutex);
		{
			messageBuffer.push_back(message);
		}
		pthread_mutex_unlock(&messageMutex);
	}
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
	<< "Type in 'mm' to turn on message mode. All incoming messages are displayed immediately. Also displays any buffered messages." << std::endl
	<< "Type in 'vm' to view currently buffered messages." << std::endl
	<< "Type in 'dd' to view the Download Manager details (thread count and ongoing downloads)." << std::endl
	<< "Type in 'sd' to view the Server details (thread count, your ip, port, files hosted)." << std::endl
	<< "Type in 'fl <host ip> <host port>' to view the list of files at the address." << std::endl
	<< "Type in 'df <host ip> <host port> <filename>' to download 'filename' from the address." << std::endl
	<< "Type in 'qq' to quit (If in message mode typing qq quits message mode.)" << std::endl
	<< std::endl;
}

void User::messageMode()
{
	flag_messageMode = true;
	viewMessages();
}

void User::viewMessages()
{
	pthread_mutex_lock(&messageMutex);
	{
		for(int i = 0; i < (int)messageBuffer.size(); i++)
		{
			std::cout << messageBuffer.back() << std::endl;
			messageBuffer.pop_back();
		}
	}
	pthread_mutex_unlock(&messageMutex);
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
	int endOfIP = input.find(' ', 3);
	int endOfPort = input.find(' ', endOfIP + 1);
	
	std::string hostIP = input.substr(3, (endOfIP - 3));
	std::string hostPort = input.substr(endOfIP + 1, (endOfPort - endOfIP - 1));
	
	int port;
	
	try
	{
		port = std::stoi(hostPort);
		
		// This only runs if the exception is not thrown.
		downloadManager->fileList(hostIP, port);
	}
	catch(...)
	{
		std::cout << "Error converting hostport to an interger." << std::endl;
	}
}

void User::download(std::string input)
{
	int endOfIP = input.find(' ', 3);
	int endOfPort = input.find(' ', endOfIP + 1);
	
	std::string hostIP = input.substr(3, (endOfIP - 3));
	std::string hostPort = input.substr(endOfIP + 1, (endOfPort - endOfIP - 1));
	std::string filename = input.substr(endOfPort + 1, std::string::npos);
	
	int port;
	
	try
	{
		port = std::stoi(hostPort);
		
		// This only runs if the exception is not thrown.
		downloadManager->download(hostIP, port, filename);
	}
	catch(...)
	{
		std::cout << "Error converting hostport to an interger." << std::endl;
	}
}

void User::quit()
{
	if(flag_messageMode)
		flag_messageMode = false;
	else
	{
		downloadManager->quit();
		server->quit();
	}
}











