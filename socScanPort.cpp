#include "definition.h"

int sendData_s(SOCKET s, const void *buffer, int buflen)
{
    const char *pbuf = (const char*) buffer;
    while (buflen > 0)
    {
        int numSent = send(s, pbuf, buflen, 0);
        if (numSent == SOCKET_ERROR)
            return SOCKET_ERROR;
        pbuf += numSent;
        buflen -= numSent;
    }
    return 1;
}

int recvData_s(SOCKET s, void *buffer, int buflen)
{
    char *pbuf = (char*) buffer;
    while (buflen > 0)
    {
        int numRecv = recv(s, pbuf, buflen, 0);
        if (numRecv == SOCKET_ERROR)
            return SOCKET_ERROR;
        if (numRecv == 0)
            return 0;
        pbuf += numRecv;
        buflen -= numRecv;
    }
    return 1;
}


SOCKET connnectSOCKS5packet1_s(GETQ * get,fd_set * rset, fd_set * wset){
    int initPacket1Length = 3;
    char initPacket1[initPacket1Length];
    initPacket1[0] = 5;
    initPacket1[1] = 1;
    initPacket1[2] = 0;

    if (sendData_s(get->fd, initPacket1, initPacket1Length) < 0){
        std::cerr << "Can't send first init packet to SOCKS server!" << std::endl;
        //closesocket(get->fd);
        return INVALID_SOCKET;
    }
    else {
        FD_CLR(get->fd, wset);
        FD_SET(get->fd, rset);
        get->flags = READRES1;
        return 1;
    }
}

bool checkConnnectSOCKS5packet1_s(GETQ * get, fd_set * rset, fd_set * wset){
    char reply1[2];
    //recv(get->fd, reply1, 2, 0);

    if (recvData_s(get->fd, reply1, 2) <= 0)
    {
        std::cerr<< "Error reading first init packet response!" <<"" << reply1 << std::endl;
        return false;
    }

    //std::cerr << "read 1: " << reply1 << std::endl;
    //std::cout <<"check 1" << reply1 << std::endl;
    //reply[0] = our version
    //reply[1] = out method. [X’00’ NO AUTHENTICATION REQUIRED]

    if( !(reply1[0] == 5 && reply1[1] == 0) )
    {
        std::cerr << "Bad response for init packet!" << std::endl;
        return false;
    }
    FD_CLR(get->fd, rset);
    FD_SET(get->fd, wset);
    return true;
}

SOCKET connnectSOCKS5packet2_s(GETQ * get, fd_set * rset, fd_set * wset){
    DestInfo destInfo;
    destInfo.hostname = "google.com";
    destInfo.port = 80;

    int hostlen = std::max((int)destInfo.hostname.size(), 255);

    //Building that packet
    char *initPacket2 = new char[7+hostlen];
    initPacket2[0] = 5; //SOCKS Version;
    initPacket2[1] = 1; //1 = CONNECT, 2 = BIND, 3 = UDP ASSOCIATE;
    initPacket2[2] = 0; //Reserved byte
    initPacket2[3] = 3; //1 = IPv4, 3 = DOMAINNAME, 4 = IPv6
    initPacket2[4] = (char) hostlen;
    memcpy(&initPacket2[5], destInfo.hostname.c_str(), hostlen);
    *((u_short*) &(initPacket2[5+hostlen])) = htons(destInfo.port);

    //Send the second init packet to server. This will inform the SOCKS5 server
    //what is our target.
    if (sendData_s(get->fd, initPacket2, 7+hostlen) < 0)
    {
        std::cerr << "Can't send second init packet!" << std::endl;
        delete[] initPacket2;
        return INVALID_SOCKET;
    }
    //std::cout <<"send 2" << std::endl;
    FD_CLR(get->fd, wset);
    FD_SET(get->fd, rset);
    get->flags = READRES2;
    delete[] initPacket2;
    return 1;
}

