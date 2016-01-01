#include "proxy.h"
#include "unistd.h"
#include "stdlib.h"


int main(int argc, char* argv[]) {
    ProxyServer proxyServer;
    if (argc != 3) {
        cout << "Usage: ./proxy [prot] [rule]" << endl;
        exit(1);
    }
    proxyServer.boost(atoi(argv[1]), atoi(argv[2]));

    return 0;
}
        
