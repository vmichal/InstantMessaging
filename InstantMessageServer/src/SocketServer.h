#pragma once
#ifndef SOCKET_MANAGER_H
#define SOCKET_MANAGER_H
#include "ClientData.h"

#include <ws2tcpip.h>
#include <vector>
#include <map>
#include <string>
#include <thread>
#include <mutex>
	  
class SocketServer {
private:
	SOCKET listening_;
	fd_set connectedClients_;
	std::map<SOCKET, ClientData> clientData_;
	bool running_ = true, showMessages_ = true;
	std::thread listeningThread_;

	std::vector<SOCKET> getActiveSockets() const;

	void acceptNewClient();

	void removeClient(SOCKET s);

	void receiveFromClient(SOCKET clientSocket);

	void processNetworkRequests();

	void threadLoop();

	
	void quit();

	void processCommands(std::string_view command);
	void listClients() const;
public:
	SocketServer() = default;

	void run(unsigned short port);


};


#endif // ^^SOCKET_MANAGER_H