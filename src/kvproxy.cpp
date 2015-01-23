#include "kvproxy.h"

bool KvProxy::is_async = false;

KvProxy::KvProxy(int count, bool cpu_affinity) : Server(count, cpu_affinity){
    int timeout;
   
    st_limit = ST_LIMIT;
    st_req = 0;
    st_conn = 0;
    thread_count = count;
    recv_timeout = TIMEOUT;
    timeout = Config::getConfInt("kvproxy","connect_timeout");
    if(timeout != 0){
        connect_timeout = timeout * 1000;
    }
    timeout = Config::getConfInt("kvproxy","send_timeout");
    if(timeout != 0){
        send_timeout = timeout * 1000;
    }
    timeout = Config::getConfInt("kvproxy","recv_timeout");
    if(timeout != 0){
        recv_timeout = timeout * 1000;
    }
    
    max_packet = Config::getConfInt("kvproxy","max_packet");
    if(max_packet <= 0){
        max_packet = MAX_PACKET;
    }
    
    failover_threshold = Config::getConfInt("kvproxy","failover_threshold");
    if(failover_threshold <= 0){
        failover_threshold = FAILOVER_THRESHOLD;
    }
    
    failover_interval = Config::getConfInt("kvproxy","failover_interval");
    if(failover_interval <= 0){
        failover_interval = FAILOVER_INTERVAL;
    }
    
    async_size = Config::getConfInt("kvproxy","async_size");
    if(async_size <= 0){
        async_size = ASYNC_SIZE;
    }
    
    conn_pool = ConnPool::getInstance();

    for(int id = 0; id <= thread_count; id++){
        req_buf[id] = malloc(max_packet * 1024 * 1024); 
        req_buf_len[id] = 0;
        req_buf_size[id] = max_packet * 1024 * 1024;
        resp_buf[id] = malloc(max_packet * 1024 * 1024); 
        resp_buf_len[id] = 0;
        resp_buf_size[id] = max_packet * 1024 * 1024;
        backend_buf[id] = malloc(max_packet * 1024 * 1024); 
        backend_buf_len[id] = 0;
        backend_buf_size[id] = max_packet * 1024 * 1024;
        client_buf[id] = malloc(max_packet * 1024 * 1024); 
        client_buf_len[id] = 0;
        client_buf_size[id] = max_packet * 1024 * 1024;
    }
}

KvProxy::~KvProxy(){
    for(int id = 0; id <= thread_count; id++){
        free(req_buf[id]); 
        free(resp_buf[id]); 
        free(backend_buf[id]);
    }
}

void KvProxy::initVar(){
    map<uint32_t, pair<string,uint32_t> >::iterator it;
    for(it = host_alias.begin(); it != host_alias.end(); it++){
        host_offline[it->first] = false;
        st_cont_fail[it->first] = 0;
        st_fail[it->first] = 0;
    }
}

map<uint32_t, pair<string,uint32_t> >  KvProxy::getHostAlias(){
    return host_alias;
}

map<uint32_t, bool>  KvProxy::getHostOffline(){
    return host_offline;
}

ConnPool * KvProxy::getConnPool(){
    return conn_pool;
}

int KvProxy::getFailOverInterval(){
    return failover_interval;
}

int KvProxy::getFailOverThreshold(){
    return failover_threshold;
}

map<uint32_t, uint32_t> KvProxy::getStContFail(){
    return st_cont_fail;
}

int KvProxy::getThreadCount(){
    return thread_count;
}

void *asyncReqThread(void* args){
    queue<req_group_async_t> async_req_tmp;
    req_group_async_t req_g_tmp;
    KvProxy * obj = (KvProxy *)args;
    int thread_count = obj->getThreadCount();
    while(1){
        if (obj->async_req.size() > 0){
            if(pthread_mutex_lock(&obj->async_req_lock) == 0){
                swap(obj->async_req,async_req_tmp);
                pthread_mutex_unlock(&obj->async_req_lock);
            }
        }else{
            usleep(1000);
        }
        while(!async_req_tmp.empty()){
            req_g_tmp = async_req_tmp.front();
            obj->talkToBackendAsync(&req_g_tmp, 0, thread_count);
            async_req_tmp.pop();
        }
    }
}

void KvProxy::createAsyncReqThread(){
    int rt;
    pthread_attr_t attr;

    rt = pthread_mutex_init(&async_req_lock,NULL);
    if (rt != 0){
        cerr << "[error]:mutex initialization failed" << endl;
        log_fatal("mutex initialization failed ");
        exit(EXIT_FAILURE);
    }

    pthread_attr_init(&attr);  
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);  
    rt = pthread_create( &th_async, &attr, asyncReqThread, this );
    if( rt != 0 ){
		cerr << "fail to create thread for async request!" << endl;
        log_fatal("fail to create thread for async request");
		exit(EXIT_FAILURE);
    }
    is_async = true;
}

