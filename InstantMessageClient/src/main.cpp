#include <iostream>
#include <thread>
#include <chrono>
#include <string>
#include <WS2tcpip.h>
#include <string>
#include <thread>
#include "Protocol.h"

//Client

bool running = true;
SOCKET sock;

void listeningThread() {
	fd_set descriptors = {};
	FD_SET(sock, &descriptors);
	timeval timeout;
	timeout.tv_usec = 10;
	while (running) {
		fd_set copy = descriptors;
		select(0, &copy, nullptr, nullptr, &timeout);
		if (copy.fd_count) {
			auto buffer = receiveFromNet(copy.fd_array[0]);
			if (!buffer.get()) {
				std::cout << "Server has been shut down\n";
				running = false;
				return;
			}
			std::cout << buffer.get() << std::endl;
		}
		else
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}

#pragma comment(lib, "ws2_32.lib")

std::string serverIP;
unsigned short serverPort = 54'000;

int main(int argc, char ** argv) {

	//Initialize WinSock
	WSADATA wsData;
	WORD version = MAKEWORD(2, 2);
	if (int errCode = WSAStartup(version, &wsData)) {
		std::cout << "Can't start Winsock, err #" << errCode << std::endl;
		return errCode;
	}
	//Create socket
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET) {
		std::cout << "Can't create socket, err #" << WSAGetLastError() << std::endl;
		WSACleanup();
		return sock;
	}
	std::cout << "Server IP:";
	std::getline(std::cin, serverIP);
	if (serverIP.empty())
		serverIP = "127.0.0.1";
	std::string buffer;
	std::cout << "Port:";
	std::getline(std::cin, buffer);
	serverPort = std::stoi(buffer);
	if (!serverPort)
		serverPort = 54000;


	//Fill in a hint structure
	sockaddr_in hint;
	hint.sin_family = AF_INET;
	hint.sin_port = htons(serverPort);
	inet_pton(AF_INET, serverIP.c_str(), &hint.sin_addr);

	//Connect to server
	int connResult = connect(sock, (sockaddr*)&hint, sizeof(hint));
	if (connResult == SOCKET_ERROR) {
		std::cout << "Can't connect to server, bitch. Err # " << WSAGetLastError() << std::endl;
		closesocket(sock);
		WSACleanup();
		return connResult;
	}

	// Do-while loop to send and receive data
	std::string input;
	std::thread listeningThread(&listeningThread);
	while (true) {
		//Prompt the user for some text lul
		std::cout << "> ";
		std::getline(std::cin, input);
		if (!input.compare("exit") || !running)
			break;

		if (input.size())
			sendThroughNet(input, sock);

		/*
		auto buffer = receiveFromNet(sock);
		if (!buffer.get()) {
			std::cout << "Server has been shut down.\n";
			break;
		}
		std::string message = buffer.get();
		std::cout << "SERVER> " << message<<std::endl;		*/
	}
	running = false;
	listeningThread.join();
	closesocket(sock);
	WSACleanup();
	return 0;
}