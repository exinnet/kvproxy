#ifndef KVPROXY_UTIL_H
#define KVPROXY_UTIL_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/syscall.h> 
#include <iostream>
#include <string>
#include <map>
#include <sys/time.h>
#include "config.h"

using namespace std;


string get_cwd(string bin_path);
inline int str2int(const char * str){
    int n;
    sscanf(str,"%d",&n);
    return n;
}
inline string int2str(int n){
    char str[20] = {'\0'};
    sprintf(str,"%d",n);
    return str;
}

inline string bool2str(bool n){
    if(n == true){
        return "yes";
    }else{
        return "no";
    }
}

inline map<int, string> split(string content, char chr){
    map<int, string> ret;
    uint32_t i = 0; 
    for(string::iterator it = content.begin(); it != content.end(); it++){
        if(*it != chr){
            ret[i] += *it;
        }else{
            i++;    
        }
    }
    return ret;
}

inline int get_second(){
    struct  timeval    tv;
    struct  timezone   tz;
    gettimeofday(&tv,&tz);
    return tv.tv_sec;
}

#endif
