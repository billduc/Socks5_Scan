#include "definition.h"

void readConfig(std::string fileconfig, int * rescan, std::string * savesocks,std::string * portranger, std::string * url, int * timeout, int *thread){
    std::ifstream file;
    file.open(fileconfig.c_str(),std::ifstream::in);
    //char s[123];
    int i = 0;
    if (file.is_open()){
        while (!file.eof()){
            std::string s;
            file >> s;
            if (i==0){
                while (s[0] <'A' || s[0] > 'Z'){
                    s.erase(s.begin());
                }
            }
            //std::cout << s << std::endl;
            if (s.compare("Rescan") == 0)
            {
                char c;
                file >> c;
                int re;
                file >> re;
                *rescan = re;
            }

            if (s.compare("savesocks") == 0)
            {
                char c;
                file >> c;
                std::string re;
                file >> re;
                *savesocks = re;
            }

            if (s.compare("portranger") == 0)
            {
                char c;
                file >> c;
                std::string re;
                file >> re;
                *portranger = re;
            }

            if (s.compare("URLcheck") == 0)
            {
                char c;
                file >> c;
                std::string re;
                file >> re;
                *url = re;
            }

            if (s.compare("RequestURLcheck") == 0)
            {
                char c;
                file >> c;
                int re;
                file >> re;
                *timeout = re;
            }

            if (s.compare("Threads") == 0)
            {
                char c;
                file >> c;
                int re;
                file >> re;
                *thread = re;
            }
            i++;
        }
        file.close();
    }
    return;
}
