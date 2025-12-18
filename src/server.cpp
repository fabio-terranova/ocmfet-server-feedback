#include "server.hpp"

#include <arpa/inet.h>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <string_view>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define coutr std::cout << "-> "

void Server::SendMessage(std::string_view message) {
  socklen_t client_address_length = sizeof(client_address_);
  sendto(socket_, std::string(message).c_str(), message.length(), 0,
         (struct sockaddr*)&client_address_, client_address_length);
}

void Server::SendData(char* data) {
  socklen_t client_address_length = sizeof(data_address_);
  sendto(data_socket_, data, BUF_LEN, 0, (struct sockaddr*)&data_address_,
         client_address_length);

  // Print the first 10 bytes of the data
  // for (int i = 0; i < 10; i++) {
  //   std::cout << std::hex << std::setw(2) << std::setfill('0') <<
  //   (int)data[i]
  //             << " ";
  // }
}

Server::Server(uint16_t port, std::string_view data_folder, float T2)
    : port_(port), running_(true) {
  // Set up the server address
  server_address_.sin_family      = AF_INET;
  server_address_.sin_addr.s_addr = INADDR_ANY;
  server_address_.sin_port        = htons(port_);

  // Create a UDP socket for receiving commands and sending messages
  socket_ = socket(AF_INET, SOCK_DGRAM, 0);
  if (socket_ == -1) {
    std::cerr << "Error creating socket" << '\n';
    return;
  }

  if (bind(socket_, (struct sockaddr*)&server_address_,
           sizeof(server_address_)) == -1) {
    std::cerr << "Error binding to port " << port_ << '\n';
    return;
  }

  // Create a UDP socket for sending data
  data_socket_ = socket(AF_INET, SOCK_DGRAM, 0);
  if (data_socket_ == -1) {
    std::cerr << "Error creating data socket" << '\n';
    return;
  }

  acq_ = new Acquirer(data_folder, T2);
  acq_->StartThreads(this);
}

void Server::Run() {
  std::cout << "Server listening on port " << port_ << '\n';

  while (running_) {
    ReceiveCommand();
  }
}

Server::~Server() {
  // Close the sockets
  close(socket_);
  close(data_socket_);
  std::cout << "Sockets closed." << '\n';
  std::cout << "Server stopped." << '\n' << '\n';
}

