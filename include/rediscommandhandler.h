#ifndef REDIS_COMMAND_HANDLER_H
#define REDIS_COMMAND_HANDLER_H
#include<string>
class rediscommandhandler {
public:
	rediscommandhandler();
	std::string processCommand(const std::string& commandLine);
};

#endif
