#include <sys/event.h>
#include <unistd.h>
#include <chrono>
#include <ctime>
#include <string>
#include <vector>
#include <iostream>
#include "ThreadPool/ThreadPool.h"

namespace Pollster{

	class Pollster;

	class client{
	public:
		int fd;
		std::chrono::time_point<std::chrono::system_clock> last_cmd;

		client(int f){
			fd = f;
			last_cmd = std::chrono::system_clock::now();
		}

		bool operator==(int f){
			return fd == f;
		}
		bool hasExpired(std::chrono::milliseconds timeout){
			auto nw =  std::chrono::system_clock::now();
			return std::chrono::duration_cast<std::chrono::milliseconds>(nw-last_cmd) >= timeout; 
		}
	};


	class Handler{
	public:
		virtual void operator()(int fd) = 0;
		virtual void disconnect(int fd, std::string reason) = 0;
	};

	class Pollster{
	public:
		Pollster(unsigned int max_clients, Handler& t) : kq(kqueue()), clients_max(max_clients), T(t){
			if(kq == -1){
				throw std::runtime_error("Unable to start pollster");
			}
		}
		~Pollster(){
			for(int i = 0; i < clients.size(); i++){
				rmClient(clients[i].fd, "Server is shutting down");
			}
			close(kq);
		}
		void operator()() { loop(); }
		void operator()(ThreadPool& pool){ auto result = pool.enqueue( [](Pollster* t){t->loop();}, this); }
		Pollster& operator=(const Pollster& p) = delete;
		bool canAddClient(){
			return clients.size() < clients_max;
		}
		bool addClient(int fd);
		bool rmClient(int fd, std::string reason);
		void setTimeout(std::chrono::milliseconds tout){
			timeout = tout;
		}
		void check_timeout(){
			if(timeout.count() <= 0) return;
			for(int i = 0; i < clients.size(); i++){
				if(clients[i].hasExpired(timeout)){
					rmClient(clients[i].fd, "Timeout");
				}
			}
		}
	private:
		int kq;
		std::vector<client> clients;
		unsigned int clients_max;
		std::chrono::milliseconds timeout;
		Handler& T;
		void loop();
		struct kevent evSet;
	};


	bool Pollster::addClient(int fd){
		EV_SET(&evSet, fd, EVFILT_READ, EV_ADD, 0, 0, NULL);
		if(kevent(kq, &evSet, 1, NULL, 0, NULL) != -1){
			client c(fd);
			clients.push_back(c);
			return true;
		}else{
			return false;
		}
	}

	bool Pollster::rmClient(int fd, std::string reason){
		auto it = std::find(clients.begin(), clients.end(), fd);
		if(it != clients.end()){
			clients.erase(it);
			T.disconnect(fd, reason);
			return true;
		}else{
			return false;
		}
	}


	void Pollster::loop(){
		struct kevent evList[32];
		int nev;
	    while(1){
	    	nev = kevent(kq, NULL, 0, evList, 32, NULL);
	    	if(nev < 1){
	    		for(int i = 0; i < clients.size(); i++){
	    			T.disconnect(clients[i].fd, "KQueue reported error polling for events");
	    		}
	    		throw std::runtime_error("kqueue reported error polling for events");
	    	}
	    	for(int i = 0; i < nev; i++){
	    		int fd = evList[i].ident;
	    		if(evList[i].flags & EV_EOF){
	    			EV_SET(&evSet, fd, EVFILT_READ, EV_DELETE, 0, 0, NULL);
	    			if(kevent(kq, &evSet, 1, NULL, 0, NULL) != -1){
	    				auto it = std::find(clients.begin(), clients.end(), fd);
	    				if(it != clients.end()){
							clients.erase(it);
							close(fd);
						}
	    			}
	    		}else{
	    			T(fd);
	    		}
	    	}
	    }
	}
}