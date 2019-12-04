#import "../SocketPool.hpp"


class EchoHandler : public Pollster::Handler {
	public:
		void operator()(int fd) const{
			std::string data(8193, 0);
			int rcvd = recv(fd, &data[0], 8192, MSG_DONTWAIT);
			if( rcvd == -1){
				throw std::runtime_error("Unable to read from socket");
			}else{
				data.resize(rcvd);
			#ifndef SO_NOSIGPIPE
				send(fd, &data[0], rcvd, MSG_DONTWAIT | MSG_NOSIGNAL);
			#else
				send(fd, &data[0], rcvd, MSG_DONTWAIT);
			#endif
			}
		}
		void disconnect(int fd, std::string reason) const{
			#ifndef SO_NOSIGPIPE
				send(fd, &reason[0], reason.length(), MSG_DONTWAIT | MSG_NOSIGNAL);
			#else
				send(fd, &reason[0], reason.length(), MSG_DONTWAIT);
			#endif
			close(fd);
		}
		void connect(int fd) const{}
};

int main(int argc, char* argv[]){
	if(argc < 3){
		std::cerr << "USAGE: " << argv[0] << " #clients #threads" << std::endl;
		exit(1);
	}
	std::cout << "Note: if # clients is not evently divisible by # threads, # clients will be truncated to be distibuted evenly amongst threads." << std::endl;
	unsigned int port = 8080;
	const char* address = "0.0.0.0";
	EchoHandler p = EchoHandler();
	SocketPool sp(port, address, atoi(argv[1]), atoi(argv[2]), p);
	while(1){
		sp.listen(std::chrono::minutes(0));
	}
}