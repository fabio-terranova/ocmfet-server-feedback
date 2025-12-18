g++ -std=c++20 -Wall -o build/server src/main.cpp src/hw_peripherals.cpp src/acquirer.cpp s
rc/server.cpp -lbcm2835 -lpthread -lrt