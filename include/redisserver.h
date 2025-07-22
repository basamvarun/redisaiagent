#ifndef REDIS_SERVER_H
#define REDIS_SERVER_H

#include <string>
#include <thread> 
#include <winsock2.h> 
#include <ws2tcpip.h> 
#include <signal.h>
#pragma comment(lib, "ws2_32.lib") 


class redisserver;
extern redisserver* globalServer;

class redisserver {
public:
    redisserver(int port);
    void run();
    void shutdown();

private:
    int port;
    SOCKET server_socket; 
    bool running;

   
    void setupSignalHandler();
};


void signalHandler(int signum);

#endif 
