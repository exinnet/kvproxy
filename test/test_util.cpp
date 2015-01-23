#include "util.h"
using namespace std;

int main(){
    cout << "start func split " << endl;
    string str;
    str = "abc:def:123";
    map<int, string> m;
    map<int, string>::iterator m_it;
    m = split(str, ':');
    for(m_it = m.begin(); m_it != m.end(); m_it++){
        cout << m_it->first  << " -- " << m_it->second << endl;
    }
    cout << "end func split " << endl;
    return 0;
}
