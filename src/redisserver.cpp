#include "../include/redisserver.h"
#include "../include/rediscommandhandler.h"
#include "../include/redisdatabase.h"

#include <iostream>
#include <vector>
#include <thread>
#include <cstring>

#include <winsock2.h>
#include <ws2tcpip.h>

redisserver* globalServer = nullptr;

void signalHandler(int signum) {
    if (globalServer) {
        std::cout << "\nCaught signal " << signum << ", shutting down...\n";
        globalServer->shutdown();
    }
    exit(signum);
}

void redisserver::setupSignalHandler() {
    signal(SIGINT, signalHandler);
}

redisserver::redisserver(int port)
    : port(port), server_socket(INVALID_SOCKET), running(true) {
    globalServer = this;
    setupSignalHandler();
}

void redisserver::shutdown() {
    running = false;

    if (redisdatabase::getInstance().dump("dump.my_rdb")) {
        std::cout << "Database dumped to dump.my_rdb\n";
    }
    else {
        std::cerr << "Error dumping database\n";
    }

    if (server_socket != INVALID_SOCKET) {
        closesocket(server_socket);
        server_socket = INVALID_SOCKET;
    }
    std::cout << "Server shutdown complete!\n";
}

void redisserver::run() {
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        std::cerr << "WSAStartup failed: " << iResult << "\n";
        return;
    }

    server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_socket == INVALID_SOCKET) {
        std::cerr << "Failed to create socket. Error: " << WSAGetLastError() << "\n";
        WSACleanup();
        return;
    }

    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt)) == SOCKET_ERROR) {
        std::cerr << "setsockopt failed. Error: " << WSAGetLastError() << "\n";
        closesocket(server_socket);
        WSACleanup();
        return;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_socket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed. Error: " << WSAGetLastError() << "\n";
        closesocket(server_socket);
        WSACleanup();
        return;
    }

    if (listen(server_socket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed. Error: " << WSAGetLastError() << "\n";
        closesocket(server_socket);
        WSACleanup();
        return;
    }

    std::cout << "redis Server Listening On Port " << port << "\n";

    std::vector<std::thread> threads;
    rediscommandhandler cmdHandler;

    while (running) {
        SOCKET client_socket = accept(server_socket, nullptr, nullptr);
        if (client_socket == INVALID_SOCKET) {
            if (running) {
                std::cerr << "Error accepting client connection. Error: " << WSAGetLastError() << "\n";
            }
            break;
        }

        threads.emplace_back([client_socket, &cmdHandler]() {
            char buffer[1024];
            while (true) {
                memset(buffer, 0, sizeof(buffer));
                int bytes = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
                if (bytes <= 0) {
                    if (bytes == SOCKET_ERROR) {
                    }
                    break;
                }
                std::string request(buffer, bytes);
                std::string response = cmdHandler.processCommand(request);
                send(client_socket, response.c_str(), response.size(), 0);
            }
            closesocket(client_socket);
            });
    }

    for (auto& t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }

    WSACleanup();
}
