#include "Communication.h"
#include "FileSender.h"

#include <sys/socket.h>
#include <math.h>
#include <pthread.h>
#include <bitset>

Octoblock::Octoblock(char blockNum, int size, FileDownloader* downloader):
blockNum(blockNum),
size(size)
{
	int legSize = size / LEGS_IN_TRANSIT; // This will be 0 in the case of a tiny block with less than 8 bytes. Otherwise its always divisble by 8.
	if(legSize == 0)
		legSize = 1;
	
	for(int i = 0; i < LEGS_IN_TRANSIT; i++)
		legs[i] = new Octoleg(blockNum, indexToLegNum(i), downloader, legSize);
}

Octoblock::Octoblock(char blockNum, int size, char* data, FileSender* sender):
blockNum(blockNum),
size(size),
sender(sender)
{
	int legSize = size / LEGS_IN_TRANSIT; // This will be 0 in the case of a tiny block with less than 8 bytes. Otherwise its always divisble by 8.
	if(legSize == 0)
		legSize = 1;
	
	for(int i = 0; i < LEGS_IN_TRANSIT; i++)
	{
		if(i * legSize < size) // This goes to the else when there are less than 8 bytes and we are past what the file held.
		{
			legs[i] = new Octoleg(blockNum, indexToLegNum(i), sender, legSize, &data[i * legSize]);
		}
		else
		{
			char temp[1];
			temp[0] = '\0';
			
			legs[i] = new Octoleg(blockNum, indexToLegNum(i), sender, legSize, temp);
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
	
	char block = data[BLOCK_BYTE];
	char type = data[KEY_BYTE];
	int legIndex = legNumToIndex(data[LEG_BYTE]);
	
	bool rv = false;
	if(block != blockNum)
	{
		rv = legs[legIndex]->clientAskForRetransmit();
	}
	else if(type == ASK_ACK_KEY)
	{
		if(hasLegAck(data[LEG_BYTE]))
			rv = legs[legIndex]->clientSendAck();
		else
			rv = legs[legIndex]->clientAskForRetransmit();
	}
	else if(OCTLEG_KEY)
	{
		if(!hasLegAck(data[LEG_BYTE]))
		{
			acksNeeded &= ~data[LEG_BYTE];
			rv = legs[legIndex]->clientRecvFileData(data + DATA_START_BYTE, size - DATA_START_BYTE);
			rv &= legs[legIndex]->clientSendAck();	
		}
		else 
			rv = true;
	}
	
	return rv;
}

int Octoblock::recvServer(const char* data, int size)
{
	if(size < 3)
		return false;
	
	char block = data[BLOCK_BYTE];
	char type = data[KEY_BYTE];
	int legIndex = legNumToIndex(data[LEG_BYTE]);
	
	bool rv = false;
	if(block != blockNum) // Client finished, sent last ack and moved to next block. Last ack was lost, so server relizes here. If its last block this wont make it here, but client finished, so server has ungraceful end
	{
		return RECV_SERVER_DIF_BLOCK;
	}
	else if(type == ACK_KEY)
	{
		rv = true;
		acksNeeded &= ~data[LEG_BYTE];
	}
	else if(type == ASK_TRANS_KEY) // This occurs either because we actually need a retransmit for that leg, or because the final ack (letting server move blocks) was lost & client is now on different block. 
	{	
		rv = legs[legIndex]->serverSendData();
	}
	
	return rv ? RECV_SERVER_SAME_BLOCK : RECV_SERVER_ERROR;
}

bool Octoblock::serverSendData()
{
	for(int i = 0; i < LEGS_IN_TRANSIT; i++)
	{
		if(!hasLegAck(legs[i]->getLegNum()))
		{
			if(!legs[i]->serverSendData())
			{
				return false;
			}
		}
	}
	
	return true;
}

bool Octoblock::requestAcks() // Called via the timer. 
{	
	bool rv = true;
	pthread_mutex_lock(sender->getMutex());
	{
		for(int i = 0; i < LEGS_IN_TRANSIT; i++)
			if(!hasLegAck(legs[i]->getLegNum()))
				rv &= legs[i]->serverAskForAck();
		
		sender->flag_continue = true;
		pthread_cond_signal(sender->getCond());
	}
	pthread_mutex_unlock(sender->getMutex());
		
	return rv;
}

bool Octoblock::requestRetrans()
{
	bool rv = true;
	for(int i = 0; i < LEGS_IN_TRANSIT; i++)
		if(!hasLegAck(legs[i]->getLegNum()))
			rv &= legs[i]->clientAskForRetransmit();
		
	return rv;
}

bool Octoblock::complete()
{
	return acksNeeded == 0;
}

bool Octoblock::hasLegAck(char legNum)
{
	return (acksNeeded & legNum) == 0; // If we have the ack than the && will return 0
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

char Octoblock::indexToLegNum(int index)
{
	switch(index)
	{
		case 0:
			return OCTOLEG_1;
			
		case 1:
			return OCTOLEG_2;
		
		case 2:
			return OCTOLEG_3;
		
		case 3:
			return OCTOLEG_4;
		
		case 4:
			return OCTOLEG_5;
		
		case 5:
			return OCTOLEG_6;
		
		case 6:
			return OCTOLEG_7;
		
		case 7:
			return OCTOLEG_8;
	}
	
	return -1;
}

int Octoblock::getSize()
{
	return size;
}

void Octoblock::getData(char* store)
{
	int pos = 0;
	for(int i = 0; i < LEGS_IN_TRANSIT; i++)
	{
		for(int j = 0; j < legs[i]->getDataSize() && ((pos + j) < size); j++)
		{
			store[pos + j] = legs[i]->getData()[j];
		}
		
		pos += legs[i]->getDataSize();
		if(pos >= size)
			break; // Can happen if there is a tiny block.
	}	
}

void Octoblock::getOctoblocks(std::vector<Octoblock*>* store, int fileSize, FileDownloader* downloader)
{
	int blocks = ceil((double)fileSize / (double)OCTOBBLOCK_MAX);
	int blkNum = 0;
	for(; blkNum < blocks - 1; blkNum++) // The last block may be split into a tiny block, so -1
	{
		store->push_back(new Octoblock(blkNum, std::min(fileSize, OCTOBBLOCK_MAX), downloader));
		fileSize -= OCTOBBLOCK_MAX;
	}
	
	// We have one block left here. 
	if(fileSize % LEGS_IN_TRANSIT == 0) // We can have 8 equal legs, so no tiny block.
	{
		store->push_back(new Octoblock(blkNum, fileSize, downloader));
	}
	else
	{
		double legPotentialSize = (double)fileSize / (double)LEGS_IN_TRANSIT;
		int normalBlockLegSize = (int)legPotentialSize;
		int tinyBlockLegSize = fileSize % LEGS_IN_TRANSIT;
		
		store->push_back(new Octoblock(blkNum, normalBlockLegSize * LEGS_IN_TRANSIT, downloader));
		blkNum++;
		
		store->push_back(new Octoblock(blkNum, tinyBlockLegSize * LEGS_IN_TRANSIT, downloader));
	}
}

void Octoblock::getOctoblocks(std::queue<Octoblock*>* store, int fileSize, char* data, FileSender* sender)
{
	int blocks = ceil((double)fileSize / (double)OCTOBBLOCK_MAX);
	int blkNum = 0;
	for(; blkNum < blocks - 1; blkNum++) // The last block may be split into a tiny block, so -1
	{
		store->push(new Octoblock(blkNum, std::min(fileSize, OCTOBBLOCK_MAX), &data[blkNum * OCTOBBLOCK_MAX], sender));
		fileSize -= OCTOBBLOCK_MAX;
	}
	
	// We have one block left here. 
	if(fileSize % LEGS_IN_TRANSIT == 0) // We can have 8 equal legs, so no tiny block.
	{
		store->push(new Octoblock(blkNum, fileSize, &data[blkNum * OCTOBBLOCK_MAX], sender));
	}
	else
	{
		double legPotentialSize = (double)fileSize / (double)LEGS_IN_TRANSIT;
		int normalBlockLegSize = (int)legPotentialSize;
		int tinyBlockLegSize = fileSize % LEGS_IN_TRANSIT;
		
		store->push(new Octoblock(blkNum, normalBlockLegSize * LEGS_IN_TRANSIT, &data[blkNum * OCTOBBLOCK_MAX], sender));
		blkNum++;
		
		store->push(new Octoblock(blkNum, tinyBlockLegSize * LEGS_IN_TRANSIT, &data[((blkNum - 1) * OCTOBBLOCK_MAX) + (normalBlockLegSize * LEGS_IN_TRANSIT)], sender));
	}
}







