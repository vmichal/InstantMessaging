#pragma once
#include <WS2tcpip.h>
#include <memory>
#include <string>

void sendThroughNet(std::string data, SOCKET sock) {
	std::size_t sizeToSend = data.size() + 1;
	int sendResult = send(sock, reinterpret_cast<char*>(&sizeToSend), 4, 0);
	if (sendResult == SOCKET_ERROR)
		throw std::runtime_error("An error occured while sending data size. Err #" + WSAGetLastError());
	sendResult = send(sock, data.c_str(), sizeToSend, 0);

	if (sendResult == SOCKET_ERROR)
		throw std::runtime_error("An error occured while sending data. Err #" + WSAGetLastError());
}

std::unique_ptr<char[]> receiveFromNet(SOCKET sock) {
	std::size_t sizeToReceive;
	int bytesReceived = recv(sock, reinterpret_cast<char*>(&sizeToReceive), 4, 0);
	if (!bytesReceived)
		return std::unique_ptr<char[]>(nullptr);
	if (bytesReceived < 0)
		throw std::runtime_error("An error in recv() occured. Err #" + WSAGetLastError());
	std::unique_ptr<char[]> buffer = std::make_unique<char[]>(sizeToReceive);
	ZeroMemory(buffer.get(), sizeToReceive);
	bytesReceived = recv(sock, buffer.get(), sizeToReceive, 0);
	if (!bytesReceived)
		return std::unique_ptr<char[]>(nullptr);
	if (bytesReceived < 0)
		throw std::runtime_error("An error in second recv() occured. Err #" + WSAGetLastError());
	return buffer;
}
