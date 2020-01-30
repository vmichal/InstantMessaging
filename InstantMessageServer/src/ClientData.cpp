#include "ClientData.h"

ClientData::ClientData(std::string name, std::string service, unsigned int * addr)
	:hostName_(name), hostService_(service) {
	address_.dword = *addr;
}

std::string ClientData::getIP() const {
	std::string res;
	res.reserve(16);
	for (int i = 0; i < 4; i++)
		(res += std::to_string(address_.bytes[3-i])) += '.';
	res.pop_back();
	return res;
}
