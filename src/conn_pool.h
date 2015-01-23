#ifndef KVPROXY_CONN_POOL_H
#define KVPROXY_CONN_POOL_H

#include <unistd.h>
#include <string.h>
#include <iostream>
#include <string>
#include <map>
#include <list>
#include <pthread.h>
#include <sys/socket.h>
#include <errno.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <pthread.h>
#include "log.h"

using namespace std;

class ConnPool {
private:
    
    map<uint32_t,map<string, map<uint32_t,int> > > conn_list;
    static ConnPool * conn_pool;
    pthread_rwlock_t conn_list_lock;

    bool destoryConnPool();
public:
    ConnPool(){
        int res;
        res=pthread_rwlock_init(&conn_list_lock,NULL);
        if(res != 0){
            cerr << "[error]:rwlock initialization failed" << endl;
            log_fatal("rwlock initialization failed ");
            exit(EXIT_FAILURE);
        }
    };
    ~ConnPool();
    static ConnPool * getInstance();
    void initConnection(string host, uint32_t port, uint32_t size);
    int getConnection(uint32_t id, string host, uint32_t port);
    int createConnection(string host, uint32_t port);
    void destoryConnection(int conn);
    void closeConnection(int conn);
    int getConnectionSize();
};

#endif
