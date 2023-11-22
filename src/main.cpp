#include "hw_peripherals.hpp"
#include "server.hpp"

#include <iostream>
#include <unistd.h>

using namespace std;

string data_folder = "/home/pi/data/";

int main(int argc, char *argv[])
{
	if (argc != 2)
	{
		cerr << "Usage: " << argv[0] << " <port>" << endl;
		return 1;
	}

	int port = atoi(argv[1]);
	
	init_system();

	while (true)
	{
		Server server(port, data_folder, T2_DEFAULT);
		server.Run();

		usleep(1000000);
	}

	return 0;
}