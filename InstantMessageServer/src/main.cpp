#include <iostream>
#include <WS2tcpip.h>
#include "SocketServer.h"

#pragma comment(lib, "ws2_32.lib")

//SERVER


int main(int argc, char **argv) {
	std::cout << std::left;
	std::ios::sync_with_stdio(false);	//hopefully increase performance omega lul
	{
		//Initialize winsock
		WSADATA wsData;
		if (int errCode = WSAStartup(MAKEWORD(2, 2), &wsData); errCode) {
			std::cout << "Can't initialize winsock! Exit code " << errCode << std::endl;
			return errCode;
		}
	}				  
	unsigned port;
	std::cout << "Insert port";
	std::cin >> port;
	SocketServer server;
	server.run(port);
//clean up winsock
	WSACleanup();
	return 0;
}