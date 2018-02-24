#include "FileSender.h"
#include "Server.h"
#include "Communication.h"
#include "User.h"

#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <vector>
#include <sys/stat.h> // get file size
#include <fstream> 

//!!!!! IMPORTANT: File sender does not actually recv any data, instead it gets it all from the server from handleData. This is because UDP does not support connecitons, so at the OS level it has no idea where to send. 
FileSender::FileSender(Server* server, int threadIndex):
server(server),
threadIndex(threadIndex)
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

void FileSender::beginRequest(int cls, int port, std::string req)
{
	clientSocket = cls;
	clientPort = port;
	request = req;
	
	pthread_mutex_lock(&requestMutex);
	{
		pthread_cond_signal(&requestCond);
	}
	pthread_mutex_unlock(&requestMutex);
}

void FileSender::handleData(char* data, int recBytes)
{
	pthread_mutex_lock(&requestMutex);
	{
		dataQueue.push(std::string(data, recBytes));
		pthread_cond_signal(&requestCond);
	}
	pthread_mutex_unlock(&requestMutex);
}

int FileSender::getClientSocket()
{
	return clientSocket;
}

int FileSender::getClientPort()
{
	return clientPort;
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
		
		if(request[0] == FILE_LIST)
			handleFileList();
		else
			handleFile();

		close(clientSocket);
		clientSocket = -1;
	}
}

void FileSender::handleFileList()
{
	// Inital comunication requires sending file size, in this case that is the bytes required to send all file names. (Handle file gets a filename to send the size for here.)
	std::vector<std::string> filenames = server->listFilenames();
	int size = 0;
	for(int i = 0; i < (int)filenames.size(); i++)
		size += filenames[i].size() + 1; // +1 for the \n which will be included in its printout.
	
	char toSend[size];
	int pos = 0;
	for(int i = 0; i < (int)filenames.size(); i++)
	{
		for(int j = 0; j < (int)filenames[i].size(); j++)
		{
			toSend[pos + j] = filenames[i][j];
		}
		
		toSend[pos + filenames[i].size()] = '\n';
		pos += filenames[i].size() + 1;
	}
	
	server->user->bufferMessage("Server sending file list to client on port " + std::to_string(clientPort));
	generalHandler(size, toSend);
	server->user->bufferMessage("Server done sending file list to client on port " + std::to_string(clientPort));
}

void FileSender::handleFile()
{
	std::string filename = "";
	for(int i = 0; (request[OPENER_FILENAME + i] != '\0'); i++)
		filename += request[OPENER_FILENAME + i];
	std::string path = server->user->getDirectory() + "/" + filename;
	
	// Inital comunication requires sending file size, in this case that is the bytes required to send all file names. (Handle file gets a filename to send the size for here.)
	int size = 0;
	
	// Thanks @ https://stackoverflow.com/questions/5840148/how-can-i-get-a-files-size-in-c Matts answer
	struct stat stat_buf;
    int rc = stat(path.c_str(), &stat_buf);
    size = (rc == 0 ? stat_buf.st_size : -1);
	
	if(size == -1)
	{
		server->user->bufferMessage("Server unable to find file: \"" + filename + "\", asked for by client on port " + std::to_string(clientPort));
		std::string sizeString = "-1";
		if(send(clientSocket, sizeString.c_str(), sizeString.size(), MSG_NOSIGNAL) == -1) // Send the -1 filesize. 
			return;
	}
	else
	{
		char toSend[size];
		std::ifstream file(path, std::ios::binary); // Thanks @ https://stackoverflow.com/questions/18816126/c-read-the-whole-file-in-buffer accepted answer

		if(file.read(toSend, size))
		{
			file.close();
			server->user->bufferMessage("Server sending " + filename + " to client on port " + std::to_string(clientPort));
			generalHandler(size, toSend);
			server->user->bufferMessage("Server done sending " + filename + " to client on port " + std::to_string(clientPort));
		}
	}
}

// I would perfer streaming the bytes, incase we get files bigger than the available ram, but thats way out of scope of this assignment. 
bool FileSender::generalHandler(int size, char* toSend)
{
	std::string sizeString = std::to_string(size);
	if(send(clientSocket, sizeString.c_str(), sizeString.size(), MSG_NOSIGNAL) == -1) // Send the filesize. 
		return false; // Client will time out.
		
	std::queue<Octoblock*> blocks;
	Octoblock::getOctoblocks(&blocks, size, toSend, this);
	
	if(blocks.size() == 0)
		return true;
	
	Octoblock* current = blocks.front();
	blocks.pop();
	
	if(!(current->serverSendData()))
		return false;
	
	while(flag_running)
	{
		std::string handleThis;
	
		// Figure out how to transmission timer. If we go over this after a few attempts we return to the Sever loop.
		pthread_mutex_lock(&requestMutex);
		{
			while(dataQueue.size() == 0)
				pthread_cond_wait(&requestCond, &requestMutex);
			
			handleThis = dataQueue.front();
			dataQueue.pop();
		}
		pthread_mutex_unlock(&requestMutex);
		
		// At this point handleThis is safe to use. 
		int rvMessage = current->recvServer(handleThis.c_str(), handleThis.size());
		if(rvMessage == RECV_SERVER_ERROR)
			return false;
		else if(rvMessage == RECV_SERVER_DIF_BLOCK)
		{
			current = blocks.front();
			blocks.pop();
			
			if(!(current->serverSendData()))
				return false;
		}
		
		if(current->complete())
		{
			if(blocks.size() == 0)
				break;
			else
			{
				current = blocks.front();
				blocks.pop();
				
				if(!(current->serverSendData()))
					return false;
			}
		}
	}
	
	return true;
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






