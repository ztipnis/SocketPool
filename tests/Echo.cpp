#import "SocketPool/SocketPool.hpp"

int main(){
	unsigned int port = 8080;
	const char* address = "0.0.0.0";
	EchoHandler p = EchoHandler();
	SocketPool sp(port, address, 4, 2, p);
	while(1){
		sp.listen(std::chrono::minutes(0));
	}
}