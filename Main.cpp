/// We are doing linux only, so no need to worry about selecting the proper cd finding method depending on OS. 
#include "User.h"
#include "DownloadManager.h"
#include "Server.h"

#include <iostream>
#include <fstream>
#include <string>
#include <unistd.h>
#include <stdlib.h>

/*
 * REALLY IMPORTANT ASSUMPTION:
 * + Anything that does not have to do with the actual File data does not need to be Octouput'd (as the assignment only talks about Data Files). The following are excluded from octo
 *	 - Filename
 *	 - Filesize
 * 	 - Connection packet
 * 	 - Acknowledgement packet
 *   - Any other realiability function
*/


// There are 4 arguments: host port, directory to store/host files, Number of Download threads (1 file per thread), Number of Server threads (1 file per thread)
int main(int argc, char const* argv[])
{
	if(argc != 5)
	{
		std::cout << "Please use the start sequence: Octouput.exe <Host port (0 for random)> <File download/host directory (cd for current)> <Number downloader threads> <Number server threads>." << std::endl;
		exit(1);
	}
	
	int hostPort;
	std::string directory(argv[2]);
	int downThreads;
	int serverThreads;
	
	try
	{
		hostPort = std::stoi(argv[1]);
	}
	catch(...) // ... Means all exceptions
	{
		std::cout << "Error converting hostport to an interger." << std::endl;
		exit(-1);
	}
	
	try
	{
		downThreads = std::stoi(argv[3]);
	}
	catch(...)
	{
		std::cout << "Error converting downloadThreads to an interger." << std::endl;
		exit(-1);
	}
	
	try
	{
		serverThreads = std::stoi(argv[4]);
	}
	catch(...)
	{
		std::cout << "Error converting serverThreads to an interger." << std::endl;
		exit(-1);
	}
	
	if(directory[0] == 'c' && directory[1] == 'd' && directory.length() == 2)
	{
		char buffer[100];
		char* currentDir = getcwd(buffer, sizeof(buffer)); // Thanks @ https://stackoverflow.com/questions/2868680/what-is-a-cross-platform-way-to-get-the-current-directory
		if(currentDir)
			directory = currentDir;
	}
	
	if(hostPort < 0 || hostPort > 65535)
	{
		std::cout << "Please enter a valid host port (0 - 65535)." << std::endl;
		exit(1);
	}
	
	if(downThreads < 1 || downThreads > 8)
	{
		std::cout << "Please enter a number for the number of download threads (1 - 8)." << std::endl;
		exit(1);
	}
	
	if(serverThreads < 1 || serverThreads > 8)
	{
		std::cout << "Please enter a number for the number of server threads (1 - 8)." << std::endl;
		exit(1);
	}
	
	std::cout << "Host port: " << (hostPort == 0 ? "Will be determined once Server has finished startup" : std::to_string(hostPort)) << std::endl 
	<< "Directory: " << directory << std::endl 
	<< "Download Threads: " << downThreads << std::endl 
	<< "Server Threads: " << serverThreads << std::endl 
	<< "Starting app..." << std::endl << std::endl;
	
	// At this point all the cmd startup is done and it is time for the user to start issuing commands, the server to make files available, and the downloaders to download. 
	// Note that all the constructors take place on this main thread, so no need to use the user message service. 
	User user(directory);
	DownloadManager manager(downThreads);
	Server server(hostPort, serverThreads);
	
	user.setDownloadManager(&manager);
	user.setServer(&server);
	manager.setUser(&user);
	server.setUser(&user);
	
	user.beginInputLoop();
	
	return 0;
}









