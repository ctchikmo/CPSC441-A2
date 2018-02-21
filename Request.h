#ifndef Request_H
#define Request_H

#include <string>

enum class RequestType{DOWNLOAD, LIST};

typedef struct Request
{
	RequestType type;
	std::string ip;
	int port;
	std::string filename;
	
	Request(RequestType type, std::string ip, int port, std::string filename):
	type(type),
	ip(ip),
	port(port),
	filename(filename){}
	
} Request;

#endif