#ifndef USER_H
#define USER_H

#include <pthread.h>
#include <vector>
#include <string>

class DownloadManager; // Class forward as #include won't work (they use each other)
class Server; // Class forward as better than #include here

// User runs on the main thread and keeps it alive by staying in an input blocking loop
class User
{
	public:
		User(std::string directory);
		~User();
		
		void setDownloadManager(DownloadManager* dm);
		void setServer(Server* s);
		void beginInputLoop();
		void bufferMessage(std::string message); // Stores the message in the message buffer. These messages are displayed using the message cmd, or if left in messageMode they are displayed once recieved.
		
		std::string& getDirectory();
		bool losePacket(); // If random loss is off than this always returns false, if its on than it uses cmath to roll a random chance. 
		
	private:
		pthread_mutex_t messageMutex;
	
		bool flag_messageMode = false;
		int randomLossChance = 0;
		std::vector<std::string> messageBuffer;
		std::string directory;
	
		DownloadManager* downloadManager;
		Server* server;
		
		bool handleCommand(std::string input); // False to stop, true to keep going in the input loop. 
		bool help();
		bool messageMode(); // Only input allowed is "r" to return to user mode. In this mode all messages recieved are displayed asap.
		bool viewMessages(); // Displays all currently buffered messages. 
		bool details(); // Shows client details (File Downloader thread count, File Downloaders in use(each in use displays file fetching))
		bool serverDetails(); // Shows server details (File Sender thread count, Files hosted (whats in file folder), host port, host ip)
		bool randomPacketLoss(std::string input);
		bool fileList(std::string ip); // Lists the files available at the input ip and port (this can be used on the own machines server, but serverDetails is preffered.)
		bool download(std::string ip); // Attempts to download the given file from the desired location. 
		bool quit();
};

#endif