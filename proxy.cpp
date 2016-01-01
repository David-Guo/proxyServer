#include "proxy.h"
#define QLEN  30

int ProxyServer::passivesock(int port) {
    struct protoent *ppe;
    struct sockaddr_in serv_addr;
    if ((ppe = getprotobyname("tcp")) == 0)
        error("can't get prtocal entry");

    /* Open a TCP socker (an Internet stream socket). */
    int msock = socket(PF_INET, SOCK_STREAM, ppe->p_proto);
    if (msock == -1)
        error("Can't open socket");

    /* Bind our local address so that the client can send to us. */
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(port);

    if (bind(msock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        error("Can't bind");
    printf("\t[Info] Binding...\n");

    if (listen(msock, QLEN) < 0)
        error("Can't listen");
    printf("\t[Info] Listening...\n");

    return msock;
}

void ProxyServer::error(const char *eroMsg) {
    cerr << eroMsg << ":" << strerror(errno) << endl;
    exit(1);
}

void ProxyServer::boost(int port, int rule){
    struct sockaddr_in client_addr;
    socklen_t alen;
    alen = sizeof(client_addr);

    int passiveListen = passivesock(port);
    while (1) {
        int newsockfd = accept(passiveListen, (struct sockaddr *) &client_addr, &alen);
        if (newsockfd <0 ) {
            close(newsockfd);
            continue;
        }

        char clentIP[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(client_addr.sin_addr), clentIP, sizeof(clentIP));  
        srcIP = clentIP;
        srcPort = to_string((int) ntohs(clent_addr.sin_port));
        
        int childpid = fork();
        if (childpid  < 0) 
            cerr << "passiveListen fork error" << endl;
        else if (childpid == 0) {
            handleRequest(newsockfd);
            if (passiveListen) 
                close(passiveListen);
        }
        else
            close(newsockfd);
    }


}
