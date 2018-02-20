#include "DownloadManager.h"

DownloadManager::DownloadManager(int threads)
{
	
}

DownloadManager::~DownloadManager()
{
	
}

bool DownloadManager::isReady()
{
	
}

std::string DownloadManager::fileList(std::string ip, int port)
{
	
}

std::string DownloadManager::download(std::string ip, int port, std::string filename)
{
	
}

void DownloadManager::reclaim(FileDownloader* fileDownloader, int threadIndex, std::string userMessage)
{
	
}

std::string DownloadManager::details()
{
	
}

void DownloadManager::setUser(User* user)
{
	
}

pthread_mutex_t* DownloadManager::getReadyMutex()
{
	
}

pthread_cond_t* DownloadManager::getReadyCond()
{
	
}

void* DownloadManager::startDownloadManagerThread(void* downloadManager)
{
	
}