string KvProxy::getStatus(){
    string content;
    string str_req_failed;
    int int_req_failed = 0;
    string str_req_cont_failed;
    int int_req_cont_failed = 0;
    string host;
    int port;
    string str_st_req;
    string str_host_offline;
    int int_host_offline = 0;
    map<uint32_t, uint32_t>::iterator it; 
    map<uint32_t, bool>::iterator it_of;

    if(st_req >= ST_LIMIT){
        str_st_req = int2str(st_req) + "+";
    }else{
        str_st_req = int2str(st_req);
    }

    content = "\nNumber of processed requests [" + str_st_req + "]\n";
    for(it = st_fail.begin(); it != st_fail.end(); it++){
        host = host_alias[it->first].first;
        port = host_alias[it->first].second;
        str_req_failed += "  - " + host + ":" + int2str(port) + " [" + int2str(it->second) + "]\n"; 
        int_req_failed += it->second;
    }
    for(it = st_cont_fail.begin(); it != st_cont_fail.end(); it++){
        host = host_alias[it->first].first;
        port = host_alias[it->first].second;
        str_req_cont_failed += "  - " + host + ":" + int2str(port) + " [" + int2str(it->second) + "]\n"; 
        int_req_cont_failed += it->second;
    }
    for(it_of = host_offline.begin(); it_of != host_offline.end(); it_of++){
        if(it_of->second == false){
            continue;
        }
        host = host_alias[it_of->first].first;
        port = host_alias[it_of->first].second;
        str_host_offline += "  - " + host + ":" + int2str(port) + " [offline]\n";
        int_host_offline++;
    }
    content += "Number of requests failed [" + int2str(int_req_failed) + "]\n";
    content += str_req_failed;
    content += "Number of continuous requests failed [" + int2str(int_req_cont_failed) + "]\n";
    content += str_req_cont_failed;
    content += "All of offline hosts ["+ int2str(int_host_offline) +"] \n";
    content += str_host_offline;
    content += "Number of client connection [" + int2str(st_conn) + "]\n";
    content += "Number of backend connection [" + int2str(conn_pool->getConnectionSize()) + "]\n";
    content += "Read timeout of backend connection [" + int2str(recv_timeout/1000) + "ms]\n";
    content += "Size of async queue [" + int2str(async_req.size()) + "]\n";
    content += "\n";
    return content;
}

void KvProxy::loadExt(string ext_path, string ext_name, string ext_filename){
    ext.setExtPath(ext_path);
    ext.load(ext_name, ext_filename);
    sync_str = Config::getConfStr(ext_name,"sync_str");

    ptr_ext_version = ext.findExtVersion(ext_name);
    if (ptr_ext_version == NULL){
        log_fatal("can not find function of extension version");
        cerr << "can not find function of extension version" << endl;
        exit(EXIT_FAILURE);
    }else if (ptr_ext_version() != EXT_VER){
        log_error("extension version dos not match");
        cerr << "extension version dos not match" << endl;
    }
    ptr_parse_req = ext.findParseReq(ext_name);
    if (ptr_parse_req == NULL){
        log_fatal("can not find function of extension parse req");
        cerr << "can not find function of extension parse req" << endl;
        exit(EXIT_FAILURE);
    }
    ptr_create_req = ext.findCreateReq(ext_name);
    if (ptr_create_req == NULL){
        log_fatal("can not find function of extension create req");
        cerr << "can not find function of extension create req" << endl;
        exit(EXIT_FAILURE);
    }
    ptr_create_req_async = ext.findCreateReqAsync(ext_name);
    if (ptr_create_req_async == NULL){
        log_fatal("can not find function of extension create req async");
        cerr << "can not find function of extension create req async" << endl;
        exit(EXIT_FAILURE);
    }
    ptr_parse_resp = ext.findParseResp(ext_name);
    if (ptr_parse_resp == NULL){
        log_fatal("can not find function of extension parse resp");
        cerr << "can not find function of extension parse resp" << endl;
        exit(EXIT_FAILURE);
    }
    ptr_create_resp = ext.findCreateResp(ext_name);
    if (ptr_create_resp == NULL){
        log_fatal("can not find function of extension create resp");
        cerr << "can not find function of extension create resp" << endl;
        exit(EXIT_FAILURE);
    }
}

