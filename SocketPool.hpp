#include <sys/socket.h>
#include <netinet/in.h> 
#include <arpa/inet.h>
#include "ThreadPool/ThreadPool.h"
#include "Pollster.hpp"

#ifndef __SOCKET_POOL_H__
#define __SOCKET_POOL_H__


inline void send(int fd, std::string data, int length){
	#ifndef SO_NOSIGPIPE
		send(fd, &data[0], length, MSG_DONTWAIT | MSG_NOSIGNAL);
	#else
		send(fd, &data[0], length, MSG_DONTWAIT);
	#endif
}

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

#endif