void Server::ReceiveCommand() {
  char      buffer[1024];
  socklen_t client_address_length = sizeof(client_address_);

  const ssize_t bytes_received =
      recvfrom(socket_, buffer, sizeof(buffer), 0,
               (struct sockaddr*)&client_address_, &client_address_length);

  if (bytes_received <= 0) {
    return;
  }

  std::string_view command(buffer, static_cast<size_t>(bytes_received));

  data_address_.sin_family = AF_INET;
  data_address_.sin_addr.s_addr =
      client_address_.sin_addr.s_addr;       // Use the client's IP address
  data_address_.sin_port = htons(port_ + 1); // Use the client's port + 1

  if (command == "start") {
    if (!acq_->acquiring_) {
      coutr << "Received start command. Starting the acquisition..." << '\n';
      acq_->Start();
      SendMessage("Started the acquisition!");
    } else {
      coutr << "Received start command, but the acquisition is already running."
            << '\n';
      SendMessage("The acquisition is already running.");
    }
  } else if (command.substr(0, 3) == "rec") {
    std::string_view filename = command.substr(4);

    if (!acq_->recording_) {
      coutr << "Received rec command. Starting recording..." << '\n';
      acq_->StartRecording(filename);
      SendMessage("Started recording!");
    } else {
      coutr << "Received rec command, but the recording is already running."
            << '\n';
      SendMessage("The server is already recording.");
    }
  } else if (command == "pause") {
    if (acq_->recording_ && !acq_->paused_) {
      coutr << "Received pause command. Pausing recording..." << '\n';
      acq_->PauseRecording();
      SendMessage("Paused recording!");
    } else {
      coutr << "Received pause command, but there is nothing to pause." << '\n';
      SendMessage("The server is not recording.");
    }
  } else if (command == "resume") {
    if (acq_->recording_ && acq_->paused_) {
      coutr << "Received resume command. Resuming recording..." << '\n';
      acq_->ResumeRecording();
      SendMessage("Resumed recording!");
    } else {
      coutr << "Received resume command, but there is nothing to resume."
            << '\n';
      SendMessage("The server is already recording.");
    }
  } else if (command.substr(0, 3) == "tag") {
    if (acq_->recording_ && !acq_->paused_) {
      std::string_view tag = command.substr(4);
      coutr << "Received tag: " << tag << '\n';

      acq_->TagRecording(std::string(tag));

      SendMessage("Tagged recording! (" + std::string(tag) + ")");
    } else {
      coutr << "Received tag command, but there is no recording." << '\n';
      SendMessage("The server is not recording.");
    }
  } else if (command == "stop") {
    if (acq_->acquiring_) {
      coutr << "Received stop command. Stopping the acquisition..." << '\n';
      std::vector<std::string> files = acq_->Stop();
      SendMessage("Stopped the acquisition!");
      if (files.size() > 0) {
        SendMessage("Stopped the recording!");
        std::cout << "Recording saved to " << files[0] << '\n';
        SendMessage("Recording saved to " + files[0]);
        std::cout << "Tags saved to " << files[1] << '\n';
        SendMessage("Tags saved to " + files[1]);
      }
    } else {
      coutr << "Received stop command, but the acquisition is already stopped."
            << '\n';
      SendMessage("The acquisition is already stopped.");
    }
  } else if (command.substr(0, 3) == "sT2") {
    std::string value = std::string(command.substr(4));

    coutr << "Received sT2 command with value " << value << '\n';

    if (acq_->SetT2(stof(value)) == -1)
      SendMessage("Error setting T2.");
    else
      SendMessage("T2 set to " + value + " \u03BCs!");
  }
  // else if (command == "reset")
  // {
  // 	coutr << "Received reset command. Resetting the dsPIC..." << endl;
  // 	SendMessage("Resetting the dsPIC...");

  // 	ResetDSPIC();
  // }
  else if (command == "kill") {
    coutr << "Received kill command. Exiting..." << '\n';
    SendMessage("Received kill command. Exiting...");

    acq_->Stop();
    acq_->StopThreads();
    running_ = false;
  } else if (command.substr(0, 4) == "vg01") {
    std::string value = std::string(command.substr(5));
    coutr << "Received vg1 command with value " << value << '\n';

    if (acq_->SetVG(stod(value), 1) == -1)
      SendMessage("Error setting VG1.");
    else
      SendMessage("VG1 set to " + value + " V!");
  } else if (command.substr(0, 4) == "vg02") {
    std::string value = std::string(command.substr(5));
    coutr << "Received vg2 command with value " << value << '\n';

    if (acq_->SetVG(stod(value), 2) == -1)
      SendMessage("Error setting VG2.");
    else
      SendMessage("VG2 set to " + value + " V!");
  } else if (command.substr(0, 4) == "id01") {
    std::string value = std::string(command.substr(5));
    coutr << "Received i1 command with value " << value << '\n';

    if (acq_->SetVsetpoint(stod(value), 1) == -1)
      SendMessage("Error setting Isetpoint1.");
    else
      SendMessage("I1 set to " + value + " \u03BCA!");
  } else if (command.substr(0, 4) == "id02") {
    std::string value = std::string(command.substr(5));
    coutr << "Received i2 command with value " << value << '\n';

    if (acq_->SetVsetpoint(stod(value), 2) == -1)
      SendMessage("Error setting Isetpoint2.");
    else
      SendMessage("I2 set to " + value + " \u03BCA!");
  } else {
    coutr << "Received unknown command: " << std::string(command) << '\n';
    SendMessage(std::string("Unknown command: ") + std::string(command));
  }
}
