#include "definition.h"

int connNon(std::string host, int port){
    int n, soc, flags;

    sockaddr_in socks;
    socks.sin_family = AF_INET;                                 // host byte order
    socks.sin_port = htons(port);                               // short, network byte order
    socks.sin_addr.S_un.S_addr = inet_addr(host.c_str());       // adding socks server IP address into structure

    soc = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    struct timeval timeout;
    timeout.tv_sec = 2;
    timeout.tv_usec = 0;

    u_long iMode = 1;

    int iResult = ioctlsocket(soc, FIONBIO, &iMode);

    if (setsockopt (soc, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,sizeof(timeout)) < 0)
        printf("setsockopt failed\n");

    if (setsockopt (soc, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout,sizeof(timeout)) < 0)
        printf("setsockopt failed\n");

    if (connect(soc, reinterpret_cast<sockaddr *>(&socks), sizeof(socks)) != 0)
    {
        closesocket(soc);
        return INVALID_SOCKET;
    }

    return -1;
}

int sendData(SOCKET s, const void *buffer, int buflen)
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

int recvData(SOCKET s, void *buffer, int buflen)
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


SOCKET connnectSOCKS5packet1(GETQ * get,fd_set * rset, fd_set * wset){
    int initPacket1Length = 3;
    char initPacket1[initPacket1Length];
    initPacket1[0] = 5;
    initPacket1[1] = 1;
    initPacket1[2] = 0;

    if (sendData(get->fd, initPacket1, initPacket1Length) < 0){
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

bool checkConnnectSOCKS5packet1(GETQ * get, fd_set * rset, fd_set * wset){
    char reply1[2];
    //recv(get->fd, reply1, 2, 0);

    if (recvData(get->fd, reply1, 2) <= 0)
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

SOCKET connnectSOCKS5packet2(GETQ * get, fd_set * rset, fd_set * wset){
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
    if (sendData(get->fd, initPacket2, 7+hostlen) < 0)
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

bool checkConnnectSOCKS5packet2(GETQ * get , fd_set * rset, fd_set * wset) {
    char reply2[262];
    if (recvData(get->fd, reply2, 4) <= 0)
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

int sendNon(GETQ *get, fd_set * rset, fd_set * wset, int * maxfd){
    int n, flags;

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;                                          // host byte order
    addr.sin_port = htons(get->port);                                   // short, network byte order
    addr.sin_addr.S_un.S_addr = inet_addr(get->host.c_str());           // adding socks server IP address into structure

    SOCKET sendfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

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
        //std::cout <<"max : " << *maxfd <<std::endl;
        if (sendfd > * maxfd )
            *maxfd = sendfd;
        return sendfd;
    } else{
        //closesocket(sendfd);
        connnectSOCKS5packet1(get, rset,wset);
        //return -1;
    }
}


bool checkip(std::string ip, std::vector<int> listPort, std::ofstream& outF ){
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    /*
    if (iResult != NO_ERROR)
         printf("Client: Error at WSAStartup().\n");
    else
         printf("Client: WSAStartup() is OK.\n");
    */
    int pPos = 0;
    std::vector<GETQ> listQ, listC;
    listQ.clear();
    listC.clear();
    fd_set rset, wset;

    FD_ZERO(&rset);
    FD_ZERO(&wset);

    int maxfd = -1;
    int total;
    rep(i,listPort.size()){ //create list to check
        GETQ que;
        que.fd = 0;
        que.host = ip;
        que.flags = 0;
        que.port = listPort[i];
        listQ.push_back(que);
    }
    //init num of process to do.
    int nlefttoread, nlefttoconn, nconn;
    nlefttoconn = nlefttoread = total = listQ.size();
    nconn = 0;

    while (nlefttoread > 0){
        //listC.clear();
        while(nconn < NOS_DEFAULT && nlefttoconn > 0){
            int index = -1;
            int fdu = -1;
            rep(i,total)
                if (listQ[i].flags == 0)
                {
                    index = i;
                    break;
                }
            //std::cout <<"index: "<< index <<" "<< listQ[index].port << std::endl;
            if (index >= 0){
                fdu = sendNon(&listQ[index],&rset, &wset, &maxfd); //connect to host and return fd of connect

                //std::cout << "conn: "<<listQ[index].host <<" " << fdu << " port: " <<listQ[index].port << " " << listQ[index].flags <<std::endl;
                if (fdu > 0 ){ //if create socket success. add to fd_set and update maxfd
                    //std::cout << "conn: "<<listQ[index].host <<" " << fdu << " port: " <<listQ[index].port << " " << listQ[index].flags <<std::endl;
                    listC.push_back(listQ[index]);
                } else{
                    --nlefttoread;
                }
                listQ[index].flags = DONE;
                ++nconn;
                --nlefttoconn;
            }
        }

        fd_set rs,ws;
        rs = rset;
        ws = wset;

        struct timeval timeout;
        timeout.tv_sec = TIMEOUT;
        timeout.tv_usec = 0;

        int n = select(maxfd + 1, &rs, &ws, NULL, &timeout);
        //std::cout << "check " << listC[0].port << " " <<n << std::endl;
        if (n > 0){

            //std::cout << "OK" << n <<std::endl;
            rep(i,listC.size()) {
                int fdu = listC[i].fd;
                int flags = listC[i].flags;
                //std::cout <<listC[i].host <<" " <<listC[i].flags <<" "<<fdu << " " << listC[i].port << std::endl;
                if ( (flags & CONNECTING) && ( FD_ISSET(fdu,&rs) || FD_ISSET(fdu,&ws) ) ){
                    if ( connnectSOCKS5packet1(&listC[i] , &rset,&wset) == INVALID_SOCKET){
                        FD_CLR(fdu, &wset);
                        FD_CLR(fdu, &rset);
                        closesocket(fdu);
                    }
                } else if ( (flags & READRES1) && ( FD_ISSET(fdu,&rs) || FD_ISSET(fdu,&ws) ) ){
                    if ( checkConnnectSOCKS5packet1(&listC[i] , &rset,&wset) ){
                        if (connnectSOCKS5packet2(&listC[i] , &rset,&wset)  == INVALID_SOCKET){
                            FD_CLR(fdu, &wset);
                            FD_CLR(fdu, &rset);
                            closesocket(fdu);
                        }
                        //std::cout <<listC[i].host <<" " <<listC[i].flags <<" "<<fdu << " " << listC[i].port << std::endl;
                    }else{
                        FD_CLR(fdu, &wset);
                        FD_CLR(fdu, &rset);
                        closesocket(fdu);
                    }
                } else if ( (flags & READRES2) && ( FD_ISSET(fdu,&rs) || FD_ISSET(fdu,&ws) ) ){
                        //std::cout <<"ok reciver 2" << std::endl;
                    if ( checkConnnectSOCKS5packet2(&listC[i], &rset,&wset)){
                        outF << ip <<":" << listC[i].port << "\n";
                        std::cout << "     socks5 "<<ip <<":" << listC[i].port << "\n";
                        FD_CLR(fdu, &wset);
                        FD_CLR(fdu, &rset);
                        listC[i].flags = DONE;
                        closesocket(fdu);
                        --nlefttoread;
                        --nconn;
                    }else{
                        FD_CLR(fdu, &wset);
                        FD_CLR(fdu, &rset);
                        closesocket(fdu);
                    }
                }
            }
        } else{
            //std::cout<<listC.size() << std::endl;
            rep(i,listC.size()){
                //std::cout <<listC[i].flags <<std::endl;
                if (listC[i].flags != DONE)
                    {
                        closesocket(listC[i].fd);
                        --nlefttoread;
                        --nconn;
                    }

            }
            listC.clear();
            //std::cout <<"NO " << nconn <<" "<<nlefttoread<<std::endl;
            //break;
        }

    }
}

bool checkip_s(std::string ip, std::vector<int> listPort, std::ofstream& outF ){
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    /*
    if (iResult != NO_ERROR)
         printf("Client: Error at WSAStartup().\n");
    else
         printf("Client: WSAStartup() is OK.\n");
    */
    std::vector<GETQ> listC;
    listC.clear();
    fd_set rset, wset;

    FD_ZERO(&rset);
    FD_ZERO(&wset);

    int maxfd = -1;
    int total;
    std::stack<GETQ> sCon;
    rep(i,listPort.size()){ //create list to check
        GETQ que;
        que.fd = 0;
        que.host = ip;
        que.flags = 0;
        que.port = listPort[i];
        sCon.push(que);
    }
    //init num of process to do.
    int nlefttoread, nlefttoconn, nconn;
    nlefttoconn = nlefttoread = total = listPort.size();
    nconn = 0;

    while (!sCon.empty()){
        //listC.clear();
        while(nconn < NOS_DEFAULT && !sCon.empty() ){
            int fdu = -1;
            GETQ con = sCon.top();
            sCon.pop();
            fdu = sendNon(&con, &rset, &wset, &maxfd); //connect to host and return fd of connect
            if (fdu > 0 ){ //if create socket success. add to fd_set and update maxfd
                listC.push_back(con);
            } else{
                --nlefttoread;
            }
            ++nconn;
        }

        fd_set rs,ws;
        rs = rset;
        ws = wset;

        struct timeval timeout;
        timeout.tv_sec = TIMEOUT;
        timeout.tv_usec = 0;

        int n = select(maxfd + 1, &rs, &ws, NULL, &timeout);
        //std::cout << "check " << listC[0].port << " " <<n << std::endl;
        if (n > 0){

            //std::cout << "OK" << n <<std::endl;
            rep(i,listC.size()) {
                int fdu = listC[i].fd;
                int flags = listC[i].flags;
                //std::cout <<listC[i].host <<" " <<listC[i].flags <<" "<<fdu << " " << listC[i].port << std::endl;
                if ( (flags & CONNECTING) && ( FD_ISSET(fdu,&rs) || FD_ISSET(fdu,&ws) ) ){
                    if ( connnectSOCKS5packet1(&listC[i] , &rset,&wset) == INVALID_SOCKET){
                        FD_CLR(fdu, &wset);
                        FD_CLR(fdu, &rset);
                        closesocket(fdu);
                    }
                } else if ( (flags & READRES1) && ( FD_ISSET(fdu,&rs) || FD_ISSET(fdu,&ws) ) ){
                    if ( checkConnnectSOCKS5packet1(&listC[i] , &rset,&wset) ){
                        if (connnectSOCKS5packet2(&listC[i] , &rset,&wset)  == INVALID_SOCKET){
                            FD_CLR(fdu, &wset);
                            FD_CLR(fdu, &rset);
                            closesocket(fdu);
                        }
                        //std::cout <<listC[i].host <<" " <<listC[i].flags <<" "<<fdu << " " << listC[i].port << std::endl;
                    }else{
                        FD_CLR(fdu, &wset);
                        FD_CLR(fdu, &rset);
                        closesocket(fdu);
                    }
                } else if ( (flags & READRES2) && ( FD_ISSET(fdu,&rs) || FD_ISSET(fdu,&ws) ) ){
                        //std::cout <<"ok reciver 2" << std::endl;
                    if ( checkConnnectSOCKS5packet2(&listC[i], &rset,&wset)){
                        outF << ip <<":" << listC[i].port << "\n";
                        std::cout << "     socks5 "<<ip <<":" << listC[i].port << "\n";
                        FD_CLR(fdu, &wset);
                        FD_CLR(fdu, &rset);
                        listC[i].flags = DONE;
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
            //std::cout<<listC.size() << std::endl;
            rep(i,listC.size()){
                //std::cout <<listC[i].flags <<std::endl;
                if (listC[i].flags != DONE)
                    {
                        closesocket(listC[i].fd);
                        --nconn;
                    }

            }
            listC.clear();
            //std::cout <<"NO " << nconn <<" "<<nlefttoread<<std::endl;
            //break;
        }

    }
}


bool checkipNoConfirm(std::string ip, std::vector<int> listPort, std::ofstream& outF ){
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);

    int pPos = 0;
    std::vector<GETQ> listQ, listC;
    listQ.clear();
    listC.clear();
    fd_set rset, wset;

    FD_ZERO(&rset);
    FD_ZERO(&wset);

    int maxfd = -1;
    int total;
    rep(i,listPort.size()){
        GETQ que;
        que.fd = 0;
        que.host = ip;
        que.flags = 0;
        que.port = listPort[i];
        listQ.push_back(que);
    }

    int nlefttoread, nlefttoconn, nconn;
    nlefttoconn = nlefttoread = total = listQ.size();
    nconn = 0;
    while (nlefttoread > 0){
        while(nconn < NOS_DEFAULT && nlefttoconn > 0){
            int index = -1;
            int fdu = -1;
            rep(i,total)
                if (listQ[i].flags == 0)
                {
                    index = i;
                    break;
                }
            fdu = sendNon(&listQ[index],&rset, &wset, &maxfd); //connect to host and return fd of connect

            //std::cout << "conn: "<<listQ[index].host <<" " << fdu << " port: " <<listQ[index].port << " " << listQ[index].flags <<std::endl;
            if (fdu > 0 ){ //if create socket success. add to fd_set and update maxfd
                //std::cout << "conn: "<<listQ[index].host <<" " << fdu << " port: " <<listQ[index].port << " " << listQ[index].flags <<std::endl;
                listC.push_back(listQ[index]);
            } else{
                --nlefttoread;
            }
            listQ[index].flags = DONE;
            ++nconn;
            --nlefttoconn;
        }
        //std::cout<< listC[listC.size()-1].port <<"\n";
        fd_set rs,ws;
        rs = rset;
        ws = wset;

        struct timeval timeout;
        timeout.tv_sec = TIMEOUT;
        timeout.tv_usec = 0;

        int n = select(maxfd + 1, &rs, &ws, NULL, &timeout);

        if (n > 0){
            //std::cout << "OK" << n <<std::endl;
            rep(i,listC.size()) {
                int fdu = listC[i].fd;
                int flags = listC[i].flags;
                //std::cout <<listC[i].host <<" " <<listC[i].flags <<" "<<fdu << " " << listC[i].port << std::endl;
                if ( (flags & CONNECTING) && ( FD_ISSET(fdu,&rs) || FD_ISSET(fdu,&ws) ) ){
                    if ( connnectSOCKS5packet1(&listC[i] , &rset,&wset) == INVALID_SOCKET){
                        FD_CLR(fdu, &wset);
                        FD_CLR(fdu, &rset);
                        closesocket(fdu);
                    }
                } else if ( (flags & READRES1) && ( FD_ISSET(fdu,&rs) || FD_ISSET(fdu,&ws) ) ){
                    if ( checkConnnectSOCKS5packet1(&listC[i] , &rset,&wset) ){
                        outF <<ip <<":" << listC[i].port <<"\n";
                        std::cout << " socks5 "<<ip <<":" << listC[i].port << " ok\n";
                        FD_CLR(fdu, &wset);
                        FD_CLR(fdu, &rset);
                        listC[i].flags = DONE;
                        closesocket(fdu);
                        --nlefttoread;
                        --nconn;
                        //std::cout <<listC[i].host <<" " <<listC[i].flags <<" "<<fdu << " " << listC[i].port << std::endl;
                    }else{
                        FD_CLR(fdu, &wset);
                        FD_CLR(fdu, &rset);
                        closesocket(fdu);
                    }
                }
            }
        } else{
            //std::cout<<listC.size() << std::endl;
            rep(i,listC.size()){
                //std::cout <<listC[i].flags <<std::endl;
                if (listC[i].flags != DONE)
                    {
                        closesocket(listC[i].fd);
                        --nlefttoread;
                        --nconn;
                    }

            }
            FD_ZERO(&rset);
            FD_ZERO(&wset);
            listC.clear();
            //std::cout <<"NO " << nconn <<" "<<nlefttoread<<std::endl;
        }
    }
}

bool checkipNoConfirm_s(std::string ip, std::vector<int> listPort, std::ofstream& outF ){
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);

    std::vector<GETQ>  listC;
    listC.clear();

    fd_set rset, wset;
    FD_ZERO(&rset);
    FD_ZERO(&wset);

    std::stack<GETQ> sCon;
    int maxfd = -1;
    int total;
    rep(i,listPort.size()){
        GETQ que;
        que.fd = 0;
        que.host = ip;
        que.flags = 0;
        que.port = listPort[i];
        sCon.push(que);
    }

    std::cout << sCon.size() << std::endl;

    int nconn;
    total = listPort.size();
    nconn = 0;
    while ( !sCon.empty() ){
        while( nconn < NOS_DEFAULT && !sCon.empty() ){
            int fdu = -1;
            GETQ con;
            con = sCon.top();
            sCon.pop();
            fdu = sendNon(&con, &rset, &wset, &maxfd); //connect to host and return fd of connect
            if (fdu > 0 ){ //if create socket success. add to fd_set and update maxfd
                listC.push_back(con);
                //std::cout <<con.flags << std::endl;
            } else{
                //--nlefttoread;
            }
            ++nconn;
        }
        //std::cout << sCon.size() << std::endl;
        fd_set rs,ws;
        rs = rset;
        ws = wset;

        struct timeval timeout;
        timeout.tv_sec = TIMEOUT;
        timeout.tv_usec = 0;
        std::cout << maxfd << std::endl;
        int n = select(maxfd + 1, &rs, &ws, NULL, &timeout);

        if (n > 0){
            rep(i,listC.size()) {
                int fdu = listC[i].fd;
                int flags = listC[i].flags;
                if ( (flags & CONNECTING) && ( FD_ISSET(fdu,&rs) || FD_ISSET(fdu,&ws) ) ){
                    if ( connnectSOCKS5packet1(&listC[i] , &rset,&wset) == INVALID_SOCKET){
                        FD_CLR(fdu, &wset);
                        FD_CLR(fdu, &rset);
                        closesocket(fdu);
                    }
                } else if ( (flags & READRES1) && ( FD_ISSET(fdu,&rs) || FD_ISSET(fdu,&ws) ) ){
                    if ( checkConnnectSOCKS5packet1(&listC[i] , &rset,&wset) ) {
                        outF <<ip <<":" << listC[i].port <<"\n";
                        std::cout << "     socks5 "<<ip <<":" << listC[i].port << " ok\n";
                        FD_CLR(fdu, &wset);
                        FD_CLR(fdu, &rset);
                        listC[i].flags = DONE;
                        closesocket(fdu);
                    }else{
                        FD_CLR(fdu, &wset);
                        FD_CLR(fdu, &rset);
                        closesocket(fdu);
                    }
                }
            }
        } else{
            /*
            rep(i,listC.size()){
                if (listC[i].flags != DONE)
                    {
                        --nconn;
                    }
            }
            */
            nconn = 0;
            listC.clear();
        }
    }
}

