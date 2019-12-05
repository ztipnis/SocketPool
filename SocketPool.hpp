#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h> 
#include <arpa/inet.h>
#include "ThreadPool/ThreadPool.h"
#include "Pollster.hpp"

class SocketPool {
public:
	SocketPool(unsigned short port, const char* addr, int max_Clients, int max_Threads, const Pollster::Handler& T);
	void listen(std::chrono::seconds gcInterval);
private:
	void accept();
	int sock;
	const Pollster::Handler& handler;
	std::vector<Pollster::Pollster> p;
	int cliPerPollster;
	int pollsters;
	ThreadPool pool;
};