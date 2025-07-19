#include<iostream>
#include <string>
#include <winsock2.h>
#include "redisserver.h"
#include<thread>
#include<chrono>
#include<rediscommandhandler.h>
#include<redisdatabase.h>
#pragma comment(lib, "ws2_32.lib")
using namespace std;
int main(int argc, char* argv[]) {
	WSADATA wsaData;
	int wsaInit = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (wsaInit != 0) {
		std::cerr << "WSAStartup failed. Error: " << wsaInit << "\n";
		return 1;
	}
	int port = 6379;
	if (argc >= 2) {
		port = std::stoi(argv[1]);
	}
	redisserver server(port);

	//backgrounud dump for 300 ever seconds
	std::thread persistanceThread([]() {
		while (true) {
			std::this_thread::sleep_for(std::chrono::seconds(300));
			//dump the database
		}
		});
	persistanceThread.detach();
	server.run();

	return 0;
}
