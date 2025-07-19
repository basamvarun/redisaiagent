#include<vector>
#include<redisdatabase.h>
#include<rediscommandhandler.h>
#include<redisserver.h>
#include<string>
#include<algorithm>
#include<iostream>

redisdatabase& redisdatabase::getInstance() {
	static redisdatabase instance;
	return instance;
}