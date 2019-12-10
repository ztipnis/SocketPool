#include <sys/socket.h>
#include <netinet/in.h> 
#include <arpa/inet.h>
#include "ThreadPool/ThreadPool.h"
#include "Pollster/Pollster.hpp"

#ifndef __SOCKET_POOL_H__
#define __SOCKET_POOL_H__

class SocketPool {
public:
	SocketPool(unsigned short port, const char* addr, int max_Clients, int max_Threads, const Pollster::Handler& T, std::chrono::seconds gcInterval);
	void listen();
private:
	void accept();
	int sock;
	const Pollster::Handler& handler;
	std::vector<Pollster::Pollster> p;
	int cliPerPollster;
	int pollsters;
	ThreadPool pool;
	std::chrono::seconds timeout;
};

#endif