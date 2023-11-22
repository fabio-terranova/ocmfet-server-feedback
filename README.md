# OCMFET server (feedback version)

Server application for the feedback version of the acquisition system developed by Elbatech.

## Index

- [Installation](#installation)
  - [Compile](#compile)
  - [Run](#run)

## Installation

### Compile

Compile the server application on the Raspberry Pi using CMake:

```sh
git clone https://github.com/fabio-terranova/ocmfet-server-feedback
cd ocmfet-server-feedback
mkdir build
cd build
cmake ..
make
```

### Run

Run the server on the Raspberry Pi with default port 8888:

```sh
sudo ./build/server 8888
```
