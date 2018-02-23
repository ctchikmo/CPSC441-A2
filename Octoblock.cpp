#include "Communication.h"

#include <sys/socket.h>
#include <math.h>

Octoblock::Octoblock(char blockNum, int size, FileDownloader* downloader):
blockNum(blockNum),
size(size)
{
	int legSize = size / LEGS_IN_TRANSIT; // This will be 0 in the case of a tiny block with less than 8 bytes. Otherwise its always divisble by 8.
	if(legSize == 0)
		legSize = 1;
	
	for(int i = 0; i < LEGS_IN_TRANSIT; i++)
		legs[i] = new Octoleg(blockNum, i, downloader, legSize);
}

Octoblock::Octoblock(char blockNum, int size, char* data, FileSender* sender):
blockNum(blockNum),
size(size)
{
	int legSize = size / LEGS_IN_TRANSIT; // This will be 0 in the case of a tiny block with less than 8 bytes. Otherwise its always divisble by 8.
	if(legSize == 0)
		legSize = 1;
	
	for(int i = 0; i < LEGS_IN_TRANSIT; i++)
	{
		if(i * legSize < size) // This goes to the else when there are less than 8 bytes and we are past what the file held.
		{
			legs[i] = new Octoleg(blockNum, i, sender, legSize, &data[i * legSize]);
		}
		else
		{
			char temp[1];
			temp[0] = '\0';
			
			legs[i] = new Octoleg(blockNum, i, sender, legSize, temp);
		}
	}
}

Octoblock::~Octoblock()
{
	for(int i = 0; i < LEGS_IN_TRANSIT; i++)
		delete legs[i];
}

bool Octoblock::recvClient(char* data, int size)
{
	if(size < 3)
		return false;
	
	char type = data[KEY_BYTE];
	int legIndex = legNumToIndex(data[LEG_BYTE]);
	
	bool rv = false;
	if(type == ASK_ACK_KEY)
	{
		if(hasLegAck(data[LEG_BYTE]))
			rv = legs[legIndex]->clientSendAck();
		else
			rv = legs[legIndex]->clientAskForRetransmit();
	}
	else if(OCTLEG_KEY)
	{
		acksNeeded &= ~data[LEG_BYTE];
		rv = legs[legIndex]->clientRecvFileData(data + DATA_START_BYTE, size - DATA_START_BYTE);
		rv &= legs[legIndex]->clientSendAck();
	}
	
	return rv;
}

int Octoblock::recvServer(char* data, int size)
{
	if(size < 3)
		return false;
	
	char block = data[BLOCK_BYTE];
	char type = data[KEY_BYTE];
	int legIndex = legNumToIndex(data[LEG_BYTE]);
	
	bool rv = false;
	if(type == ACK_KEY)
	{
		rv = acksNeeded &= ~data[LEG_BYTE];
	}
	else if(type == ASK_TRANS_KEY) // This occurs either because we actually need a retransmit for that leg, or because the final ack (letting server move blocks) was lost & client is now on different block. 
	{
		if(block != blockNum) // Client finished, sent last ack and moved to next block. Last ack was lost, so server relizes here. If its last block this wont make it here, but client finished, so server has ungraceful end
			return RECV_SERVER_DIF_BLOCK;
			
		rv = legs[legIndex]->serverSendData();
	}
	
	return rv ? RECV_SERVER_SAME_BLOCK : RECV_SERVER_ERROR;
}

bool Octoblock::serverSendData()
{
	for(int i = 0; i < LEGS_IN_TRANSIT; i++)
		if(!legs[i]->serverSendData())
			return false;
	
	return true;
}

bool Octoblock::complete()
{
	return acksNeeded == 0;
}

bool Octoblock::hasLegAck(char legNum)
{
	return (acksNeeded && legNum) == 0; // If we have the ack than the && will return 0
}

int Octoblock::legNumToIndex(char legNum)
{
	if(legNum == OCTOLEG_1)
		return 0;
	else if(legNum == OCTOLEG_2)
		return 1;
	else if(legNum == OCTOLEG_3)
		return 2;
	else if(legNum == OCTOLEG_4)
		return 3;
	else if(legNum == OCTOLEG_5)
		return 4;
	else if(legNum == OCTOLEG_6)
		return 5;
	else if(legNum == OCTOLEG_7)
		return 6;
	else if(legNum == OCTOLEG_8)
		return 7;
	
	return -1;
}

std::vector<Octoblock> Octoblock::getOctoblocks(int fileSize, FileDownloader* downloader)
{
	std::vector<Octoblock> rv;
	
	int blocks = ceil((double)fileSize / (double)OCTOBBLOCK_MAX);
	int blkNum = 0;
	for(; blkNum < blocks - 1; blkNum++) // The last block may be split into a tiny block, so -1
	{
		rv.push_back(Octoblock(blkNum, std::min(fileSize, OCTOBBLOCK_MAX), downloader));
		fileSize -= OCTOBBLOCK_MAX;
	}
	
	// We have one block left here. 
	if(fileSize % LEGS_IN_TRANSIT == 0) // We can have 8 equal legs, so no tiny block.
	{
		rv.push_back(Octoblock(blkNum, fileSize, downloader));
	}
	else
	{
		double legPotentialSize = (double)fileSize / (double)LEGS_IN_TRANSIT;
		int normalBlockLegSize = (int)legPotentialSize;
		int tinyBlockLegSize = fileSize % LEGS_IN_TRANSIT;
		
		rv.push_back(Octoblock(blkNum, normalBlockLegSize * LEGS_IN_TRANSIT, downloader));
		blkNum++;
		
		rv.push_back(Octoblock(blkNum, tinyBlockLegSize * LEGS_IN_TRANSIT, downloader));
	}
	
	return rv;
}

std::queue<Octoblock> Octoblock::getOctoblocks(int fileSize, char* data, FileSender* sender)
{
	std::queue<Octoblock> rv;
	
	int blocks = ceil((double)fileSize / (double)OCTOBBLOCK_MAX);
	int blkNum = 0;
	for(; blkNum < blocks - 1; blkNum++) // The last block may be split into a tiny block, so -1
	{
		rv.push(Octoblock(blkNum, std::min(fileSize, OCTOBBLOCK_MAX), &data[blkNum * OCTOBBLOCK_MAX], sender));
		fileSize -= OCTOBBLOCK_MAX;
	}
	
	// We have one block left here. 
	if(fileSize % LEGS_IN_TRANSIT == 0) // We can have 8 equal legs, so no tiny block.
	{
		rv.push(Octoblock(blkNum, fileSize, &data[blkNum * OCTOBBLOCK_MAX], sender));
	}
	else
	{
		double legPotentialSize = (double)fileSize / (double)LEGS_IN_TRANSIT;
		int normalBlockLegSize = (int)legPotentialSize;
		int tinyBlockLegSize = fileSize % LEGS_IN_TRANSIT;
		
		rv.push(Octoblock(blkNum, normalBlockLegSize * LEGS_IN_TRANSIT, &data[blkNum * OCTOBBLOCK_MAX], sender));
		blkNum++;
		
		rv.push(Octoblock(blkNum, tinyBlockLegSize * LEGS_IN_TRANSIT, &data[((blkNum - 1) * OCTOBBLOCK_MAX) + (normalBlockLegSize * LEGS_IN_TRANSIT)], sender));
	}
	
	return rv;
}







