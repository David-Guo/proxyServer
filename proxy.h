#include <iostream>
#include <stdio.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string>
#include <fcntl.h>
#include <sys/select.h>



using namespace std;

class ProxyServer {
    public:
        void boost(int port, int rule);
        int passivesock(int port);
        void error(const char *eroMsg);
        void handleRequest(int);
        void connectMode(int sockfd, string ip, string port, unsigned char *SOCK4_REPLY);
        void bindMode(int sockfd, string ip, string port);


    public:
        int modeRule;
        string srcIP;
        string srcPort;

};

