#include "SocketPool.hpp"
inline void send(int fd, std::string data, int length){
	#ifndef SO_NOSIGPIPE
		send(fd, &data[0], length, MSG_DONTWAIT | MSG_NOSIGNAL);
	#else
		send(fd, &data[0], length, MSG_DONTWAIT);
	#endif
}


SocketPool::SocketPool(unsigned short port, const char* addr, int max_Clients, int max_Threads, const Pollster::Handler& T, std::chrono::seconds gcInterval):sock(socket(AF_INET, SOCK_STREAM, 0)), handler(T), cliPerPollster(max_Clients/max_Threads), pollsters(max_Threads), pool(max_Threads + 1), timeout(gcInterval){
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

	static const int one(1);
	#ifdef SO_NOSIGPIPE
		setsockopt(sock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT | SO_NOSIGPIPE, &one, sizeof(int));
	#else
		setsockopt(sock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT , &one, sizeof(int));
	#endif
	if(bind(sock, reinterpret_cast<sockaddr*>(&sockopt), sizeof(sockopt)) < 0){
		throw std::runtime_error("Unable to bind to port");
	}
	if(gcInterval.count() > 0){
		pool.enqueue( [](std::chrono::seconds s, std::vector<Pollster::Pollster> *psters){
			while(true){
				sleep(s.count() / 10);
				for(unsigned int i = 0; i < psters->size(); i++){
					(*psters)[i].cleanup();
				}
			}
		}, gcInterval, &p);
	}
}

void SocketPool::listen(){
	if(::listen(sock, 5) < 0){
		throw std::runtime_error("Unable to listen on socket");
	}
	for(unsigned int i = 0; i < p.size(); i++){
		p[i].cleanup();
	}
	this->accept();
}

void SocketPool::accept(){
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
				p[p.size()-1].setTimeout(timeout);
				if(!p[p.size()-1].addClient(cli_fd)){
					throw std::runtime_error("Unable to add client to new Pollster");
				}
				pool.enqueue( [](Pollster::Pollster* t){(*t)();}, &(p[p.size()-1]));
			}else{
				handler.disconnect(cli_fd, "Too many simultaneos connections...");
			}
		}
	}
}