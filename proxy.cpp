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
    // 代理模式规则
    modeRule = rule;

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
        srcPort = to_string((int) ntohs(client_addr.sin_port));

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

void ProxyServer::handleRequest(int sockfd) {
    unsigned char SOCK4_REQUEST[20];
    // 接收SOCK4 请求头
    while(recv(sockfd, SOCK4_REQUEST, 20, 0) <= 0);
    // 返回 SOCK4 响应头
    unsigned char SOCK4_REPLY[8] = {0};
    SOCK4_REPLY[0] = 0;
    SOCK4_REPLY[1] = 90; // granted
    for (int i = 2; i < 8; i++)
        SOCK4_REPLY[i] = SOCK4_REQUEST[i];

    // 解析端口与ip
    string dstPort = to_string(((int)SOCK4_REPLY[2]) * 256 + ((int)SOCK4_REPLY[3]));
    string dstIP = to_string((int)SOCK4_REPLY[4]);
    dstIP += '.';
    dstIP += to_string((int)SOCK4_REPLY[5]);
    dstIP += '.';
    dstIP += to_string((int)SOCK4_REPLY[6]);
    dstIP += '.';
    dstIP += to_string((int)SOCK4_REPLY[7]);

    // 检查代理规则
    bool isOK = true;
    if (modeRule == 0); // 全部允许
    else if (modeRule == 1) {  // nctu 允许
        if ((int)SOCK4_REPLY[4] == 140 && (int)SOCK4_REPLY[5] == 113);
        else isOK = false;
    } 
    else { // nhtu 允许
        if ((int)SOCK4_REPLY[4] == 140 && (int)SOCK4_REPLY[5] == 114);
        else isOK = false;
    }
    if (!isOK) {
        cout << "Invalid Connecting" << endl;
        // 返回错误请求
        SOCK4_REPLY[1] = 91;
        write(sockfd, SOCK4_REPLY, 8);
        cout << "<S_IP>:   " << srcIP << endl;
        cout << "<S_Port>: " << srcPort << endl;
        cout << "<D_IP>;   " << dstIP << endl;
        cout << "<D_Port>: " << dstPort << endl;
        cout << "<Reply>:  " << "Denied" << endl;
        return;
    }
    else {
        cout << "<S_IP>:   " << srcIP << endl;
        cout << "<S_Port>: " << srcPort << endl;
        cout << "<D_IP>;   " << dstIP << endl;
        cout << "<D_Port>: " << dstPort << endl;
        cout << "<Reply>:  " << "Granted" << endl;
    }

    // 检查连接模式 connect or bing
    if ((int)SOCK4_REQUEST[1] == 1) 
        connectMode(sockfd, dstIP, dstPort, SOCK4_REPLY);
    else 
        bindMode(sockfd, dstIP, dstPort);

    return;
}

void ProxyServer::connectMode(int sockfd, string ip, string port, unsigned char *SOCK4_REPLY) {
    // rws: remote work shell
    int rwsfd;
    struct sockaddr_in rws_sin;
    struct hostent *he;
    struct in_addr ipv4addr;

    if (!inet_aton(ip.c_str(), &ipv4addr))
        ;

    he = gethostbyaddr(&ipv4addr, sizeof(ipv4addr), AF_INET);
    rwsfd = socket(AF_INET, SOCK_STREAM, 0);
    rws_sin.sin_family = AF_INET;
    rws_sin.sin_addr = *((struct in_addr *) he->h_addr);
    rws_sin.sin_port = htons(atoi(port.c_str()));

    // 设置 non-blocking socket
    int flags;
    if ((flags = fcntl(rwsfd, F_GETFL, 0)))
        flags = 0;
    fcntl(rwsfd, F_SETFL, flags | O_NONBLOCK);

    // 连接到 rws
    while (1) {
        if (connect(rwsfd, (struct sockaddr *)&rws_sin, sizeof(rws_sin)) == -1) 
            ;
        else 
            break;
    }

    fd_set rfds, afds;
    FD_ZERO(&rfds);
    FD_ZERO(&afds);

    int maxfd = 0;
    if (rwsfd > maxfd) maxfd = rwsfd;
    if (sockfd > maxfd) maxfd = sockfd;
    FD_SET(rwsfd, &afds);
    FD_SET(sockfd, &afds);
    int stopCount = 0;

    write(sockfd, SOCK4_REPLY, 8);

    while (1) {
        memcpy(&rfds, &afds, sizeof(afds));
        if (select(maxfd + 1, &rfds, NULL, NULL, NULL) < 0) 
            return;
        int recvByte = 0;
        int bufSize = 16384000;
        char buf[bufSize];
        memset(buf, 0, sizeof(buf));

        if (FD_ISSET(rwsfd, &rfds)) {
            if ((recvByte = recv(rwsfd, buf, bufSize, 0)) > 0) {
                write(sockfd, buf, recvByte);
                cout << "<S_IP>:   " << srcIP << endl;
                cout << "<S_PORT>: " << srcPort << endl;
                cout << "<D_IP>:   " << ip << endl;
                cout << "<D_PORT>: " << port << endl;
                cout << "<Contetn>:\n" << string(buf).substr(0, 100) << endl;
                memset(buf, 0, sizeof(buf));
            }
            else {
                FD_CLR(rwsfd, &afds);
                stopCount++;
            }
        }

        if (FD_ISSET(sockfd, &rfds)) {
            if ((recvByte = recv(sockfd, buf, bufSize, 0)) > 0) {
                write(rwsfd, buf, recvByte);
                cout << "<S_IP>:   " << srcIP << endl;
                cout << "<S_PORT>: " << srcPort << endl;
                cout << "<D_IP>:   " << ip << endl;
                cout << "<D_PORT>: " << port << endl;
                cout << "<Contetn>:\n" << string(buf).substr(0, 100) << endl;
                memset(buf, 0, sizeof(buf));
            }
            else {
                FD_CLR(sockfd, &afds);
                stopCount++;
            }
        }

        if (stopCount == 2)
            break;
    }
    close(sockfd);
    close(rwsfd);

}

