#include<redisserver.h>
#include "redisdatabase.h"
#include<algorithm>
#include<string>
#include<vector>
#include<sstream>
#include<exception>
#include<iostream>
#include "rediscommandhandler.h"


std::vector<std::string> parseRespcommand(const std::string& input) {
	std::vector < std::string> tokens;
	if (input.empty())return tokens;

	if (input[0] != '*') {
		std::istringstream iss(input);
		std::string token;
		while (iss >> token) {
			tokens.push_back(token);
		}
		return tokens;
	}

	size_t pos = 0;
	if (input[pos] != '*')return tokens;
	pos++;
	size_t  crlf = input.find("\r\n", pos);
	if (crlf == std::string::npos)return tokens;
	int numelements = std::stoi(input.substr(pos, crlf - pos));
	pos = crlf + 2;
	for (int i = 0; i < numelements; i++) {
		if (pos >= input.size() || input[pos] != '$') break; // format error
		pos++; // skip '$'

		crlf = input.find("\r\n", pos);
		if (crlf == std::string::npos) break;
		int len = std::stoi(input.substr(pos, crlf - pos));
		pos = crlf + 2;

		if (pos + len > input.size()) break;
		std::string token = input.substr(pos, len);
		tokens.push_back(token);
		pos += len + 2;
	}
	return tokens;
}
rediscommandhandler::rediscommandhandler() {}
std::string rediscommandhandler::processCommand(const std::string& commandLine) {
	auto tokens = parseRespcommand(commandLine);
	if (tokens.empty())return "error empty command";
	std::string cmd = tokens[0];
	std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);

}