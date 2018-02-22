#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include <queue>

#define SERVER_BUFFER	1500

#define OPENER_SIZE 	1
#define OPENER_POS		0
#define FILE_LIST 		0x0
#define FILE_DOWNLOAD	0x1

#define OCTOBBLOCK_MAX		8888
#define OCTOLEG_MAX_SIZE	1111
#define LEGS_IN_TRANSIT		8
#define OCTOLEG_1			((char)0x01)
#define OCTOLEG_2			((char)0x02)
#define OCTOLEG_3			((char)0x04)
#define OCTOLEG_4			((char)0x08)
#define OCTOLEG_5			((char)0x10)
#define OCTOLEG_6			((char)0x20)
#define OCTOLEG_7			((char)0x40)
#define OCTOLEG_8			((char)0x80)

// Forward declarations, only 1 used depending on if it is the Server or the User side. 
class FileDownloader;
class FileSender;

// Processing for the block takes place either during the FileSender or FileDownloaders mutex.

//!!
// Each leg does not timeout and ask for another transmit (cause I dont want to have 16 (8 Sends, 8 Downloaders) + (#BlocksTotal * 8LegsPerBlock Threads) + UserThread + ServerThread + ManagerThread threads.)

// An ack in transmit looks as follows:
// <1 byte (char) indicating the block>A<1 byte (char) indicating the legNum> 

// An octoleg in transmit looks as follows
class Octoleg
{
	public:
		Octoleg(char blockNum, char legNum, FileDownloader downloader, int size);
		Octoleg(char blockNum, char legNum, FileSender sender, int size, char* data);
		~Octoleg();
		
		void serverSendData(); // This is called at the start, and any time the associated Octoblock gets a retransmit for this leg. 
		void serverAskForAck(); // If the server has not recieved and Ack after the timeout we send to the client a request for ack. Client either does not send anything or sends (ack or req for data).
		
		void clientRecvFileData(char* data, int size); // Fills data if size == -1;
		void clientSendAck(); // Called if the server asks for ack and we have got file. 
		
		// Called if the server sends an ack request, but we have not got leg, so we send transmit req.
		// If the client moved to the next block, but the last ack did not reach the server than either the server will ask for an ack (which client will ask for retrans), or the client will ask for retransmit (or both).
		// 		If this happens then the server will recieve a retransmit for an Octoblock with a different blockNum, so it will realize that the client moved on and will perform a serverSendData. 
		void clientAskForRetransmit();
	
	private:
		FileDownloader* downloader;
		FileSender* sender;
		char blockNum;
		char legNum;
		int size = -1;
		char* data;
};

class Octoblock
{
	public:
		Octoblock(char blockNum, int size, FileDownloader* downloader);
		Octoblock(char blockNum, int size, char* data, FileSender* sender);
		~Octoblock();
	
		void sendRequest();
		void recvClient(char* data, int size); // recvs either a request for ack, which if that leg is already got this sends, or if leg not got then we call that leg retransmit. Or we recv file data, so we store and ack
		
		void serverSendData(); // Sends all of the leg data. Called at the start of the block, or in a very special retransmit case.
		bool recvServer(char* data, int size); // The data is either an ack of recv, or asking for a retransmit. If retrans we call Octoleg.serverSendData() on the associated leg. Returns true if on same block, false if not
		
		bool complete(); // Returns true once all acks done.
		
		static std::queue<Octoblock> getOctoblocks(int fileSize, FileDownloader* downloader);
		static std::queue<Octoblock> getOctoblocks(int fileSize, char* data, FileSender* sender);
		
	private:
		char blockNum;
		int size = 0;
		Octoleg* legs[LEGS_IN_TRANSIT];
		char acksNeeded = 0xFF; // Once this reaches 0 we are good to go. 
};

#endif