void KvProxy::setHosts(string type, string ext_name, uint32_t thread_count){
    map<pair<string,uint32_t>, pair<uint32_t,uint32_t> > hosts_tmp;
    string group_name;
    map<string, string> host_group;
    map<string, string>::iterator g_it;
    map<int, string> split_vals;
    string host;
    string host_port;
    uint32_t port = 0;
    uint32_t index;
    uint32_t weight;
    uint32_t alias_index;
   
    group_name = Config::getConfStr(ext_name, type);
    if(group_name != ""){
        host_group = Config::getConfStr(group_name);
    }

    for(g_it = host_group.begin(); g_it != host_group.end(); g_it++){
        split_vals = split(g_it->first, ':');
        if(split_vals.size() != 2){
            log_fatal("host group in config is invalid: %s" , g_it->first.c_str());
            cerr << "host group in config is invalid: " + g_it->first << endl;
            exit(EXIT_FAILURE);
        }else{
            host = split_vals[0];
            port = str2int(split_vals[1].c_str());
        }
        split_vals = split(g_it->second, ':');
        if(split_vals.size() != 2){
            log_fatal("host group in config is invalid: %s" , g_it->second.c_str());
            cerr << "host group in config is invalid:" + g_it->second << endl;
            exit(EXIT_FAILURE);
        }else{
            index = str2int(split_vals[0].c_str());
            weight = str2int(split_vals[1].c_str());
        }
        
        host_port = host + ":" + int2str(port);
        alias_index = setHostAlias(host, port);
        host_infos[alias_index] = make_pair(index,weight);
        hosts_tmp[make_pair(host_port,alias_index)] = make_pair(index,weight);
        conn_pool->initConnection(host, port, thread_count);
        if(type == "hosts"){
            hosts_default.insert(alias_index);
        }else if(type == "hosts_backup"){
            hosts_backup.insert(alias_index);
        }else if(type == "hosts_read"){
            hosts_read.insert(alias_index);
        } 
    }
    if(type == "hosts" && hosts_tmp.size() == 0){
        log_fatal("hosts in config is empty");
        cerr << "host in config is empty " << endl;
        exit(EXIT_FAILURE);
    }else if(type == "hosts"){
        hash.setHosts(hosts_tmp);
    }else if(type == "hosts_backup" && hosts_tmp.size() > 0){
        hash_backup.setHosts(hosts_tmp);
        createAsyncReqThread();
    }else if(type == "hosts_read" && hosts_tmp.size() > 0){
        hash_read.setHosts(hosts_tmp);
    }
}

uint32_t KvProxy::setHostAlias(string host, uint32_t port){
    static uint32_t index = 0;
    map<uint32_t, pair<string,uint32_t> >::iterator it;
    pair<string, uint32_t> host_port;
    host_port.first = host;
    host_port.second = port;
    for(it = host_alias.begin(); it != host_alias.end(); it++){
        if(it->second.first == host && it->second.second == port){
            return it->first;
        }
    }
    index++;
    host_alias[index] = host_port;
    return index;
}

bool KvProxy::failover(uint32_t alias_index, bool is_del){
    bool ret = false;
    string host_port;
    uint32_t index;
    uint32_t weight;

    if(is_del == true){
        if(host_offline[alias_index] == true){
             return true;
        } 
        if(host_alias.find(alias_index) != host_alias.end()){
            host_port = host_alias[alias_index].first + ":" + int2str(host_alias[alias_index].second);
        }else{
            return false;
        } 
        if(hosts_default.find(alias_index) != hosts_default.end()){
            ret = hash.delHost(alias_index);
            log_warn("%s offline in hosts, %s", host_port.c_str(), bool2str(ret).c_str());
        }
        if(hosts_read.find(alias_index) != hosts_read.end()){
            ret = hash_read.delHost(alias_index);
            log_warn("%s offline in hosts read, %s", host_port.c_str(), bool2str(ret).c_str()); 
        }
        if(hosts_backup.find(alias_index) != hosts_backup.end()){
            hash_backup.delHost(alias_index);
            log_warn("%s offline in hosts backup, %s", host_port.c_str(), bool2str(ret).c_str());
        }
        host_offline[alias_index] = true;
    }else{
        if(host_offline[alias_index] == false){
            return true;
        }
        if(host_alias.find(alias_index) != host_alias.end()){
            host_port = host_alias[alias_index].first + ":" + int2str(host_alias[alias_index].second);
        }else{
            return false;
        }
        if(host_infos.find(alias_index) != host_infos.end()){
           index = host_infos[alias_index].first;
           weight = host_infos[alias_index].second;
        }else{
            return false;
        }
        if(hosts_default.find(alias_index) != hosts_default.end()){
            ret = hash.addHost(host_port, alias_index, index, weight); 
            log_warn("%s online in hosts, %s", host_port.c_str(), bool2str(ret).c_str());
        }
        if(hosts_read.find(alias_index) != hosts_read.end()){
            ret = hash_read.addHost(host_port, alias_index, index, weight); 
            log_warn("%s online in hosts read, %s", host_port.c_str(), bool2str(ret).c_str());
        }
        if(hosts_backup.find(alias_index) != hosts_backup.end()){
            ret = hash_backup.addHost(host_port, alias_index, index, weight); 
            log_warn("%s online in hosts backup, %s", host_port.c_str(), bool2str(ret).c_str());
        }
        
        host_offline[alias_index] = false;
    }
    return true;
}

