#include "redisserver.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include "rediscommandhandler.h"
#include<vector>
#include<thread>

#pragma comment(lib, "ws2_32.lib") // Link WinSock library

redisserver::redisserver(int port)
    : port(port), server_socket(INVALID_SOCKET), running(true)
{
}

void redisserver::run() {
    // Create socket
    server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_socket == INVALID_SOCKET) {
        std::cerr << "Failed to create socket. Error: " << WSAGetLastError() << "\n";
        return;
    }

    // Set socket options
    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

    // Configure server address
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    // Bind
    if (bind(server_socket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed. Error: " << WSAGetLastError() << "\n";
        closesocket(server_socket);
        return;
    }

    // Listen
    if (listen(server_socket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed. Error: " << WSAGetLastError() << "\n";
        closesocket(server_socket);
        return;
    }

    std::cout << "Redis server listening on port " << port << "\n";
    
    std::vector<std::thread> threads;
    rediscommandhandler cmdhandler;
    while (running) {
        int client_socket = accept(server_socket, nullptr, nullptr);
        if (client_socket < 0) {
            if (running)std::cerr << "error accpeting clinet connection\n";
            break;
        }
        threads.emplace_back([client_socket, &cmdhandler]() {
            char buffer[1024];
            while (true) {
                memset(buffer, 0, sizeof(buffer));
                int bytes = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
                if (bytes < 0)break;
                std::string request(buffer, bytes);
                std::string response = cmdhandler.processCommand(request);
                send(client_socket, response.c_str(), response.size(), 0);
            }
            closesocket(client_socket);
            });
    }
}

void redisserver::shutdown() {
    running = false;
    if (server_socket != INVALID_SOCKET) {
        closesocket(server_socket);
    }
    std::cout << "Server shutdown complete!\n";
}
