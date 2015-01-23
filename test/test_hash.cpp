#include <hash.h>
#include <set>
#include <iostream>
#include <string>

using namespace std;

int main(){
    /*
    set<string> s;
    string key;
    pair<string, uint32_t> p;
    ConsistentHash hash;
    char port[10];
    s.insert("192.168.1.1:11211");
    s.insert("192.168.1.2:11211");
    s.insert("192.168.1.3:11211");
    s.insert("192.168.1.4:11211");
    hash.setHosts(s);
    key = "a";
    p = hash.find(key);
    sprintf(port,"%d",p.second);
    cout << "host " << p.first << "port " << port << endl;
    */
    map<pair<string,uint32_t>, pair<uint32_t,uint32_t> > m;
    map<uint32_t, pair<string,uint32_t> > h;
    map<uint32_t, pair<string,uint32_t> >::iterator it;
    pair<string,uint32_t> p;
    m[make_pair("192.168.1.1",11211)] = make_pair(1,3);
    m[make_pair("192.168.1.2",11212)] = make_pair(2,2);
    m[make_pair("192.168.1.3",11213)] = make_pair(3,4);
    ConsistentHash hash;
    hash.setHosts(m);
    h = hash.getAll();
    for(it = h.begin(); it != h.end(); it++){
        cout << it->first << " -- " << it->second.first << ":" << it->second.second << endl;
    }
    p = hash.find("123");
    cout << p.first << " -- " << p.second << endl;
    cout << "hash size is " << hash.getSize() << endl;
}