void KvProxy::countFail(bool is_fail, uint32_t alias_index){ 
    if(is_fail == true){
        if(st_fail[alias_index] < st_limit){
            st_fail[alias_index]++;
            st_cont_fail[alias_index]++;
        }
    }else if(st_cont_fail[alias_index] != 0){
        st_cont_fail[alias_index] = 0;
    }
}

void * checkHealthThread(void* args){
    string host;
    uint32_t port;
    uint32_t alias_index;
    int conn_fd;
    int failover_interval;
    int failover_threshold;
    int thread_count;
    map<uint32_t, pair<string,uint32_t> > host_alias;
    map<uint32_t, bool> host_offline;
    map<uint32_t,bool>::iterator it;
    map<uint32_t, uint32_t> st_cont_fail;
    map<uint32_t, uint32_t>::iterator it_fail;
    ConnPool * conn_pool;
    KvProxy * obj = (KvProxy *)args;
    
    failover_interval = obj->getFailOverInterval();
    failover_threshold = obj->getFailOverThreshold();
    host_alias = obj->getHostAlias();
    conn_pool = obj->getConnPool();
    thread_count = obj->getThreadCount();

    while(1){
        sleep(failover_interval);
        //start to check online host which is failed
        st_cont_fail = obj->getStContFail();
        for(it_fail = st_cont_fail.begin(); it_fail != st_cont_fail.end(); it_fail++){
            if((int)it_fail->second >= failover_threshold){
                obj->failover(it_fail->first, true);      
            } 
        }
        //start to check offline host
        host_offline = obj->getHostOffline();
        for(it = host_offline.begin(); it != host_offline.end(); it++){
            if(it->second == false){
                continue;
            }
            alias_index = it->first;
            if(host_alias.find(alias_index) == host_alias.end()){
                continue;
            }
            host = host_alias[alias_index].first;
            port = host_alias[alias_index].second;
            conn_fd = conn_pool->createConnection(host, port); 
            if(conn_fd > 0){
                obj->failover(alias_index, false);
                obj->countFail(false, alias_index);
                conn_pool->closeConnection(conn_fd);
            }
        }
    }
}

void KvProxy::createCheckHealthThread(){
    int rt;
    pthread_attr_t attr;

    pthread_attr_init(&attr);  
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);  
    rt = pthread_create( &th_check_health, &attr, checkHealthThread, this );
    if( rt != 0 ){
		cerr << "fail to create thread for check health!" << endl;
        log_fatal("fail to create thread for check health");
		exit(EXIT_FAILURE);
    }
}

void KvProxy::createReqGroup(uint32_t cur_conn_fd, uint32_t thread_index, req_group_t *group, req_group_t *backup, req_group_t *sync){ 
    pair<string, uint32_t> host;  
    req_ptr_list_t req_list_each;
    req_ptr_list_t::iterator req_ptr_list_it;
    req_list_t::iterator req_list_it;
    req_group_t::iterator req_group_it;
    
    for (req_list_it = req_list_data[thread_index].begin(); req_list_it != req_list_data[thread_index].end(); req_list_it++){
        if(req_list_it->each == true){
            req_list_each.push_back(&*req_list_it);
        }else if (req_list_it->key_len == 0){
                continue;
        }else{ 
            if(req_list_it->update != true && hash_read.getSize() > 0){
                host = hash_read.find(req_list_it->key, req_list_it->key_len);
            }else{
                host = hash.find(req_list_it->key, req_list_it->key_len);
            }
            if (host.second == 0){
                continue;
            }
            log_debug("[%d]the key is %*.*s, host info is %s", 
                    cur_conn_fd, req_list_it->key_len, req_list_it->key_len, req_list_it->key, host.first.c_str());
            (*group)[host.second].push_back(&*req_list_it);
        }
        if(req_list_it->update == true && hash_backup.getSize() > 0){ 
            host = hash_backup.find(req_list_it->key, req_list_it->key_len);
            if (host.second == 0){
                continue;
            }
            if (strncmp(req_list_it->key, sync_str.c_str(), sync_str.size()) == 0){
                (*sync)[host.second].push_back(&*req_list_it);
            }else{
                    //todo
                    (*backup)[host.second].push_back(&*req_list_it);
            }        
        }
    }
        
    for(req_ptr_list_it = req_list_each.begin(); req_ptr_list_it != req_list_each.end(); req_ptr_list_it++){
        for(req_group_it = group->begin(); req_group_it != group->end(); req_group_it++){
            (*group)[req_group_it->first].push_back(*req_ptr_list_it);
        }
        for(req_group_it = backup->begin(); req_group_it != backup->end(); req_group_it++){
            (*backup)[req_group_it->first].push_back(*req_ptr_list_it);
        }
    }
}

