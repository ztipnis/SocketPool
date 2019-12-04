#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h> 
#include <arpa/inet.h>
#include "ThreadPool/ThreadPool.h"
#include "Pollster.hpp"

inline void send(int fd, std::string data, int length){
	#ifndef SO_NOSIGPIPE
		send(fd, &data[0], length, MSG_DONTWAIT | MSG_NOSIGNAL);
	#else
		send(fd, &data[0], length, MSG_DONTWAIT);
	#endif
}

class SocketPool {
public:
	SocketPool(unsigned short port, const char* addr, int max_Clients, int max_Threads, const Pollster::Handler& T):sock(socket(PF_INET, SOCK_STREAM, 0)), handler(T), cliPerPollster(max_Clients/max_Threads), pollsters(max_Threads), pool(max_Threads){
		p.reserve(max_Threads);
		sockaddr_in sockopt;
		if(sock < 0){
			throw std::runtime_error("Unable to create socket");
		}
		sockopt.sin_family = AF_INET;
		sockopt.sin_port = htons(port);
		if(inet_aton(addr, &(sockopt.sin_addr)) < 0){
			throw std::runtime_error("Listen Address Invalid");
		}
		if(bind(sock, reinterpret_cast<sockaddr*>(&sockopt), sizeof(sockopt)) < 0){
			throw std::runtime_error("Unable to bind to port");
		}
		int opt = 1;
		#ifdef SO_NOSIGPIPE
			setsockopt(sock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT | SO_NOSIGPIPE, &opt, sizeof(opt));
		#else
			setsockopt(sock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT , &opt, sizeof(opt));
		#endif

	}
	void listen(std::chrono::seconds gcInterval){
		if(::listen(sock, 5) < 0){
			throw std::runtime_error("Unable to listen on socket");
		}
		if(gcInterval.count() > 0){
			timeval tv;
			fd_set rfds;
			tv.tv_sec = gcInterval.count();
			tv.tv_usec = 0;
			FD_ZERO(&rfds);
           	FD_SET(sock, &rfds);
           	int retval = select(1, &rfds, NULL, NULL, &tv);
           	if(retval < 0){
           		throw std::runtime_error("Polling Listening Socket Failed");
           	}else if(retval > 0){
           		this->accept();
           	}
		}else{
			this->accept();
		}
	}
private:
	void accept(){
		sockaddr_in address;
   		socklen_t adrlen = static_cast<socklen_t>(sizeof(address));
   		int cli_fd = ::accept(sock, reinterpret_cast<sockaddr*>(&address), &adrlen);
   		if(cli_fd > 0){
   			bool assigned = false;
   			for(int i = 0; i < p.size(); i++){
   				if(p[i].canAddClient()){
   					if(p[i].addClient(cli_fd)){
   						assigned = true;
   						break;
   					}
   				}
   			}
   			if(!assigned){
   				if(p.size() < pollsters){
   					p.emplace_back(cliPerPollster,handler);
       				if(!p[p.size()-1].addClient(cli_fd)){
       					throw std::runtime_error("Unable to add client to new Pollster");
       				}
       				p[p.size()-1](pool);
   				}else{
   					handler.disconnect(cli_fd, "Too many simultaneos connections...");
   				}
   			}
   		}
	}
	int sock;
	const Pollster::Handler& handler;
	std::vector<Pollster::Pollster> p;
	int cliPerPollster;
	int pollsters;
	ThreadPool pool;
};