bool checkConnnectSOCKS5packet2_s(GETQ * get , fd_set * rset, fd_set * wset) {
    char reply2[262];
    if (recvData_s(get->fd, reply2, 4) <= 0)
    {
        std::cerr << "Error while reading response for second init packet!"<< std::endl;
        //closesocket(hSocketSock);
        return false;
    }
    //std::cout <<"check 2" << reply2 << std::endl;
    FD_CLR(get->fd, rset);
    FD_SET(get->fd, wset);
    if (!(reply2[0] == 5 && reply2[1] == 0))
    {
        std::cerr << "The connection between and SOCKS and DESTINATION can't be established!"<< std::endl;
        //closesocket(hSocketSock);
        return false;
    }
    return true;
}

int sendNon_s(GETQ *get, fd_set * rset, fd_set * wset, int * maxfd){
    int n, flags;

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;                                          // host byte order
    addr.sin_port = htons(get->port);                                   // short, network byte order
    addr.sin_addr.S_un.S_addr = inet_addr(get->host.c_str());           // adding socks server IP address into structure

    int sendfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    u_long iMode = 1;

    int iResult = ioctlsocket(sendfd, FIONBIO, &iMode);

    if (sendfd < 0){
        puts("can't create socket!!");
        closesocket(sendfd);
        return -1;
    }
    get->fd = sendfd;
    if ( (n = connect(sendfd, (struct sockaddr*) &addr, sizeof(addr) ) ) < 0 ){
        get->flags = CONNECTING;
        FD_SET(sendfd, rset);
        FD_SET(sendfd, wset);

        if (sendfd > *maxfd ){
            //std::cout <<"max : " << *maxfd  << " " << sendfd <<std::endl;
            *maxfd = sendfd;
        }
        return sendfd;
    } else{
        //closesocket(sendfd);
        connnectSOCKS5packet1_s(get, rset,wset);
        //return -1;
    }
}

bool checkPort(int port, std::vector<std::string>listIP, std::ofstream& outF){
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);

    std::vector<GETQ> listCon;
    listCon.clear();
    std::stack<GETQ> sCon;

    fd_set rset, wset;
    FD_ZERO(&rset);
    FD_ZERO(&wset);

    int maxfd = -1;
    rep(i,listIP.size()){
        GETQ que;
        que.fd = 0;
        que.host = listIP[i];
        que.flags = 0;
        que.port = port;
        sCon.push(que);
    }

    int nconn = 0;
    while ( !sCon.empty()  || nconn > 0){
        while (!sCon.empty() && nconn < NOS_DEFAULT) {
            int fdu = -1;
            GETQ con = sCon.top();
            sCon.pop();
            fdu = sendNon_s(&con, &rset, &wset, &maxfd);
            if (fdu >= 0 ){
                listCon.pb(con);
                ++nconn;
            }
        }
        fd_set rs,ws;
        rs = rset;
        ws = wset;

        struct timeval timeout;
        timeout.tv_sec = TIMEOUT;
        timeout.tv_usec = 0;

        int n = select(maxfd + 1, &rs, &ws, NULL, &timeout);
        //std::cout << "max: " << maxfd << std::endl;
        //rep(i,listCon.size())
        //    std::cout << listCon.size()<<" " <<listCon[i].host <<" " <<listCon[i].flags << " " <<maxfd<< std::endl;
        if (n > 0){
            rep(i,listCon.size()){
                int fdu = listCon[i].fd;
                int flags = listCon[i].flags;
                //std::cout << fdu <<" " << flags << std::endl;
                if ( (flags & CONNECTING) && ( FD_ISSET(fdu,&rs) || FD_ISSET(fdu,&ws) ) ){
                    if ( connnectSOCKS5packet1_s(&listCon[i] , &rset,&wset) == INVALID_SOCKET){
                        FD_CLR(fdu, &wset);
                        FD_CLR(fdu, &rset);
                        --nconn;
                        closesocket(fdu);
                    }
                } else if ( (flags & READRES1) && ( FD_ISSET(fdu,&rs) || FD_ISSET(fdu,&ws) ) ){
                    if ( checkConnnectSOCKS5packet1_s(&listCon[i] , &rset,&wset) ){
                        if (connnectSOCKS5packet2_s(&listCon[i] , &rset,&wset)  == INVALID_SOCKET){
                            FD_CLR(fdu, &wset);
                            FD_CLR(fdu, &rset);
                            --nconn;
                            closesocket(fdu);
                        }
                        //std::cout <<listC[i].host <<" " <<listC[i].flags <<" "<<fdu << " " << listC[i].port << std::endl;
                    }else{
                        FD_CLR(fdu, &wset);
                        FD_CLR(fdu, &rset);
                        --nconn;
                        closesocket(fdu);
                    }
                } else if ( (flags & READRES2) && ( FD_ISSET(fdu,&rs) || FD_ISSET(fdu,&ws) ) ){
                        //std::cout <<"ok reciver 2" << std::endl;
                    if ( checkConnnectSOCKS5packet2_s(&listCon[i], &rset,&wset)){
                        outF <<listCon[i].host <<":" << listCon[i].port <<"\n";
                        std::cout << "     socks5 "<<listCon[i].host <<":" << listCon[i].port << " ok\n";
                        FD_CLR(fdu, &wset);
                        FD_CLR(fdu, &rset);
                        listCon[i].flags = DONE;
                        closesocket(fdu);
                        --nconn;
                    }else{
                        FD_CLR(fdu, &wset);
                        FD_CLR(fdu, &rset);
                        closesocket(fdu);
                    }
                }
            }
        } else{
            rep(i,listCon.size()){
                if (listCon[i].flags != DONE)
                    {
                        closesocket(listCon[i].fd);
                        --nconn;
                    }
            }
            FD_ZERO(&rset);
            FD_ZERO(&wset);
            listCon.clear();
        }
    }
}