void KvProxy::readEvent(Conn *conn){
    char cmd[7] = "status";
    ext_ret_t ext_ret; 
    char buf[FAST_BUFF_SIZE];
    char * buf_ptr;
    int ret;
    int buf_len;
    int cur_conn_fd = conn->getFd();
    int thread_index = conn->getThread()->index;
    
    req_group_t req_group;
    req_group_t req_group_backup;
    req_group_t req_group_backup_sync;
    req_group_t::iterator group_it;
    req_ptr_list_t::iterator list_it;
    req_group_async_t req_group_backup_async;
    req_group_async_t::iterator async_group_it;
    comm_t async_item;

    string resp_cont;
    string resp_cont_sync;

    //read request data
    buf_len = conn->getReadBufLen();
  
    if(buf_len <= FAST_BUFF_SIZE){
        buf_ptr = buf;
    }else if(buf_len <= req_buf_size[thread_index]){
        buf_ptr = (char *)req_buf[thread_index];
    }else{
        conn->clearReadBuf(buf_len);
        return;
    }
    
    buf_len = conn->CopyReadBuf(buf_ptr, buf_len);
    log_debug("[%d]read size %d", cur_conn_fd, buf_len);

    if (memcmp(buf_ptr,cmd,6) == 0 && buf_len <= 8){
        resp_cont = getStatus();
        ret = conn->addToWriteBuf(resp_cont.c_str(), resp_cont.size());
        conn->clearReadBuf(buf_len);
        return ;
    }
    
    ext_ret = ptr_parse_req(buf_ptr, buf_len, &position[thread_index], &req_list_data[thread_index]);
    
    if (ext_ret == SUCCESS){
        log_debug("[%d]parse request function return %d. success", cur_conn_fd, ext_ret);
        if (st_req < st_limit){
            st_req++;
        }
        conn->clearReadBuf(buf_len);
        
        createReqGroup(cur_conn_fd, thread_index, &req_group, &req_group_backup, &req_group_backup_sync);
        log_info("[%d]start to talk to backend. content size %d. ", cur_conn_fd, buf_len);
        talkToBackend(&req_group, buf_ptr, buf_len, cur_conn_fd, thread_index);
        
        //deal async req data
        if (req_group_backup.size() > 0 && (int)async_req.size() < async_size){
            req_group_backup_async.clear();
            for(group_it = req_group_backup.begin(); group_it != req_group_backup.end(); group_it++){
                for(list_it = group_it->second.begin(); list_it != group_it->second.end(); list_it++){
                   async_item.content = string((*list_it)->content, (*list_it)->content_len);
                   memcpy(async_item.ext, (*list_it)->ext, sizeof((*list_it)->ext));
                   req_group_backup_async[group_it->first].push_back(async_item); 
                }
            } 
            if(pthread_mutex_lock(&async_req_lock) == 0){
                async_req.push(req_group_backup_async);
                pthread_mutex_unlock(&async_req_lock);
            }
            
        }

        //deal sync req data
        if (req_group_backup_sync.size() > 0){
            talkToBackend(&req_group_backup_sync, buf_ptr, buf_len, cur_conn_fd, thread_index);
            log_info("[%d]talk to backend. content size %d. response size %d.", cur_conn_fd, buf_len, resp_cont_sync.size());
        }

        if(client_buf_len[thread_index] > 0){
            ret = conn->addToWriteBuf((char *)client_buf[thread_index], client_buf_len[thread_index]);
            log_info("[%d]finish to response to client.content size %d. return %d", cur_conn_fd, client_buf_len[thread_index], ret);
        }else{
            log_warn("[%d]dos not response to client, because the response content size is zero",cur_conn_fd);
        }
    }else if(ext_ret == ERROR){
        log_error("[%d]parse request function return %d. fail", cur_conn_fd, ext_ret);
        conn->clearReadBuf(buf_len);
    }else if(ext_ret == IGNORE){
        log_info("[%d]parse request function return %d. ignore", cur_conn_fd, ext_ret);
        conn->clearReadBuf(buf_len);
    }
    position[thread_index] = 0;
    req_list_data[thread_index].clear();
}

