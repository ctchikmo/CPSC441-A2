#include "Server.h"
#include "FileSender.h"
#include "User.h"
#include "Communication.h"

#include <arpa/inet.h> // inet_ntop
#include <iostream>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h> // Check if file is directory
#include <stdio.h>

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
	std::cout << "Host ip: " << "Auto assigned and bound to all local IPs (use the IPV4 address)" << std::endl;
	std::cout << "Host port: " << port << std::endl;
	
	std::cout << "Hosted files:" << std::endl;
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
	if(dpdf != NULL)
	{
		while((epdf = readdir(dpdf)))
		{
			std::string path = user->getDirectory() + "/" + epdf->d_name;
			
			if(!isDir(path.c_str()))
				rv.push_back(epdf->d_name);
		}
		
		closedir(dpdf);
	}
	else
		std::cout << "Check the directory you entered, it may be invalid." << std::endl;
	
	return rv;
}

// Thanks @ http://forum.codecall.net/topic/68935-how-to-test-if-file-or-directory/
bool Server::isDir(const char* path) 
{
    struct stat buf;
    stat(path, &buf);
    return S_ISDIR(buf.st_mode);
}

pthread_mutex_t* Server::getReadyMutex()
{
	return &readyMutex;
}

pthread_cond_t* Server::getReadyCond()
{
	return &readyCond;
}

// Thanks @ https://gist.github.com/listnukira/4045436 && @ https://stackoverflow.com/questions/1075399/how-to-bind-to-any-available-port
void Server::startupServer()
{
	serverSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(serverSocket < 0)
	{
		std::cout << "There was an error opening the server socket." << std::endl;
		exit(1);
	}
	
	serverAddressing.sin_family = AF_INET; // IPV4 byte ordering
	serverAddressing.sin_addr.s_addr = INADDR_ANY; // Use any/all of the computers registered IPs. 
	serverAddressing.sin_port = htons(port); // convert port into network byte order using htons. Note that if port is 0 than when we bind the OS will get us a free port. 
	
	if(bind(serverSocket, (struct sockaddr*) &serverAddressing, sizeof(serverAddressing)) < 0)
	{
		std::cout << "There was an error binding the server socket." << std::endl;
		exit(1);
	}
	
	struct sockaddr_in myAddress;
	int len = sizeof(myAddress);
	if(getsockname(serverSocket, (struct sockaddr*) &myAddress, &len) < 0)
	{
		std::cout << "Error reading server socket bind information" << std::endl;
		exit(1);
	}
	
	char myIP[16]; // The ip in the address is in byte form, but it is converted in the next step
	inet_ntop(AF_INET, &myAddress.sin_addr, myIP, sizeof(myIP)); // Convert byte form ip to char array
	ip.assign(myIP);
	port = ntohs(myAddress.sin_port); // Convert the byte form port to an int 
	
	pthread_mutex_lock(&readyMutex);
	{
		flag_ready = true;
		pthread_cond_signal(&readyCond);
	}
	pthread_mutex_unlock(&readyMutex);
	
	reciveData();
}

// I need to implement connections for UDP as thats how the thread handing off works. I don't do any queing cause of timeouts. The client side will do multiple attempts, so it has a good shot of getting a con. 
// Since UDP does not support connections though, my version is pretty simple. The server class gets all messages, then just hands them to the Sender thread associated with that client.
// In the connection area I: Create a socket for the FileSender to use, send the paidLoad body to the FileSender (this first message contains the file requested. Since this is not the exact File data its not octo'd)
void Server::reciveData()
{
	while(flag_running)
	{
		// Message buffering
		char* buffer = new char[SERVER_BUFFER];
		
		// This stuff is used to store the clients address info
		struct sockaddr_in clientInfo; // Very inportant, clientInfo stores the part of the quartent of info (client port) which diferentiates things.
		int clientInfoLen = sizeof(clientInfo);
		
		int recBytes = recvfrom(serverSocket, buffer, SERVER_BUFFER, 0, (struct sockaddr*)&clientInfo, &clientInfoLen);
		if(recBytes < 0)
		{
			std::cout << "Server error recieving data." << std::endl;
			perror("Error");
			exit(-1);
		}
		
		// The client is actually listening on a port sent in the opener. The source port needs to be kept here tho to compare from the port the client sends on.
		// There are 2 ports (1 for send, 1 for recv) becuase the thing would crash if while a thread was blocking on recv another tried to sendTo on that port. Docs says it should work, and sockets are just numbers
		// But w/e. 
		// PS: after some testing the port the client sends on and the one i bind to arbitrailly actually seem to end up matching, im not sure if this is guranteed though, so leaving
		// The requirement to send and read the port as the the payload will remain in. 
		int cliPort = ntohs(clientInfo.sin_port); // Convert the byte form port to an int 
		bool found = false;
		pthread_mutex_lock(&senderMutex);
		{
			for(int i = 0; i < numThreads; i++)
			{
				if(inProgress[i] != NULL && inProgress[i]->getClientPort() == cliPort)
				{
					inProgress[i]->handleData(buffer, recBytes);
					found = true;
				}
			}
		}
		pthread_mutex_unlock(&senderMutex);
		
		if(found)
		{
			delete[] buffer;
			continue;
		}
		
		std::string fileSenderData(buffer, recBytes);
		int clientRecvPort = std::stoi(&fileSenderData[OPENER_RECVPORT]);
		std::cout << cliPort  << "::" << clientRecvPort << std::endl;
		clientInfo.sin_port = htons(clientRecvPort);
		
		int clientSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if(clientSock < 0)
		{
			std::cout << "Error creating fileSender socket" << std::endl;
			exit(-1);
		}
		
		if(connect(clientSock, (struct sockaddr *)&clientInfo, clientInfoLen) < 0)
		{
			std::cout << "Error connecting fileSender socket" << std::endl;
			exit(-1);
		}
		
		FileSender* sender;
		pthread_mutex_lock(&senderMutex);
		{
			while(senders.size() == 0)
				pthread_cond_wait(&senderCond, &senderMutex);
			
			sender = senders.front();
			senders.pop();
			inProgress[sender->getThreadIndex()] = sender;
		}
		pthread_mutex_unlock(&senderMutex);
		
		// At this point we have the FileSender. We send the server socket as the client side is connected and expecting data from the source port assigned to this socket. Nothing else will be accepted. 
		sender->beginRequest(clientSock, cliPort, fileSenderData);
		
		delete[] buffer;
	}
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




