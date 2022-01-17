#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#ifdef __linux__
#include <arpa/inet.h>
#include <sys/socket.h>
#endif // __linux
#ifdef WIN32
#include <winsock2.h>
#include "../mingw_net.h"
#endif // WIN32
#include <thread>
#include <mutex>
#include <list> 

using namespace std;

#ifdef WIN32
void perror(const char* msg) { fprintf(stderr, "%s %ld\n", msg, GetLastError()); }
#endif // WIN32

list<int> cli_list;

void usage() {
	printf("syntax\n");
	printf("  -e : echo\n");
	printf("sample: \n");
}

struct Param {
	bool echo{false};
	uint16_t port{0};
	bool broad{false};

	bool parse(int argc, char* argv[]) {
		for (int i = 1; i < argc; i++) { 
			if (strcmp(argv[i], "-e") == 0) {
				echo = true;
				continue;
			}
			if (strcmp(argv[i], "-b") == 0) {
				broad = true;
				continue;
			}
			port = atoi(argv[i++]);
		}
		return port != 0;
	}
} param;

void recvThread(int sd) {
	printf("connected\n");
	static const int BUFSIZE = 65536;
	char buf[BUFSIZE];
	static mutex m;
	
	while (true) {
		ssize_t res = ::recv(sd, buf, BUFSIZE - 1, 0);
		if (res == 0 || res == -1) {
			fprintf(stderr, "recv return %ld", res);
			perror(" ");
			break;
		}
		buf[res] = '\0';
		printf("%s", buf); 
		fflush(stdout);
		if (param.echo) {
			res = ::send(sd, buf, res, 0); 
			if (res == 0 || res == -1) {
				fprintf(stderr, "send return %ld", res);
				perror(" ");
				break;
			}
			if(param.broad){
				list<int>::iterator iter;
				for(iter =cli_list.begin(); iter!=cli_list.end();iter++){
					if(*iter == sd) continue;
					res = ::send(*iter, buf, res, 0);
					if (res == 0 || res == -1) {
						fprintf(stderr, "send broadcast return %ld", res);
						perror(" ");
						break;
					}
				}
			}
		}
	}
	printf("disconnected\n");
	m.lock();
	cli_list.remove(sd);
	m.unlock();
	::close(sd);
}

int main(int argc, char* argv[]) {
	if (!param.parse(argc, argv)) {
		usage();
		return -1;
	}

#ifdef WIN32
	WSAData wsaData;
	WSAStartup(0x0202, &wsaData);
#endif // WIN32

	int sd = ::socket(AF_INET, SOCK_STREAM, 0);
	if (sd == -1) {
		perror("socket");
		return -1;
	}

	int res;
#ifdef __linux__
	int optval = 1;
	res = ::setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
	if (res == -1) {
		perror("setsockopt");
		return -1;
	}
#endif // __linux

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(param.port);

	ssize_t res2 = ::bind(sd, (struct sockaddr *)&addr, sizeof(addr));
	if (res2 == -1) {
		perror("bind");
		return -1;
	}

	res = listen(sd, 5);
	if (res == -1) {
		perror("listen");
		return -1;
	}
	static mutex m;

	while (true) {
		struct sockaddr_in cli_addr;
		socklen_t len = sizeof(cli_addr);
		int cli_sd = ::accept(sd, (struct sockaddr *)&cli_addr, &len);
		m.lock();
		cli_list.push_back(cli_sd);
		m.unlock();
		if (cli_sd == -1) {
			perror("accept");
			break;
		}
		std::thread* t = new std::thread(recvThread, cli_sd);
		t->detach();
	}
	::close(sd);
}