int KvProxy::writeToBackend(uint32_t *conn_fd, char *content, uint32_t len, uint32_t cur_conn_fd, 
                                uint32_t alias_index, uint32_t thread_index){
    char *msg;
    int msg_len;
    int send_len;
    int ret_len;

    msg_len = len;
    send_len = 0;
    while(1){
        msg = content;
        ret_len = send(*conn_fd, msg+send_len, msg_len-send_len, MSG_WAITALL);
        if(ret_len > 0){
            send_len += ret_len;
            if (send_len >= msg_len){
                break;
            }
        }else if (errno == EINTR){
            continue;
        }else{
            conn_pool->destoryConnection(*conn_fd);
            log_warn("[%d]connection is not useable fd %d",cur_conn_fd, *conn_fd);
            *conn_fd = getConnection(alias_index, cur_conn_fd, thread_index);
            if (*conn_fd <= 0){
                send_len = 0;
                break;
            }
        }
    }
    log_info("[%d]send to backend. need to send %d, have sended content length %d",cur_conn_fd, msg_len, send_len);
    return send_len;
}

uint32_t KvProxy::getConnection(uint32_t alias_index, uint32_t cur_conn_fd, uint32_t thread_index){ 
    pair<string,uint32_t> host_port; 
    string host;
    uint32_t port;
    int conn_fd;
    
    host_port = host_alias[alias_index];
    host = host_port.first;
    port = host_port.second;
    log_debug("[%d]start to get connection %s:%d",cur_conn_fd, host.c_str(), port);
    conn_fd = conn_pool->getConnection(thread_index, host, port);
    if (conn_fd <= 0){
        log_error("[%d]fail to get connection %s:%d",cur_conn_fd, host.c_str(), port);
        conn_fd = 0;
    }
    return conn_fd;
}

void KvProxy::readFromBackend(uint32_t conn_fd, resp_list_t * resp_list, uint32_t cur_conn_fd, uint32_t thread_index){
    int pos = 0; 
    fd_set rdfds;
    struct timeval tv;
    int ret;
    int len;
    ext_ret_t ext_ret;
   
    resp_buf_len[thread_index] = 0;
    while(1){
        FD_ZERO(&rdfds);
        FD_SET(conn_fd, &rdfds);
        tv.tv_sec = 0;
        tv.tv_usec = recv_timeout;
        log_info("[%d]start to select. fd %d",cur_conn_fd, conn_fd);
        ret = select(conn_fd+1, &rdfds, NULL, NULL, &tv);
        if(ret < 0){
            if (errno == EINTR){
                continue;
            }
            log_error("[%d]select error. fd %d, errno %d",cur_conn_fd, conn_fd, errno);
            break;
        }else if (ret == 0){
            log_error("[%d]select timeout. tv.tv_sec %d,tv.tv_usec %d. fd %d",cur_conn_fd, tv.tv_sec, tv.tv_usec, conn_fd);
            break;
        }else if (FD_ISSET(conn_fd,&rdfds) <= 0){
            log_warn("[%d]can not read. fd %d",cur_conn_fd, conn_fd);
            continue;
        }
        log_info("[%d]start to recv. fd %d",cur_conn_fd, conn_fd);
        len = recv(conn_fd, (char *)resp_buf[thread_index] + resp_buf_len[thread_index], 
                    resp_buf_size[thread_index] - resp_buf_len[thread_index], MSG_DONTWAIT);
        if(len > 0){
            log_info("[%d]start to ptr_parse_resp", cur_conn_fd);
            resp_buf_len[thread_index] += len;
            if(resp_buf_len[thread_index] >= resp_buf_size[thread_index]){
                return ;
            }
            ext_ret = ptr_parse_resp((const char *)resp_buf[thread_index], resp_buf_len[thread_index], &pos, resp_list);
            if(ext_ret == SUCCESS){
                log_info("[%d]ptr_parse_resp. content size %d. return code %d. success",cur_conn_fd, resp_buf_len[thread_index], ext_ret);
                resp_buf_len[thread_index] = 0;
                pos = 0;
                break;
            }else if(ext_ret == ERROR){
                log_info("[%d]ptr_parse_resp. content size %d. return code %d. error",cur_conn_fd, resp_buf_len[thread_index], ext_ret);
                pos = 0;
                resp_buf_len[thread_index] = 0;
                resp_list->clear();
                break;
            }
            continue;
        }else if(len == 0){
            log_error("[%d]connection is closed. conn fd %d",cur_conn_fd, conn_fd);
            resp_buf_len[thread_index] = 0;
            conn_pool->destoryConnection(conn_fd);
        }else if(len == -1){
            if(errno == EAGAIN){
                break;
            }else if (errno == EINTR){
                continue;
            }else{
                break;
            }
        }
    }
    resp_buf_len[thread_index] = 0;
}

