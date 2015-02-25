#include "serv.h"

Conn::Conn(uint32_t fd) : m_fd(fd){
    m_prev = NULL;
    m_next = NULL;
}
Conn::~Conn(){};

ConnQueue::ConnQueue(){
    m_head = new Conn(0);
    m_tail = new Conn(0);
    m_head->m_prev = m_tail->m_next = NULL;
    m_head->m_next = m_tail;
    m_tail->m_prev = m_head;
}
ConnQueue::~ConnQueue(){
    Conn *tcur, *tnext;
    tcur = m_head;
    while( tcur != NULL ){
        tnext = tcur->m_next;
        delete tcur;
        tcur = tnext;
    }
}
Conn *ConnQueue::insertConn(uint32_t fd, LibeventThread *t){
    Conn *c = new Conn(fd);
    c->m_thread = t;
    Conn *next = m_head->m_next;

    c->m_prev = m_head;
    c->m_next = m_head->m_next;
    m_head->m_next = c;
    next->m_prev = c;
    return c;
}
void ConnQueue::deleteConn(Conn *c){
    c->m_prev->m_next = c->m_next;
    c->m_next->m_prev = c->m_prev;
    delete c;
}

Server::Server(uint32_t count, bool cpu_affinity){
    m_thread_count = count;
    m_cpu_affinity = cpu_affinity;
    m_port = -1;
    m_main_base = new LibeventThread;
    m_threads = new LibeventThread[m_thread_count];
    m_main_base->tid = pthread_self();
    m_main_base->base = event_base_new();

    for(int i=0; i<m_thread_count; i++){
        setupThread(&m_threads[i], i);
    }
}

Server::~Server(){
    stopRun(NULL);
    event_base_free(m_main_base->base);
    for(int i=0; i<m_thread_count; i++){
        event_base_free(m_threads[i].base);
    }

    delete m_main_base;
    delete [] m_threads;
}

void Server::errorQuit(const char *str){
    fprintf(stderr, "%s", str);
    if( errno != 0 ){
        fprintf(stderr, " : %s", strerror(errno));
    }
    fprintf(stderr, "\n");
    exit(1);    
}

void Server::setupThread(LibeventThread *me, uint32_t i){
    me->tcp_connect = this;
    me->base = event_base_new();
    if(NULL == me->base){
        errorQuit("event base new error");
    }

    int fds[2];
    if(pipe(fds)){
        errorQuit("create pipe error");
    }
    me->notify_receive_fd = fds[0];
    me->notify_send_fd = fds[1];
    me->index = i;

    event_set( &me->notify_event, me->notify_receive_fd,
        EV_READ | EV_PERSIST, threadProcess, me );
    event_base_set(me->base, &me->notify_event);
    if (event_add(&me->notify_event, 0) == -1){
        errorQuit("Can't monitor libevent notify pipe\n");
    }
}

void *Server::workerLibevent(void *arg){
    LibeventThread *me = (LibeventThread*)arg;
    event_base_dispatch(me->base);
}

bool Server::startRun(int backlog){
    evconnlistener *listener;

#ifdef linux
    int cpu_num = get_nprocs();
    cpu_set_t cpu_info;
#endif

    if( m_port != EXIT_CODE ){
        sockaddr_in sin;
        memset(&sin, 0, sizeof(sin));
        sin.sin_family = AF_INET;
        sin.sin_port = htons(m_port);
        listener = evconnlistener_new_bind(m_main_base->base, 
          listenerEventCb, (void*)this,
          LEV_OPT_REUSEABLE|LEV_OPT_CLOSE_ON_FREE, -1,
          (sockaddr*)&sin, sizeof(sockaddr_in));
        if(NULL == listener){
            errorQuit("TCP listen error");
        }
    }
    for(int i=0; i<m_thread_count; i++){
        pthread_create(&m_threads[i].tid, NULL,  
            workerLibevent, (void*)&m_threads[i]);
    }

#ifdef linux
    if(m_cpu_affinity != true){
        cpu_num = 0;
    }else if(cpu_num > m_thread_count){
        cpu_num = m_thread_count;
    }
    for(int i=0; i<cpu_num; i++){
        CPU_ZERO(&cpu_info);
        CPU_SET(i, &cpu_info);
        if (0!=pthread_setaffinity_np(m_threads[i].tid, sizeof(cpu_set_t), &cpu_info)){
            errorQuit("set affinity failed");
        }
    }
#endif
    event_base_dispatch(m_main_base->base);
    if(m_port != EXIT_CODE){
        evconnlistener_free(listener);
    }
    return true;
}

