#pragma once

#include "hw_peripherals.hpp"
#include "acquirer.hpp"

#include <string>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <thread>

#define MEM_SIZE 1024 * 1024 * 1024 // 1 GB

using namespace std;

class Server
{
public:
	Server(int, string, float);
	~Server();
	void Run();
	void SendMessage(string);
	void SendData(char *);

private:
	int port_;
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
	void StartRecording(string);
	void TagRecording(string);
	void StopRecording();
	void PauseRecording();
	void StartAcquisition();
	void StopAcquisition();
};