void KvProxy::talkToBackend(req_group_t * const req_group_data, const char * buf, int size, int cur_conn_fd, uint32_t thread_index){
    req_group_t::iterator req_group_it;
    ext_ret_t ext_ret;
    resp_list_t resp_list;

    uint32_t conn_fd;
    int send_len = 0;
   
    log_debug("[%d]req_group_data size %d.",cur_conn_fd, req_group_data->size());
    for (req_group_it = req_group_data->begin(); req_group_it != (*req_group_data).end(); req_group_it++){
        log_debug("[%d]req_group_data to host alias %d, size %d.",cur_conn_fd, req_group_it->first, req_group_it->second.size());
        ext_ret = ptr_create_req(&req_group_it->second, (char *)backend_buf[thread_index], &backend_buf_len[thread_index]);
        if (ext_ret == SUCCESS && backend_buf_len[thread_index] != 0){
            log_debug("[%d]ptr_create_req return success.req_data_to_backend size %d",cur_conn_fd, backend_buf_len[thread_index]);
            conn_fd = getConnection(req_group_it->first, cur_conn_fd, thread_index);
            if(conn_fd == 0){
                countFail(true, req_group_it->first);
                continue;
            }
            send_len = writeToBackend(&conn_fd, (char *)backend_buf[thread_index], backend_buf_len[thread_index], 
                                        cur_conn_fd, req_group_it->first, thread_index);
            if (send_len >= backend_buf_len[thread_index]){
                readFromBackend(conn_fd, &resp_list, cur_conn_fd, thread_index);
                if(resp_list.empty()){
                    countFail(true, req_group_it->first);
                }else{
                    countFail(false, req_group_it->first);
                }
            }else if(send_len <= 0){
                countFail(true, req_group_it->first);
            }
        }else{
            log_error("[%d]ptr_create_req return %d. fail. req_data_to_backend size %d",
                            cur_conn_fd, ext_ret, backend_buf_len[thread_index]);
        }
        backend_buf_len[thread_index] = 0;
    }
    client_buf_len[thread_index] = 0;
    ext_ret = ptr_create_resp(&resp_list, (char *)client_buf[thread_index], &client_buf_len[thread_index], &client_buf_size[thread_index]);
}

void KvProxy::talkToBackendAsync(req_group_async_t * const req_group_data, int cur_conn_fd, uint32_t thread_index){
    req_group_async_t::iterator req_group_it;
    ext_ret_t ext_ret;
    resp_list_t resp_list;

    uint32_t conn_fd;
    int send_len = 0;
   
    log_debug("[%d]req_group_data size %d.",cur_conn_fd, req_group_data->size());
    for (req_group_it = req_group_data->begin(); req_group_it != (*req_group_data).end(); req_group_it++){
        log_debug("[%d]req_group_data to host alias %d, size %d.",cur_conn_fd, req_group_it->first, req_group_it->second.size());
        ext_ret = ptr_create_req_async(&req_group_it->second, (char *)backend_buf[thread_index], &backend_buf_len[thread_index]);
        if (ext_ret == SUCCESS && backend_buf_len[thread_index] != 0){
            log_debug("[%d]ptr_create_req return success.req_data_to_backend size %d",cur_conn_fd, backend_buf_len[thread_index]);
            conn_fd = getConnection(req_group_it->first, cur_conn_fd, thread_index);
            if(conn_fd == 0){
                countFail(true, req_group_it->first);
                continue;
            }
            send_len = writeToBackend(&conn_fd, (char *)backend_buf[thread_index], backend_buf_len[thread_index], 
                                        cur_conn_fd, req_group_it->first, thread_index);
            if (send_len >= backend_buf_len[thread_index]){
                readFromBackend(conn_fd, &resp_list, cur_conn_fd, thread_index);
                if(resp_list.empty()){
                    countFail(true, req_group_it->first);
                }else{
                    countFail(false, req_group_it->first);
                }
            }else if(send_len <= 0){
                countFail(true, req_group_it->first);
            }
        }else{
            log_error("[%d]ptr_create_req return %d. fail. req_data_to_backend size %d",
                            cur_conn_fd, ext_ret, backend_buf_len[thread_index]);
        }
        backend_buf_len[thread_index] = 0;
    }
    client_buf_len[thread_index] = 0;
}