bool checkPortNoConfirm(int port, std::vector<std::string>listIP, std::ofstream& outF){
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);

    std::vector<GETQ> listCon;
    listCon.clear();
    std::stack<GETQ> sCon;

    fd_set rset, wset;
    FD_ZERO(&rset);
    FD_ZERO(&wset);

    int maxfd = -1;
    rep(i,listIP.size()){
        GETQ que;
        que.fd = 0;
        que.host = listIP[i];
        que.flags = 0;
        que.port = port;
        sCon.push(que);
    }

    int nconn = 0;
    while ( !sCon.empty()  || nconn > 0){
        while (!sCon.empty() && nconn < NOS_DEFAULT) {
            int fdu = -1;
            GETQ con = sCon.top();
            sCon.pop();
            fdu = sendNon_s(&con, &rset, &wset, &maxfd);
            if (fdu >= 0 ){
                listCon.pb(con);
                ++nconn;
            }
        }
        fd_set rs,ws;
        rs = rset;
        ws = wset;

        struct timeval timeout;
        timeout.tv_sec = TIMEOUT;
        timeout.tv_usec = 0;

        int n = select(maxfd + 1, &rs, &ws, NULL, &timeout);
        //std::cout << "max: " << maxfd << std::endl;
        //rep(i,listCon.size())
        //    std::cout << listCon.size()<<" " <<listCon[i].host <<" " <<listCon[i].flags << " " <<maxfd<< std::endl;
        if (n > 0){
            rep(i,listCon.size()){
                int fdu = listCon[i].fd;
                int flags = listCon[i].flags;
                //std::cout << fdu <<" " << flags << std::endl;
                if ( (flags & CONNECTING) && ( FD_ISSET(fdu,&rs) || FD_ISSET(fdu,&ws) ) ){
                        if ( connnectSOCKS5packet1_s(&listCon[i] , &rset,&wset) == INVALID_SOCKET){
                            FD_CLR(fdu, &wset);
                            FD_CLR(fdu, &rset);
                            closesocket(fdu);
                            --nconn;
                        } else{
                            //std::cout <<"ok send 1 " << listCon[i].flags <<"\n";
                        }
                } else if (flags & READRES1 && ( FD_ISSET(fdu,&rs) || FD_ISSET(fdu,&ws) ) ){
                        if ( checkConnnectSOCKS5packet1_s(&listCon[i] , &rset,&wset) ){
                            outF <<listCon[i].host <<":" << listCon[i].port <<"\n";
                            std::cout << "     socks5 "<<listCon[i].host <<":" << listCon[i].port << " ok\n";
                            FD_CLR(fdu, &wset);
                            FD_CLR(fdu, &rset);
                            listCon[i].flags = DONE;
                            closesocket(fdu);
                            --nconn;
                        }else{
                            FD_CLR(fdu, &wset);
                            FD_CLR(fdu, &rset);
                            closesocket(fdu);
                            --nconn;
                        }
                }
            }
        } else{
            rep(i,listCon.size()){
                if (listCon[i].flags != DONE)
                    {
                        closesocket(listCon[i].fd);
                        --nconn;
                    }
            }
            FD_ZERO(&rset);
            FD_ZERO(&wset);
            listCon.clear();
        }
    }
}