void ProxyServer::bindMode(int sockfd, string ip, string port) {
    int msock = passivesock(INADDR_ANY);
    int size, sa_len;
    struct sockaddr_in sa;
    sa_len = sizeof(sa);
    size = getsockname(msock, (struct sockaddr *)&sa, (socklen_t *)&sa_len);
    if (size == -1) 
        cerr << "getsockname failed" << endl;

    // 返回响应头
    unsigned char SOCK4_REPLY[8] = {0};
    SOCK4_REPLY[0] = 0;
    SOCK4_REPLY[1] = 90;
    SOCK4_REPLY[2] = (unsigned char)((ntohs(sa.sin_port))/256);
    SOCK4_REPLY[3] = (unsigned char)((ntohs(sa.sin_port))%256);
    for (int i = 4; i < 8; i++)
        SOCK4_REPLY[i] = 0;

    // 第一次响应
    write(sockfd, SOCK4_REPLY, 8);

    struct sockaddr_in client_addr;
    socklen_t alen;
    alen = sizeof(client_addr);
    // 接收到 dest host 的sock 连接
    int dstHostfd = accept(msock, (struct sockaddr *) &client_addr, &alen);
    // 第二次响应
    write(sockfd, SOCK4_REPLY, 8);

    // 开始传递数据
    fd_set rfds, afds;
    FD_ZERO(&rfds);
    FD_ZERO(&afds);

    int maxfd = 0;
    if (dstHostfd > maxfd) maxfd = dstHostfd;
    if (sockfd > maxfd) maxfd = sockfd;

    FD_SET(dstHostfd, &afds);
    FD_SET(sockfd, &afds);

    bool isServeFinsh = false;
    bool isClientFinsh = false;
    while (1) {
        FD_ZERO(&rfds);
        memcpy(&rfds, &afds, sizeof(afds));
        if (select(maxfd + 1, &rfds, NULL, NULL, NULL) < 0) 
            return;

        int recvByte = 0;
        char buf[4096];
        memset(buf, 0, sizeof(buf));

        if (FD_ISSET(dstHostfd, &rfds)) {
            if ((recvByte = recv(dstHostfd, buf, 4096, 0)) > 0) {
                write(sockfd, buf, recvByte);
                cout << "<S_IP>:   " << srcIP << endl;
                cout << "<S_PORT>: " << srcPort << endl;
                cout << "<D_IP>:   " << ip << endl;
                cout << "<D_PORT>: " << port << endl;
                cout << "<Contetn>:\n" << string(buf).substr(0, 100) << endl;
                memset(buf, 0, sizeof(buf));
            }
            else {
                FD_CLR(dstHostfd, &afds);
                isServeFinsh = true;
                break;
            }
        }
        if (FD_ISSET(sockfd, &rfds)) {
            if ((recvByte = recv(sockfd, buf, 4096, 0)) > 0) {
                write(dstHostfd, buf, recvByte);
                cout << "<S_IP>:   " << srcIP << endl;
                cout << "<S_PORT>: " << srcPort << endl;
                cout << "<D_IP>:   " << ip << endl;
                cout << "<D_PORT>: " << port << endl;
                cout << "<Contetn>:\n" << string(buf).substr(0, 100) << endl;

                memset(buf, 0, sizeof(buf));
            }
            else {
                FD_CLR(sockfd, &afds);
                isClientFinsh = true;
                break;
            }
        }
        if (isServeFinsh && isClientFinsh) 
            break;
    }
    close(sockfd);
    close(dstHostfd);
}

