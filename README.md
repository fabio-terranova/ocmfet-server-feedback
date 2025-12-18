# OCMFET server (feedback version)

Server application for the **feedback** version of the acquisition system developed by Elbatech. The server is intended to run on a Raspberry Pi 4 with Raspbian OS.

## Index

- [OCMFET server (feedback version)](#ocmfet-server-feedback-version)
  - [Index](#index)
  - [Installation](#installation)
    - [Prerequisites](#prerequisites)
    - [Download](#download)
    - [Compile](#compile)
      - [Using CMake](#using-cmake)
      - [Using g++ directly](#using-g-directly)
  - [Run](#run)

## Installation

### Prerequisites

Git is required to download the repository from GitHub. CMake can be used to compile the server application. Install them with the following command:

```sh
sudo apt-get install git cmake
```

### Download

Download the repository from GitHub:

```sh
git clone https://github.com/fabio-terranova/ocmfet-server-feedback.git
```

### Compile

Compile the server application using CMake or g++ directly.

#### Using CMake

```sh
cd ocmfet-server-feedback
mkdir build
cmake -S . -B build
cmake --build build/
```

#### Using g++ directly

```sh
cd ocmfet-server-feedback
mkdir build
g++ -std=c++20 -Wall -o build/server src/main.cpp src/hw_peripherals.cpp src/acquirer.cpp src/server.cpp -lbcm2835 -lpthread -lrt
```

## Run

Run the server on the Raspberry Pi with port 8888:

```sh
sudo ./build/server 8888
```

Equivalently with default port 8888:

```sh
sudo ./run.sh
```
