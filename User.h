#ifndef USER_H
#define USER_H

#include <pthread.h>
#include <vector>
#include <string>

#define CMD_HELP 		"help"
#define CMD_HELP_SHORT 	"h"

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
		
	private:
		pthread_mutex_t messageMutex;
	
		std::vector<std::string> messageBuffer;
		std::string directory;
	
		DownloadManager* downloadManager;
		Server* server;
		
		void handleCommand(std::string input);
		void help();
		void messageMode(); // Only input allowed is "r" to return to user mode. In this mode all messages recieved are displayed asap.
		void viewMessages(); // Displays all currently buffered messages. 
		void details(); // Shows client details (File Downloader thread count, File Downloaders in use(each in use displays file fetching))
		void serverDetails(); // Shows server details (File Sender thread count, Files hosted (whats in file folder), host port, host ip)
		void fileList(std::string ip); // Lists the files available at the input ip and port (this can be used on the own machines server, but serverDetails is preffered.)
		void download(std::string ip); // Attempts to download the given file from the desired location. 
		void quit();
};

#endif