#include "definition.h"

using namespace std;

string  savesocks, portRange, URLcheck;
int rescan, threadnum, timeoutURL;
vector<int> listPort;
vector<string> listIP;
vector<pair<string,string> > listRange;


ofstream resultFile;

int scan(string host, int port){
    CSocks sock(host, port, 5);
    sock.setDestination("google.com", 80);

    SOCKET hSock = sock.Connect();

    if(hSock == INVALID_SOCKET)
          {
              cout << "ERROR: " << sock.getLastError();
              return -1;
          }

          std::string headers = "GET / HTTP/1.1\r\n\r\n";

          //Send our request
          cout << endl << "Sending custom request...";
          if(sock.sendData(hSock, headers.c_str(), headers.length()) < 0)
          {
              cout << "failed";
              return -1;
          }
          cout << "done!" << endl;

          std::string fullResp = "";
          char buffer[4096];

          cout << "Reading response from server...\n";
          int d = 0;
          while(true)
          {
              int retval = recv(hSock, buffer, sizeof(buffer), 0);
              cout << (retval) << endl;
              if(retval == 0)
              {
                  break;
              }
              else if(retval == SOCKET_ERROR)
              {
                  cout << "failed";
                  return -1;
              }
              else
              {
                  fullResp +=  buffer;
              }
                d++;
              if (d==2)
                break;
              //break;
          }
          cout << "done" << endl;
          cout << "What we have got:" << endl << fullResp;
          std::regex ex("HTTP/1.1 200 OK");
          //if (regex_match(fullResp,ex))
          if (fullResp.find("HTTP/1.1 200 OK") != std::string::npos)
                cout << "OK" << endl;
          closesocket(hSock);
          shutdown(hSock, 2);

}

int main()
{
    readConfig("files/config.txt",&rescan,&savesocks,&portRange,&URLcheck, &timeoutURL, &threadnum);
    listPort = getListPort();
    getListIp("files/ip.txt", &listIP, &listRange);
    rep(i, listRange.size()){
        uint32_t be, en;
        string sbe, sen;
        sbe = listRange[i].first;
        sen = listRange[i].second;
        be = ip_to_int(sbe.c_str());
        en = ip_to_int(sen.c_str());
        for(uint32_t ip = be; ip <= en; ++ip){
            char ipc[20];
            int_to_ip(ip,ipc);
            //cout << ipc << endl;
            string s(ipc);
            listIP.pb(s);
        }
    }

    cout << "___________CONFIG SOCKS5 SCAN___________" <<endl;
    cout <<"- Time to rescan: " << rescan <<" minutes" <<endl;
    cout <<"- Result file: " << savesocks << endl;
    cout <<"- Port file: " << portRange << endl;
    cout <<"- URL to check: " << URLcheck << endl;
    cout <<"- Time to check url: " << timeoutURL<< endl;
    cout <<"- Num of thread: " << threadnum << endl;
    cout <<"- Num of port: " << listPort.size() << endl;
    cout <<"- Num if IP: " << listIP.size() << endl;
    cout << "________________________________________\n" << endl;

    //uncomment to save log file
    //freopen("log.txt","w",stdout);

    int time = 1;
    while(1)
    {
        std::stringstream sstm;
        sstm << time;
        string fileres ="result/" + sstm.str() + savesocks;
        resultFile.open(fileres.c_str());
        cout << "+ " << fileres <<endl << endl;
/*
        rep(i,listIP.size()){
            auto start = std::chrono::system_clock::now();
            cout << "IP: " << listIP[i] << " scanning............" << endl;
            if (timeoutURL > 0){
                checkip(listIP[i],listPort,resultFile);
                //checkip_s(listIP[i],listPort,resultFile);
            }
            else{
                checkipNoConfirm(listIP[i],listPort,resultFile);
                //checkipNoConfirm_s(listIP[i],listPort,resultFile);
            }
            auto end = std::chrono::system_clock::now();
            std::chrono::duration<double> elapsed_seconds = end-start;
            std::time_t end_time = std::chrono::system_clock::to_time_t(end);
            std::cout << "     Time: " << elapsed_seconds.count() << " (s)\n";
        }
*/
        rep(i,listPort.size()){
            auto start = std::chrono::system_clock::now();
            //scan ip per port
            cout << "Port: " << listPort[i] << " scanning............" << endl;
            if (timeoutURL > 0){
                checkPort(listPort[i],listIP,resultFile);
                //checkip_s(listIP[i],listPort,resultFile);
            }
            else{
                checkPortNoConfirm(listPort[i],listIP,resultFile);
                //checkipNoConfirm_s(listIP[i],listPort,resultFile);
            }
            auto end = std::chrono::system_clock::now();
            std::chrono::duration<double> elapsed_seconds = end-start;
            std::time_t end_time = std::chrono::system_clock::to_time_t(end);
            std::cout << "     Time: " << elapsed_seconds.count() << " (s)\n";
        }
        ++time;
        resultFile.close();

        cout << endl<<"DONE\n" << "__________________________________________________________________" <<endl;
        Sleep(1000 * 60 * rescan);

    }
    //resultFile.close();
    //scan("37.220.31.34 ", 46234);
    return 0;
}
