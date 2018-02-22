#include "Communication.h"

#include <sys/socket.h>

Octoblock::Octoblock(char blockNum, int size, FileDownloader* downloader)
{
	
}

Octoblock::Octoblock(char blockNum, int size, char* data, FileSender* sender)
{
	
}

Octoblock::~Octoblock()
{
	
}

bool Octoblock::sendRequest()
{
	
}

bool Octoblock::recvClient(char* data, int size)
{
	
}

bool Octoblock::serverSendData()
{
	
}

int Octoblock::recvServer(char* data, int size)
{
	
}

bool Octoblock::complete()
{
	
}

std::vector<Octoblock> Octoblock::getOctoblocks(int fileSize, FileDownloader* downloader)
{
	
}

std::queue<Octoblock> Octoblock::getOctoblocks(int fileSize, char* data, FileSender* sender)
{
	
}







