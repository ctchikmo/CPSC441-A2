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
	serverData = new char[size + HEADER_SIZE];
	serverData[BLOCK_BYTE] = blockNum;
	serverData[KEY_BYTE] = OCTLEG_KEY;
	serverData[LEG_BYTE] = legNum;
	
	for(int i = 0; i < size; i++)
		serverData[DATA_START_BYTE + i] = d[i];
}

Octoleg::~Octoleg()
{
	if(data != NULL)
		delete[] data;
	
	else if(serverData != NULL)
		delete[] serverData;
}

int Octoleg::getDataSize()
{
	return size;
}

char* Octoleg::getData()
{
	return data;
}

char Octoleg::getLegNum()
{
	return legNum;
}

bool Octoleg::serverSendData()
{
	if(send(sender->getClientSocket(), serverData, size + HEADER_SIZE, MSG_NOSIGNAL) == -1)
		return false; // The client will handle clean up if this happens.
	
	return true;
}

bool Octoleg::serverAskForAck()
{
	char askAck[HEADER_SIZE];
	askAck[BLOCK_BYTE] = blockNum;
	askAck[KEY_BYTE] = ASK_ACK_KEY;
	askAck[LEG_BYTE] = legNum;
	
	if(send(sender->getClientSocket(), askAck, sizeof(askAck), MSG_NOSIGNAL) == -1)
		return false; // The client will handle clean up if this happens.
	
	return true;
}

bool Octoleg::clientRecvFileData(char* d, int s)
{
	// If the data is not the excpected size we return.
	if(s != size)
		return false;
	
	for(int i = 0; i < s; i++)
		data[i] = d[i];
	
	return true;
}

bool Octoleg::clientSendAck()
{
	char sendAck[HEADER_SIZE];
	sendAck[BLOCK_BYTE] = blockNum;
	sendAck[KEY_BYTE] = ACK_KEY;
	sendAck[LEG_BYTE] = legNum;
	
	if(sendto(downloader->getServSocket(), sendAck, sizeof(sendAck), MSG_NOSIGNAL, (struct sockaddr*)downloader->getServAddress(), sizeof(*downloader->getServAddress())) == -1)
		return false; // The client will handle clean up if this happens.
	
	return true;
}

bool Octoleg::clientAskForRetransmit()
{
	char askRetrans[HEADER_SIZE];
	askRetrans[BLOCK_BYTE] = blockNum;
	askRetrans[KEY_BYTE] = ASK_TRANS_KEY;
	askRetrans[LEG_BYTE] = legNum;
	
	if(sendto(downloader->getServSocket(), askRetrans, sizeof(askRetrans), MSG_NOSIGNAL, (struct sockaddr*)downloader->getServAddress(), sizeof(*downloader->getServAddress())) == -1)
		return false; // The server will handle clean up if this happens.
	
	return true;
}








