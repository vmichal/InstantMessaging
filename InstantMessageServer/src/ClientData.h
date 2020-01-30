#pragma once
#ifndef CLIENT_DATA_H
#define CLIENT_DATA_H
#include <string>


struct ClientData {
	std::string hostName_, hostService_;
	union {
		unsigned char bytes[4];
		unsigned short words[2];
		unsigned int dword;
	} address_;

	ClientData() = default;

	ClientData(std::string name, std::string service, unsigned int * address);

	std::string getIP() const;
};


#endif //^^CLIENT_DATA_H