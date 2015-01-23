#include "conn_pool.h"

ConnPool * ConnPool::conn_pool;
 
ConnPool * ConnPool::getInstance() {
    if (conn_pool == NULL) {
        conn_pool = new ConnPool();
    }
    return conn_pool;
}
 
void ConnPool::initConnection(string host, uint32_t port, uint32_t size) {
    int conn;
    string key;
    uint32_t id;
    for(id = 0; id < size; id++){
        conn = this->createConnection(host, port);
        if (conn > 0) {
            conn_list[id][host][port] = conn;
        }
    }
}
 
int ConnPool::createConnection(string host, uint32_t port) {
    int sock_fd = 0;
    int optval = 1;
    int optlen = 0;
    struct sockaddr_in server_addr;
    socklen_t socklen = sizeof(server_addr);
    if((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0){  
        log_error("create socket error. %s. host %s, port %d\n", strerror(errno), host.c_str(), port);
        return -1;
    }
    optlen = sizeof(optval);
    if(setsockopt(sock_fd, SOL_SOCKET, SO_KEEPALIVE, &optval, optlen) < 0) {
        log_error("fail to set keep alive. %s. host %s, port %d\n", strerror(errno), host.c_str(), port);
    }
    bzero(&server_addr, sizeof(server_addr));  
    server_addr.sin_family = AF_INET;  
    inet_aton(host.c_str(), &server_addr.sin_addr);  
    server_addr.sin_port = htons(port);  
    if(connect(sock_fd, (struct sockaddr*)&server_addr, socklen) < 0){  
        return -1;
    }
    return sock_fd;
}
 
int ConnPool::getConnection(uint32_t id, string host, uint32_t port) {
    int conn;
    int res;
    
    res = pthread_rwlock_rdlock(&conn_list_lock);
    if(res != 0){
        log_error("pthread_wrlock_rdlock");
        return -1;
    }
    if (conn_list[id][host][port] > 0) {   //连接池容器中还有连接
        conn = conn_list[id][host][port];
        pthread_rwlock_unlock(&conn_list_lock);
    }else{
        pthread_rwlock_unlock(&conn_list_lock);
        res = pthread_rwlock_wrlock(&conn_list_lock);
        if(res != 0){
            log_error("pthread_rwlock_wrlock fail");
            return -1;
        }
        conn = this->createConnection(host, port);
        conn_list[id][host][port] = conn;
        pthread_rwlock_unlock(&conn_list_lock);
    }
    return conn;
}
 
 
ConnPool::~ConnPool() {
    this->destoryConnPool();
}
 
bool ConnPool::destoryConnPool() {
    map<uint32_t,map<string, map<uint32_t,int> > >::iterator it;
    map<string, map<uint32_t,int> >::iterator it_a;
    map<uint32_t,int>::iterator it_b;
    int res;

    res = pthread_rwlock_wrlock(&conn_list_lock);
    if(res != 0){
        log_error("pthread_rwlock_wrlock fail");
    }
    for (it = conn_list.begin(); it != conn_list.end(); ++it){
        for(it_a = it->second.begin(); it_a != it->second.end(); it_a++){
            for(it_b = it_a->second.begin(); it_b != it_a->second.end(); it_b++){
                destoryConnection(it_b->second);
            }
        }
    }
    pthread_rwlock_unlock(&conn_list_lock);
    return true;
}

void ConnPool::closeConnection(int conn){
    if(conn > 0){
        close(conn);
    }
}
void ConnPool::destoryConnection(int conn) {
    int res;
    map<uint32_t,map<string, map<uint32_t,int> > >::iterator it;
    map<string, map<uint32_t,int> >::iterator it_a;
    map<uint32_t,int>::iterator it_b;

    if (conn <= 0) {
        return;
    }
    close(conn);
    res = pthread_rwlock_wrlock(&conn_list_lock);
    if(res != 0){
        log_error("pthread_rwlock_wrlock fail");
    }
    for (it = conn_list.begin(); it != conn_list.end(); ++it){
        for(it_a = it->second.begin(); it_a != it->second.end(); it_a++){
            for(it_b = it_a->second.begin(); it_b != it_a->second.end(); it_b++){
                if(it_b->second == conn){
                    it_b->second  = 0;
                    break;
                }
            }
         }
    }
    pthread_rwlock_unlock(&conn_list_lock);
}

int ConnPool::getConnectionSize(){
    int n = 0;
    int res = 0;
    map<uint32_t,map<string, map<uint32_t,int> > >::iterator it;
    map<string, map<uint32_t,int> >::iterator it_a;
    map<uint32_t,int>::iterator it_b;

    res = pthread_rwlock_rdlock(&conn_list_lock);
    if(res != 0){
        log_error("pthread_wrlock_rdlock");
        return -1;
    }
    for (it = conn_list.begin(); it != conn_list.end(); ++it){
        for(it_a = it->second.begin(); it_a != it->second.end(); it_a++){
            for(it_b = it_a->second.begin(); it_b != it_a->second.end(); it_b++){
                n++;
            }
        }
    }
    pthread_rwlock_unlock(&conn_list_lock);
    return n;
}

