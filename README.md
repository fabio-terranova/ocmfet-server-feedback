# OCMFET server (feedback version)

Server application for the **feedback** version of the acquisition system developed by Elbatech. The server is intended to run on a Raspberry Pi 4 with Raspbian OS.

## Index

- [OCMFET server (feedback version)](#ocmfet-server-feedback-version)
  - [Index](#index)
  - [Installation](#installation)
    - [Prerequisites](#prerequisites)
    - [Download](#download)
    - [Compile](#compile)
  - [Run](#run)

## Installation

### Prerequisites

Git is required to download the repository from GitHub. CMake is required to compile the server application.

```sh
sudo apt-get install git cmake
```

### Download

Download the repository from GitHub:

```sh
git clone https://github.com/fabio-terranova/ocmfet-server-feedback.git
```

### Compile

Compile the server application using CMake:

```sh
cd ocmfet-server-feedback
mkdir build
cd build
cmake ..
make
```

## Run

Run the server on the Raspberry Pi with default port 8888:

```sh
sudo ./build/server 8888
```
