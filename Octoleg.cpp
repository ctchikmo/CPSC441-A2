#include "Communication.h"
#include "FileDownloader.h"
#include "FileSender.h"

#include <sys/socket.h>

Octoleg::Octoleg(char blockNum, char legNum, FileDownloader* downloader, int size):
blockNum(blockNum),
legNum(legNum),
downloader(downloader),
size(size)
{
	data = new char[size];
}

Octoleg::Octoleg(char blockNum, char legNum, FileSender* sender, int size, char* d):
blockNum(blockNum),
legNum(legNum),
sender(sender),
size(size)
{
	data = new char[size];
	for(int i = 0; i <size; i++)
		data[i] = d[i];
}

Octoleg::~Octoleg()
{
	delete[] data;
}

int Octoleg::getDataSize()
{
	return size;
}

char* Octoleg::getData()
{
	return data;
}

bool Octoleg::serverSendData()
{
	if(send(sender->getClientSocket(), data, size, MSG_NOSIGNAL) == -1)
		return false; // The client will handle clean up if this happens.
	
	return true;
}

bool Octoleg::serverAskForAck()
{
	
}

bool Octoleg::clientRecvFileData(char* data, int size)
{
	
}

bool Octoleg::clientSendAck()
{
	
}

bool Octoleg::clientAskForRetransmit()
{
	serverSendData();
}








