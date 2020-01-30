#include "SocketServer.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include "Protocol.h"

void SocketServer::threadLoop() {
	while (running_) {
		processNetworkRequests();
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}

void SocketServer::quit() {
	running_ = false;
	std::cout << "Closing server\n";
	if (listeningThread_.joinable())
		listeningThread_.join();
	for (unsigned int i = 0; i < connectedClients_.fd_count; ++i) {
		SOCKET sock = connectedClients_.fd_array[i];
		auto & data = clientData_[sock];
		std::cout << "Closing connection with " << data.hostName_ << " [" << data.getIP() << "], service " << data.hostService_ << std::endl;
		clientData_.erase(sock);
		closesocket(sock);
	}

	std::cout << "All connections closed\n";
}

void SocketServer::run(unsigned short port) {
	//create a socket
	listening_ = socket(AF_INET, SOCK_STREAM, 0);
	if (listening_ == INVALID_SOCKET)
		throw std::runtime_error("Error creating socket.\n");
	FD_SET(listening_, &connectedClients_);
	unsigned localAddr = 127 << 24 | 1;
	clientData_[listening_] = ClientData("localhost listening socket", "127.0.0.1", &localAddr);
	std::cout << "Created socket " << listening_ << std::endl;

	//Bind ip and port to a socket
	sockaddr_in hint;
	hint.sin_family = AF_INET;
	hint.sin_port = htons(port);
	hint.sin_addr.S_un.S_addr = INADDR_ANY;

	bind(listening_, (sockaddr*)(&hint), sizeof(hint));
	std::cout << "Socket bound\n";

	//Start listening
	listen(listening_, SOMAXCONN);
	std::cout << "Server listening @ port " << port << '\n';
	listeningThread_ = std::thread(&SocketServer::threadLoop, this);

	std::string input;
	while (running_) {
		std::getline(std::cin, input);
		if (input.size())
			processCommands(input);
	}
}

void SocketServer::processCommands(std::string_view command) {
	if (!command.compare("exit"))
		return quit();
	if (!command.compare("list"))
		return listClients();
	if (!command.compare("show")) {
		showMessages_ = !showMessages_;
		std::cout << "Showing messages " << (showMessages_ ? "on\n" : "off\n");
		return;
	}
	std::cout << "Unknown command " << command.data() << std::endl;
}

void SocketServer::listClients() const {
	fd_set copy = connectedClients_;
	std::cout << "Connected clients:\n";
	while (copy.fd_count--) {
		SOCKET sock = copy.fd_array[copy.fd_count];
		auto& data = clientData_.at(sock);
		std::cout << data.hostName_ << " [" << data.getIP() << "] @ " << data.hostService_ << '\n';
	}
	std::cout.flush();
}

void SocketServer::receiveFromClient(SOCKET clientSocket) {
	auto buffer = receiveFromNet(clientSocket);
	if (!buffer.get()) {
		removeClient(clientSocket);
		return;
	}
	static unsigned remainingLogsBeforeHeader = 1;
	if (!--remainingLogsBeforeHeader) {
		std::cout << std::right << std::setw(6) << "socket" << std::setw(18) << "IP adress" << std::setw(8) << "Port"
			<< std::setw(10) << "Message size [Bytes]" << std::left << "Host name\n";
		remainingLogsBeforeHeader = 20;
	}

	auto &data = clientData_[clientSocket];
	std::string message = buffer.get();
	std::cout << std::setw(6) << clientSocket << std::setw(18) << data.getIP() << std::setw(8) << data.hostService_
		<< std::right << std::setw(10) << message.size() << std::left << "B" << data.hostName_ << '\n';
	if (showMessages_)
		std::cout << message << '\n';
	std::string messageToForward;
	{
		std::ostringstream stream;
		stream << data.hostName_ << " [" << data.getIP() << "]> " << message;
		messageToForward = stream.str();
	}
	for (unsigned i = 0; i < connectedClients_.fd_count; i++)
		if (SOCKET s = connectedClients_.fd_array[i]; s != listening_ && s != clientSocket)
			sendThroughNet(messageToForward, s);
}

void SocketServer::processNetworkRequests() {
	std::vector<SOCKET> pending = getActiveSockets();
	for (auto sock : pending)
		if (sock == listening_)
			acceptNewClient();
		else
			receiveFromClient(sock);
}

void SocketServer::acceptNewClient() {
	sockaddr_in client;
	int clientSize = sizeof(client);
	SOCKET clientSocket = accept(listening_, (sockaddr*)&client, &clientSize);
	if (clientSocket == INVALID_SOCKET)
		throw std::runtime_error("Error estabilishing connection");

	char host[NI_MAXHOST];	   //client's remote name
	char service[NI_MAXSERV];  //Service (port) the client is connected on
	char ipBuffer[32];
	ZeroMemory(host, NI_MAXHOST);		//effectively memset(host, 0, NI_MAXHOST)
	ZeroMemory(service, NI_MAXSERV);
	ZeroMemory(ipBuffer, 32);

	if (!getnameinfo((sockaddr*)&client, sizeof(client), host, NI_MAXHOST, service, NI_MAXSERV, 0))
		std::cout << "Client " << host << " [" << inet_ntop(AF_INET, &client.sin_addr, ipBuffer, 32) << "] connected on port " << service << std::endl;
	else {
		inet_ntop(AF_INET, &client.sin_addr, host, NI_MAXHOST);
		std::cout << "DNS lookup failed, host " << host << " connected on port " << ntohs(client.sin_port) << std::endl;
	}
	std::string newClientNotification = (std::string("SERVER: new client ") += host) += " has joined the room.";
	for (unsigned i = 0; i < connectedClients_.fd_count; i++)
		if (SOCKET s = connectedClients_.fd_array[i]; s != listening_)
			sendThroughNet(newClientNotification, s);
	clientData_[clientSocket] = ClientData(host, service, (unsigned int*)&client.sin_addr);
	FD_SET(clientSocket, &connectedClients_);
}

void SocketServer::removeClient(SOCKET s) {
	auto &data = clientData_[s];
	std::cout << "Client " << data.hostName_ << " [" << data.getIP() << "] has disconnected from service " << data.hostService_ << std::endl;
	std::string clientDisconnectNotification = (std::string("SERVER: client ") += data.hostName_) += " has disconnected.";
	clientData_.erase(s);
	FD_CLR(s, &connectedClients_);
	for (unsigned i = 0; i < connectedClients_.fd_count; i++)
		if (SOCKET s = connectedClients_.fd_array[i]; s != listening_)
			sendThroughNet(clientDisconnectNotification, s);
	closesocket(s);
}

std::vector<SOCKET> SocketServer::getActiveSockets() const {
	fd_set tmp = connectedClients_;
	timeval timeout;
	timeout.tv_sec = 1;
	int socketCount = select(0, &tmp, nullptr, nullptr, &timeout);
	if (socketCount == SOCKET_ERROR)
		throw std::runtime_error("Err #" + WSAGetLastError());
	std::vector<SOCKET> sockets(socketCount);
	for (unsigned int i = 0; i < sockets.size(); ++i)
		sockets[i] = tmp.fd_array[i];
	return sockets;
}