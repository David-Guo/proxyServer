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

using namespace std;

class ProxyServer {
    public:
        void boost(int port, int rule);
        int passivesock(int port);
        void error(const char *eroMsg);
        void handleRequest(int);

    public:
        int srcIP;
        int srcPort;

};