void KvProxy::writeEvent(Conn *conn){
}

void KvProxy::connectionEvent(Conn *conn){
    st_conn++;
}

void KvProxy::closeEvent(Conn *conn, short events){
    st_conn--;
}

void KvProxy::quitCb(int sig, short events, void *data){ 
    KvProxy *me = (KvProxy*)data;
    if(is_async == true){
        pthread_kill(me->th_async,SIGINT);
    }
    pthread_kill(me->th_check_health,SIGINT);
    timeval tv = {1, 0};
    me->stopRun(&tv);
    exit(EXIT_SUCCESS);
}

void KvProxy::timeOutCb(int id, short events, void *data){
}

void daemon(void){
        int pid;
        
        pid = fork();
        if(pid > 0){
            exit(EXIT_SUCCESS);
        }else if(pid < 0){
            exit(EXIT_FAILURE);
        }
        setsid();
        umask(0);
}

void help(){
    cout << "Usage: kvproxy" << endl;
    cout << "-d run as daemon. optional." << endl;
    cout << "-h show help info. optional." << endl;
    cout << "-c specify the path of config. optional." << endl;
    cout << "-v show version info. optional." << endl;
    cout << "more help info: http://www.bo56.com" << endl;
    exit(EXIT_SUCCESS);
}

void show_version(){
    cout << "KvProxy "<< KVPROXY_VERSION << "  (built: " << KVPROXY_BUILT << ") " << endl;
    exit(EXIT_SUCCESS);
}

void initLog(string cwd){
    string log_path;
    string log_level;
    int level;

    log_path = Config::getConfStr("kvproxy","log_path");
    if (log_path.empty()){
        log_path = cwd + "../log/kvproxy.log";
    }
    log_open(log_path.c_str());
    log_level = Config::getConfStr("kvproxy","log_level");
    if (!log_level.empty()){
        level = get_log_level(log_level.c_str());
        set_log_level(level);
    }
}

int main(int argc ,char **args){    
    string ext_path;
    string ext_name;
    string ext_filename;
    string cwd;
    string conf_path;
    uint32_t thread_count;
    uint32_t port;
    int backlog;
    int ch;
    bool is_daemon = false;
    bool cpu_affinity = false;
    int cpu_num = 0;

    signal(SIGPIPE, SIG_IGN);

    while((ch = getopt(argc, args, "c:dhv")) != -1){
        switch(ch){
            case 'h':
                help();
                break;
            case 'd':
                is_daemon = true;
                break;
            case 'c':
                conf_path = optarg;
                break;
            case 'v':
                show_version();
                break;
            default:
                help();
        }
    }
    
    cwd = get_cwd(args[0]);
    if(conf_path.empty()){
        conf_path = cwd + "../etc/kvproxy.ini";
    }
    Config::setConfFile(conf_path);
    Config::loadConf();

    if(is_daemon == true){
        daemon();
    }
    
    initLog(cwd);
    cpu_num = get_nprocs();
    cpu_affinity = Config::getConfBool("kvproxy","cpu_affinity");
    thread_count = Config::getConfInt("kvproxy","thread_count");
    if(cpu_affinity == true){
        thread_count = cpu_num;
    }else if(thread_count <= 0){
        thread_count = 1;
    }
    KvProxy server(thread_count, cpu_affinity);
    
    ext_path = Config::getConfStr("kvproxy","ext_path");
    if (ext_path.empty()){
        ext_path = cwd + "../ext";
    }
    if(ext_name.empty()){
        ext_name = Config::getExtName();
    }
    ext_filename = Config::getConfStr(ext_name,"extension");
    
    server.loadExt(ext_path, ext_name, ext_filename);
    
    server.setHosts("hosts", ext_name, thread_count);
    server.setHosts("hosts_backup", ext_name, thread_count);
    server.setHosts("hosts_read", ext_name, thread_count);

    server.addSignalEvent(SIGINT, KvProxy::quitCb);
    timeval tv = {10, 0};
    server.addTimerEvent(KvProxy::timeOutCb, tv, false);
    server.initVar();
    server.createCheckHealthThread();

    port = Config::getConfInt("kvproxy","port");
    if (port == 0){
        port = 55669;
    }
    backlog = Config::getConfInt("kvproxy","backlog");
    if (backlog <= 0){
        backlog = -1;
    }
    server.setPort(port);
    server.startRun(backlog);
    return 0;
}
