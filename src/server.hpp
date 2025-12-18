#pragma once

#include "acquirer.hpp"

#include <arpa/inet.h>
#include <cstdint>
#include <string>
#include <string_view>
#include <sys/socket.h>

#define MEM_SIZE 1024 * 1024 * 1024 // 1 GB

class Server {
public:
  Server(uint16_t, std::string_view, float);
  ~Server();
  void Run();
  void SendMessage(std::string_view);
  void SendData(char*);

private:
  const uint16_t     port_;
  bool               running_;
  int                socket_;
  int                data_socket_;
  struct sockaddr_in server_address_;
  struct sockaddr_in client_address_;
  struct sockaddr_in data_address_;

  Acquirer* acq_;

  void ReceiveCommand();
  void StartThreads();
  void StartRecording();
  void StartRecording(std::string_view);
  void TagRecording(std::string);
  void StopRecording();
  void PauseRecording();
  void StartAcquisition();
  void StopAcquisition();
};
