#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include <iostream>
#include <stdlib.h>
#include <queue>
#include <vector>

#define SERVER_BUFFER	1500

#define OPENER_SIZE 	10
#define OPENER_POS		0
#define OPENER_RECVPORT 1
#define FILE_LIST 		0x0
#define FILE_DOWNLOAD	0x1
#define FILE_SIZE_BUFF	8

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

#define RECV_SERVER_ERROR		-1
#define RECV_SERVER_SAME_BLOCK	 0
#define RECV_SERVER_DIF_BLOCK	 1

#define BLOCK_BYTE			0
#define KEY_BYTE			1
#define LEG_BYTE			2
#define DATA_START_BYTE		3

#define ACK_KEY				'A'
#define ASK_ACK_KEY			'Q'
#define ASK_TRANS_KEY		'T'
#define OCTLEG_KEY			'O'

// Forward declarations, only 1 used depending on if it is the Server or the User side. 
class FileDownloader;
class FileSender;

// Processing for the block takes place either during the FileSender or FileDownloaders mutex.

//!!
// Each leg does not timeout and ask for another transmit (cause I dont want to have 16 (8 Sends, 8 Downloaders) + (#BlocksTotal * 8LegsPerBlock Threads) + UserThread + ServerThread + ManagerThread threads.)

// An ack in transmit looks as follows:
// <1 byte (char) indicating the block>A<1 byte (char) indicating the legNum> 

// An ask for ack in transmit looks as follows:
// <1 byte (char) indicating the block>Q<1 byte (char) indicating the legNum> 

// An ask for retransmit in transmit looks as follows:
// <1 byte (char) indicating the block>T<1 byte (char) indicating the legNum> 

// An octoleg in transmit looks as follows
// <1 byte (char) indicating the block>O<1 byte (char) indicating the legNum><... bytes for the file according to this legs size.>

class Octoleg
{
	public:
		Octoleg(char blockNum, char legNum, FileDownloader* downloader, int size);
		Octoleg(char blockNum, char legNum, FileSender* sender, int size, char* d);
		~Octoleg();
		
		bool serverSendData(); // This is called at the start, and any time the associated Octoblock gets a retransmit for this leg. 
		bool serverAskForAck(); // If the server has not recieved and Ack after the timeout we send to the client a request for ack. Client either does not send anything or sends (ack or req for data).
		
		bool clientRecvFileData(char* d, int s);
		bool clientSendAck(); // Called if the server asks for ack and we have got file. Also called upon recieving the file. 
		
		// Called if the server sends an ack request, but we have not got leg, so we send transmit req. Also called if server recvs a retransmit.
		bool clientAskForRetransmit();
		
		int getDataSize();
		char* getData();
		
		char getLegNum();
	
	private:
		char blockNum;
		char legNum;
		FileDownloader* downloader;
		FileSender* sender;
		int size = -1;
		char* data = NULL;
		char* serverData = NULL; // Same as data, just has the heading info.
};

class Octoblock
{
	public:
		Octoblock(char blockNum, int size, FileDownloader* downloader);
		Octoblock(char blockNum, int size, char* data, FileSender* sender);
		~Octoblock();
	
		bool recvClient(char* data, int size); // recvs either a request for ack, which if that leg is already got this sends, or if leg not got then we call that leg retransmit. Or we recv file data, so we store and ack
		
		// If the client moved to the next block, but the last ack did not reach the server than either the server will ask for an ack (which client will ask for retrans), or the client will ask for retransmit (or both).
		// If this happens then the server will recieve a retransmit for an Octoblock with a different blockNum, so it will realize that the client moved on and will perform a serverSendData. 
		int recvServer(const char* data, int size); // The data is either an ack of recv, or asking for a retransmit. If retrans we call Octoleg.serverSendData() on the associated leg. See defines for return value. 
		bool serverSendData(); // Sends all of the leg data. Called at the start of the block, or in a very special retransmit case.
		
		// These are called by the timout manz.
		bool requestAcks();
		bool requestRetrans();
		
		bool complete(); // Returns true once all acks done.
		int getSize();
		void getData(char* store); // Gets all the leg data into one array
		
		static void getOctoblocks(std::vector<Octoblock*>* store, int fileSize, FileDownloader* downloader); // Downloader we go until we hit size, then we loop through again to getData.
		static void getOctoblocks(std::queue<Octoblock*>* store, int fileSize, char* data, FileSender* sender); // Sender we keep going till empty
		
	private:
		char blockNum;
		int size = 0;
		Octoleg* legs[LEGS_IN_TRANSIT];
		char acksNeeded = 0xFF; // Once this reaches 0 we are good to go. 
		
		bool hasLegAck(char legNum);
		int legNumToIndex(char legNum);
		char indexToLegNum(int index);
};

#endif