void Server::stopRun(timeval *tv){
    int contant = EXIT_CODE;
    for(int i=0; i<m_thread_count; i++){
        write(m_threads[i].notify_send_fd, &contant, sizeof(int));
    }
    event_base_loopexit(m_main_base->base, tv);
}

void Server::listenerEventCb(struct evconnlistener *listener, 
                  evutil_socket_t fd,
                  struct sockaddr *sa, 
                  int socklen, 
                  void *user_data){
    static int last_thread = -1;
    Server *server = (Server*)user_data;
    int num = (last_thread + 1) % server->m_thread_count;
    last_thread = num;
    int sendfd = server->m_threads[num].notify_send_fd;
    write(sendfd, &fd, sizeof(evutil_socket_t));
}

void Server::threadProcess(int fd, short which, void *arg){
    LibeventThread *me = (LibeventThread*)arg;
    int pipefd = me->notify_receive_fd;
    evutil_socket_t confd;
    read(pipefd, &confd, sizeof(evutil_socket_t));
    if(EXIT_CODE == confd){
        event_base_loopbreak(me->base);
        return;
    }
    struct bufferevent *bev;
    bev = bufferevent_socket_new(me->base, confd, BEV_OPT_CLOSE_ON_FREE);
    if (!bev){
        fprintf(stderr, "Error constructing bufferevent!");
        event_base_loopbreak(me->base);
        return;
    }
    Conn *conn = me->connect_queue.insertConn(confd, me);
    bufferevent_setcb(bev, readEventCb, writeEventCb, closeEventCb, conn);
    //bufferevent_setwatermark(bev, EV_WRITE, 1, 10);
    bufferevent_enable(bev, EV_WRITE);
    bufferevent_enable(bev, EV_READ);
    me->tcp_connect->connectionEvent(conn);
}

void Server::readEventCb(struct bufferevent *bev, void *data){
    Conn *conn = (Conn*)data;
    conn->m_read_buf = bufferevent_get_input(bev);
    conn->m_write_buf = bufferevent_get_output(bev);
    conn->m_thread->tcp_connect->readEvent(conn);
} 

void Server::writeEventCb(struct bufferevent *bev, void *data){
    Conn *conn = (Conn*)data;
    conn->m_read_buf = bufferevent_get_input(bev);
    conn->m_write_buf = bufferevent_get_output(bev);
    conn->m_thread->tcp_connect->writeEvent(conn);
}

void Server::closeEventCb(struct bufferevent *bev, short events, void *data){
    Conn *conn = (Conn*)data;
    conn->m_thread->tcp_connect->closeEvent(conn, events);
    conn->getThread()->connect_queue.deleteConn(conn);
    bufferevent_free(bev);
}

bool Server::addSignalEvent(int sig, void (*ptr)(int, short, void*)){
    event *ev = evsignal_new(m_main_base->base, sig, ptr, (void*)this);
    if (!ev || event_add(ev, NULL) < 0 ){
        event_del(ev);
        return false;
    }
    deleteSignalEvent(sig);
    m_signal_events[sig] = ev;
    return true;
}

bool Server::deleteSignalEvent(int sig){
    map<int, event*>::iterator iter = m_signal_events.find(sig);
    if(iter == m_signal_events.end()){
        return false;
    }
    event_del(iter->second);
    m_signal_events.erase(iter);
    return true;
}

event *Server::addTimerEvent(void (*ptr)(int, short, void *), 
    timeval tv, bool once){
    int flag = 0;
    if(!once){
        flag = EV_PERSIST;
    }
    event *ev = new event;
    event_assign(ev, m_main_base->base, -1, flag, ptr, (void*)this);
    if( event_add(ev, &tv) < 0 ){
        event_del(ev);
        return NULL;
    }
    return ev;
}

bool Server::deleteTimerEvent(event *ev){
    int res = event_del(ev);
    return (0 == res);
}
