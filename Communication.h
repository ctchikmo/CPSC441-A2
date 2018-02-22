#ifndef COMMUNICATION_H
#define COMMUNICATION_H

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

class Octoleg
{
	
};

class Octoblock
{
	
};

#endif