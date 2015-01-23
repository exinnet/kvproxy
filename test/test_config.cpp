#include <config.h>
#include <set>
#include <iostream>
#include <string>

using namespace std;

int main(){
    map<string, string> m;
    map<string, string>::iterator it;
    Config conf;
    conf.setConfFile("../etc/kvproxy.ini");
    conf.loadConf();
    m = conf.getConfStr("master");
    for(it = m.begin(); it != m.end(); it++){
        cout << it->first << " -- " << it->second << endl;
    }
}
