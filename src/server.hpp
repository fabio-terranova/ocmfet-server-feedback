#pragma once

#include "hw_peripherals.hpp"
#include "acquirer.hpp"

#include <string>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <thread>

#define MEM_SIZE 1024 * 1024 * 1024 // 1 GB

class Server
{
public:
	Server(int, std::string, float);
	~Server();
	void Run();
	void SendMessage(std::string);
	void SendData(char *);

private:
	const int port_;
	bool running_;
	int socket_;
	int data_socket_;
	struct sockaddr_in server_address_;
	struct sockaddr_in client_address_;
	struct sockaddr_in data_address_;

	Acquirer *acq_;

	void ReceiveCommand();
	void StartThreads();
	void StartRecording();
	void StartRecording(std::string);
	void TagRecording(std::string);
	void StopRecording();
	void PauseRecording();
	void StartAcquisition();
	void StopAcquisition();
};
