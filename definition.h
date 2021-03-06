#ifndef DEFINITION_H_INCLUDED
#define DEFINITION_H_INCLUDED

#include <iostream>
#include <regex>
#include <string>
#include <algorithm>
#include <stdio.h>
#pragma comment(lib,"Ws2_32.lib")
#include<winsock2.h>

#include <fstream>

#include <windows.h>
#include <winsock.h>
#include <sys/stat.h>
#include <io.h>
#include <conio.h>
#include <fcntl.h>
#include <chrono>
#include <stack>
#include <thread>
#include "CSocks.h"

#define rep(i,n) for(int i = 0; i < n; ++i)
#define FOR(i,a,b) for(int i = a; i <= b; ++i)
#define pb push_back

#define CONNECTING 1
#define READRES1 2
#define READRES2 4
#define DONE 8

#define NOS_DEFAULT 500
#define TIMEOUT 2
#define X first
#define Y second


struct GETQ{
    std::string host;
    int port;
    int fd;
    int flags;
};

struct DestInfo{
    std::string IPAddr   = "";
    std::string hostname = "";
    u_short port         = 80;
};

std::vector <int> getListPort();
bool checklineRangePort(std::string line);

void  getListIp(std::string namefile,std::vector<std::string> *listIP, std::vector< std::pair<std::string, std::string> > *listRange);

//ip handle
uint32_t ip_to_int (const char * ip);
void int_to_ip(uint32_t ip, char * addr);

bool checkip(std::string ip, std::vector<int> listPort, std::ofstream& outF);
bool checkip_s(std::string ip, std::vector<int> listPort, std::ofstream& outF );
bool checkipNoConfirm(std::string ip, std::vector<int> listPort, std::ofstream& outF );
bool checkipNoConfirm_s(std::string ip, std::vector<int> listPort, std::ofstream& outF );

bool checkPort(int port, std::vector<std::string>listIP, std::ofstream& resultFile);
bool checkPortNoConfirm(int port, std::vector<std::string>listIP, std::ofstream& resultFile);

bool checkPort_P(std::vector< std::pair<std::string, int> > checkList, std::ofstream& resultFile, int nos, int timeout);
bool checkPortNoConfirm_P(std::vector<std::pair<std::string, int> > checkList, std::ofstream& resultFile, int nos, int timeout);

void readConfig(std::string fileconfig, int * rescan, std::string * savesocks,std::string * portranger, std::string * url, int * timeout, int *thread);

#endif // DEFINITION_H_INCLUDED