bool checkPort_P(std::vector<std::pair<std::string, int> > checkList, std::ofstream& outF){
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);

    std::vector<GETQ> listCon;
    listCon.clear();
    std::stack<GETQ> sCon;

    fd_set rset, wset;
    FD_ZERO(&rset);
    FD_ZERO(&wset);

    int maxfd = -1;
    rep(i,checkList.size()){
        GETQ que;
        que.fd = 0;
        que.host = checkList[i].X;
        que.flags = 0;
        que.port = checkList[i].Y;
        sCon.push(que);
    }

    int nconn = 0;
    while ( !sCon.empty()  || nconn > 0){
        while (!sCon.empty() && nconn < NOS_DEFAULT) {
            int fdu = -1;
            GETQ con = sCon.top();
            sCon.pop();
            fdu = sendNon_s(&con, &rset, &wset, &maxfd);
            if (fdu >= 0 ){
                listCon.pb(con);
                ++nconn;
            }
        }
        fd_set rs,ws;
        rs = rset;
        ws = wset;

        struct timeval timeout;
        timeout.tv_sec = TIMEOUT;
        timeout.tv_usec = 0;

        int n = select(maxfd + 1, &rs, &ws, NULL, &timeout);
        //std::cout << "max: " << maxfd << std::endl;
        //rep(i,listCon.size())
        //    std::cout << listCon.size()<<" " <<listCon[i].host <<" " <<listCon[i].flags << " " <<maxfd<< std::endl;
        if (n > 0){
            rep(i,listCon.size()){
                int fdu = listCon[i].fd;
                int flags = listCon[i].flags;
                //std::cout << fdu <<" " << flags << std::endl;
                if ( (flags & CONNECTING) && ( FD_ISSET(fdu,&rs) || FD_ISSET(fdu,&ws) ) ){
                    if ( connnectSOCKS5packet1_s(&listCon[i] , &rset,&wset) == INVALID_SOCKET){
                        FD_CLR(fdu, &wset);
                        FD_CLR(fdu, &rset);
                        --nconn;
                        closesocket(fdu);
                    }
                } else if ( (flags & READRES1) && ( FD_ISSET(fdu,&rs) || FD_ISSET(fdu,&ws) ) ){
                    if ( checkConnnectSOCKS5packet1_s(&listCon[i] , &rset,&wset) ){
                        if (connnectSOCKS5packet2_s(&listCon[i] , &rset,&wset)  == INVALID_SOCKET){
                            FD_CLR(fdu, &wset);
                            FD_CLR(fdu, &rset);
                            --nconn;
                            closesocket(fdu);
                        }
                        //std::cout <<listC[i].host <<" " <<listC[i].flags <<" "<<fdu << " " << listC[i].port << std::endl;
                    }else{
                        FD_CLR(fdu, &wset);
                        FD_CLR(fdu, &rset);
                        --nconn;
                        closesocket(fdu);
                    }
                } else if ( (flags & READRES2) && ( FD_ISSET(fdu,&rs) || FD_ISSET(fdu,&ws) ) ){
                        //std::cout <<"ok reciver 2" << std::endl;
                    if ( checkConnnectSOCKS5packet2_s(&listCon[i], &rset,&wset)){
                        outF <<listCon[i].host <<":" << listCon[i].port <<"\n";
                        std::cout << "     socks5 "<<listCon[i].host <<":" << listCon[i].port << " ok\n";
                        FD_CLR(fdu, &wset);
                        FD_CLR(fdu, &rset);
                        listCon[i].flags = DONE;
                        closesocket(fdu);
                        --nconn;
                    }else{
                        FD_CLR(fdu, &wset);
                        FD_CLR(fdu, &rset);
                        closesocket(fdu);
                    }
                }
            }
        } else{
            rep(i,listCon.size()){
                if (listCon[i].flags != DONE)
                    {
                        closesocket(listCon[i].fd);
                        --nconn;
                    }
            }
            FD_ZERO(&rset);
            FD_ZERO(&wset);
            listCon.clear();
        }
    }
}
bool checkPortNoConfirm_P(std::vector<std::pair<std::string, int> > checkList, std::ofstream& outF){
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);

    std::vector<GETQ> listCon;
    listCon.clear();
    std::stack<GETQ> sCon;

    fd_set rset, wset;
    FD_ZERO(&rset);
    FD_ZERO(&wset);

    int maxfd = -1;
    rep(i,checkList.size()){
        GETQ que;
        que.fd = 0;
        que.host = checkList[i].X;
        que.flags = 0;
        que.port = checkList[i].Y;
        sCon.push(que);
    }

    int nconn = 0;
    while ( !sCon.empty()  || nconn > 0){
        while (!sCon.empty() && nconn < NOS_DEFAULT) {
            int fdu = -1;
            GETQ con = sCon.top();
            sCon.pop();
            fdu = sendNon_s(&con, &rset, &wset, &maxfd);
            if (fdu >= 0 ){
                listCon.pb(con);
                ++nconn;
            }
        }
        fd_set rs,ws;
        rs = rset;
        ws = wset;

        struct timeval timeout;
        timeout.tv_sec = TIMEOUT;
        timeout.tv_usec = 0;

        int n = select(maxfd + 1, &rs, &ws, NULL, &timeout);
        //std::cout << "max: " << maxfd << std::endl;
        //rep(i,listCon.size())
        //    std::cout << listCon.size()<<" " <<listCon[i].host <<" " <<listCon[i].flags << " " <<maxfd<< std::endl;
        if (n > 0){
            rep(i,listCon.size()){
                int fdu = listCon[i].fd;
                int flags = listCon[i].flags;
                //std::cout << fdu <<" " << flags << std::endl;
                if ( (flags & CONNECTING) && ( FD_ISSET(fdu,&rs) || FD_ISSET(fdu,&ws) ) ){
                        if ( connnectSOCKS5packet1_s(&listCon[i] , &rset,&wset) == INVALID_SOCKET){
                            FD_CLR(fdu, &wset);
                            FD_CLR(fdu, &rset);
                            closesocket(fdu);
                            --nconn;
                        } else{
                            //std::cout <<"ok send 1 " << listCon[i].flags <<"\n";
                        }
                } else if (flags & READRES1 && ( FD_ISSET(fdu,&rs) || FD_ISSET(fdu,&ws) ) ){
                        if ( checkConnnectSOCKS5packet1_s(&listCon[i] , &rset,&wset) ){
                            outF <<listCon[i].host <<":" << listCon[i].port <<"\n";
                            std::cout << "     socks5 "<<listCon[i].host <<":" << listCon[i].port << " ok\n";
                            FD_CLR(fdu, &wset);
                            FD_CLR(fdu, &rset);
                            listCon[i].flags = DONE;
                            closesocket(fdu);
                            --nconn;
                        }else{
                            FD_CLR(fdu, &wset);
                            FD_CLR(fdu, &rset);
                            closesocket(fdu);
                            --nconn;
                        }
                }
            }
        } else{
            rep(i,listCon.size()){
                if (listCon[i].flags != DONE)
                    {
                        closesocket(listCon[i].fd);
                        --nconn;
                    }
            }
            //FD_ZERO(&rset);
            //FD_ZERO(&wset);
            listCon.clear();
        }
    }
}
