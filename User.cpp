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
	
	std::cout << "You can begin entering commands now, type 'help' to bring up the list of commands." << std::endl << "(No caps or anything fancy for the commands)" << std::endl << std::endl;
	std::cout << "As a check for what you're hosting, here are the server details" << std::endl << "(this can be seen again using the command 'sd')" << std::endl;
	serverDetails();
	
	std::string input;
	while(std::getline(std::cin, input))
	{
		if(!handleCommand(input))
			break;
	}
}

void User::bufferMessage(std::string message)
{
	if(flag_messageMode)
		std::cout << " + " << message << std::endl;
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

bool User::losePacket()
{
	if(randomLossChance > 0)
	{
		int rando = rand() % 100 + 1; // Gets a number between 1 and 100
		if(randomLossChance >= rando)
			return true;
	}
	
	return false; // Default, random loss off. 
}

bool User::handleCommand(std::string input)
{
	if(input.size() == 2 && input[0] == 'm' && input[1] == 'm')
		return messageMode();
	
	else if(input.size() == 2 && input[0] == 'v' && input[1] == 'm')
		return viewMessages();
	
	else if(input.size() == 2 && input[0] == 'd' && input[1] == 'd')
		return details();
	
	else if(input.size() == 2 && input[0] == 's' && input[1] == 'd')
		return serverDetails();
	
	else if(input[0] == 'p' && input[1] == 'l')
		return randomPacketLoss(input);
	
	else if(input[0] == 'f' && input[1] == 'l')
		return fileList(input);
	
	else if(input[0] == 'd' && input[1] == 'f')
		return download(input);
	
	else if(input.size() == 4 && input[0] == 'h' && input[1] == 'e' && input[2] == 'l' && input[3] == 'p')
		return help();
	
	else if(input.size() == 2 && input[0] == 'q' && input[1] == 'q')
		return quit();
	
	else
		std::cout << "No command recognized." << std::endl << std::endl;
	
	return true;
}

bool User::help()
{
	std::cout
	<< "Type in 'mm' to turn on message mode. All incoming messages are displayed immediately. Also displays any buffered messages." << std::endl
	<< "Type in 'vm' to view currently buffered messages." << std::endl
	<< "Type in 'dd' to view the Download Manager details (thread count and ongoing downloads)." << std::endl
	<< "Type in 'sd' to view the Server details (thread count, port, files hosted)." << std::endl
	<< "Type in 'pl <% loss chance(0 - 100)>' to turn on random packet loss (if 0 is entered no packets are lost, starts at 0)." << std::endl
	<< "Type in 'fl <host ip> <host port>' to view the list of files at the address." << std::endl
	<< "Type in 'df <host ip> <host port> <filename>' to download 'filename' from the address." << std::endl
	<< "Type in 'qq' to quit (If in message mode typing qq quits message mode.)" << std::endl
	<< std::endl;
	
	return true;
}

bool User::messageMode()
{
	if(flag_messageMode)
	{
		flag_messageMode = false;
		std::cout << "Message mode is off" << std::endl << std::endl;
	}
	else
	{
		flag_messageMode = true;
		std::cout << "Message mode on" << std::endl;
		viewMessages();
	}
	
	return true;
}

bool User::viewMessages()
{
	pthread_mutex_lock(&messageMutex);
	{
		std::cout << "Printing " << messageBuffer.size() << " message(s):" << std::endl;
		if((int)messageBuffer.size() != 0)
		{
			for(int i = 0; i < (int)messageBuffer.size(); i++)
				std::cout << " + " << messageBuffer[i] << std::endl;
			
			messageBuffer.clear();
		}
		std::cout << std::endl;
	}
	pthread_mutex_unlock(&messageMutex);
	
	return true;
}

bool User::details()
{
	std::cout << "Client downloading details:" << std::endl;
	downloadManager->details();
	std::cout << std::endl;
	
	return true;
}

bool User::serverDetails()
{
	std::cout << "Server hosting details:" << std::endl;
	server->details();
	std::cout << std::endl;
	
	return true;
}

bool User::randomPacketLoss(std::string input)
{
	int chance;
	std::string str_chance;
	
	if((int)input.size() >= 4)
		str_chance.assign(input.substr(3, std::string::npos));
	else
	{
		std::cout << "Missing chance argument" << std::endl << std::endl;
		return true;
	}
	
	try
	{
		chance = std::stoi(str_chance);
		
		if(chance < 0 || chance > 100)
		{
			std::cout << "Please enter a chance between 0 and 100 (inclusive on both)" << std::endl;
			return true;
		}
		
		randomLossChance = chance;
	}
	catch(...)
	{
		std::cout << "Error converting chance to an interger." << std::endl << std::endl;
		return true;
	}
	
	std::cout << "Random loss with " << randomLossChance << "% loss chance." << std::endl << std::endl;
	return true;
}

bool User::fileList(std::string input)
{
	int endOfIP; // This is the position of the serperating space
	int endOfPort; // This is the position of the serperating space
	
	if((int)input.size() >= 4)
		endOfIP = input.find(' ', 3);
	else
	{
		std::cout << "Missing ip argument" << std::endl << std::endl;
		return true;
	}
	
	if((int)input.size() >= endOfIP + 2 && endOfIP != -1)
		endOfPort = input.find(' ', endOfIP + 1);
	else
	{
		std::cout << "Missing port argument" << std::endl << std::endl;
		return true;
	}
	
	std::string hostIP = input.substr(3, (endOfIP - 3));
	std::string hostPort = input.substr(endOfIP + 1, (endOfPort - endOfIP - 1));
	
	int port;
	
	try
	{
		port = std::stoi(hostPort);
		std::cout << "IP: " << hostIP << std::endl;
		std::cout << "Port: " << port << std::endl;
		
		// This only runs if the exception is not thrown.
		std::cout << downloadManager->fileList(hostIP, port) << std::endl << std::endl;
	}
	catch(...)
	{
		std::cout << "Error converting hostport to an interger." << std::endl << std::endl;
	}
	
	return true;
}

bool User::download(std::string input)
{
	int endOfIP; // This is the position of the serperating space
	int endOfPort; // This is the position of the serperating space
	
	if((int)input.size() >= 4)
		endOfIP = input.find(' ', 3);
	else
	{
		std::cout << "Missing ip argument" << std::endl << std::endl;
		return true;
	}
	
	if((int)input.size() >= endOfIP + 2 && endOfIP != -1)
		endOfPort = input.find(' ', endOfIP + 1);
	else
	{
		std::cout << "Missing port argument" << std::endl << std::endl;
		return true;
	}
	
	std::string hostIP = input.substr(3, (endOfIP - 3));
	std::string hostPort = input.substr(endOfIP + 1, (endOfPort - endOfIP - 1));
	std::string filename;
	
	if((int)input.size() >= endOfPort + 2 && endOfPort != -1)
		filename = input.substr(endOfPort + 1, std::string::npos);
	else
	{
		std::cout << "Missing filename argument" << std::endl << std::endl;
		return true;
	}
	
	int port;
	
	try
	{
		port = std::stoi(hostPort);
		std::cout << "IP: " << hostIP << std::endl;
		std::cout << "Port: " << port << std::endl;
		std::cout << "Filename: " << filename << std::endl;
		
		// This only runs if the exception is not thrown.
		std::cout << downloadManager->download(hostIP, port, filename) << std::endl << std::endl;
	}
	catch(...)
	{
		std::cout << "Error converting hostport to an interger." << std::endl << std::endl;
	}
	
	return true;
}

bool User::quit()
{
	if(flag_messageMode)
	{
		flag_messageMode = false;
		std::cout << "Message mode is off" << std::endl << std::endl;
		return true;
	}
	else
	{
		downloadManager->quit();
		server->quit();
		return false;
	}
}











