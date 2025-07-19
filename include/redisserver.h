
#ifndef REDIS_SERVER_H
#define REDIS_SERVER_H
#include <string>
#include <atomic>
class redisserver {
public:
	redisserver(int port);
	void run();
	void shutdown();
	
private:
	int port;
	int server_socket;
	std::atomic<bool> running;
	
};
#endif // !REDIS_SERVER_H
