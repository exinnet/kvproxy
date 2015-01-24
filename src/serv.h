#ifndef KVPROXY_SERV_H
#define KVPROXY_SERV_H

#include <iostream>
#include <map>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <pthread.h>
#include <stdint.h>

#ifdef linux
    #include <sys/sysinfo.h>
#endif

#include <event.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/listener.h>
#include <event2/util.h>
#include <event2/event.h>

using namespace std;

class Server;
class Conn;
class ConnQueue;
struct LibeventThread;

class Conn{
    friend class ConnQueue;
    friend class Server;

private:
    const uint32_t  m_fd;
    evbuffer *m_read_buf;
    evbuffer *m_write_buf;

    Conn *m_prev;
    Conn *m_next;
    LibeventThread *m_thread;

    Conn(uint32_t fd=0);
    ~Conn();

public:
    LibeventThread *getThread() const { 
        return m_thread;
    }
    uint32_t  getFd() const {
        return m_fd;
    }
    //http://www.wangafu.net/~nickm/libevent-book/Ref7_evbuffer.html
    //http://blog.csdn.net/feitianxuxue/article/details/9386843
    size_t getReadBufLen() const { 
        return evbuffer_get_length(m_read_buf);
    }
    int clearReadBuf(uint32_t  len){
        return evbuffer_drain(m_read_buf, len);
    }
    int getReadBuf(char *buffer, uint32_t  len){ 
        return evbuffer_remove(m_read_buf, buffer, len);
    }
    ev_ssize_t CopyReadBuf(char *buffer, uint32_t  len){ 
        return evbuffer_copyout(m_read_buf, buffer, len);
    }
    size_t getWriteBufLen() const { 
        return evbuffer_get_length(m_write_buf);
    }
    int addToWriteBuf(const char *buffer, uint32_t  len){ 
        return evbuffer_add(m_write_buf, buffer, len);
    }
    void moveBufData(){ 
        evbuffer_add_buffer(m_write_buf, m_read_buf);
    }

};

class ConnQueue{
private:
    Conn *m_head;
    Conn *m_tail;
public:
    ConnQueue();
    ~ConnQueue();
    Conn *insertConn(uint32_t  fd, LibeventThread *t);
    void deleteConn(Conn *c);
};

struct LibeventThread{
    pthread_t tid;
    uint32_t index;
    struct event_base *base;
    struct event notify_event;
    uint32_t notify_receive_fd;
    uint32_t notify_send_fd;
    ConnQueue connect_queue;
    Server *tcp_connect;
};

class Server
{
private:
    uint32_t m_thread_count;
    uint32_t m_port;
    LibeventThread *m_main_base;
    LibeventThread *m_threads;
    map<int, event*> m_signal_events;
    bool m_cpu_affinity;

public:
    static const int  EXIT_CODE = -1;

private:
    void setupThread(LibeventThread *thread, uint32_t i);
    static void *workerLibevent(void *arg);
    static void threadProcess(int  fd, short which, void *arg);
    static void listenerEventCb(evconnlistener *listener, evutil_socket_t fd,
        sockaddr *sa, int  socklen, void *user_data);
    static void readEventCb(struct bufferevent *bev, void *data);
    static void writeEventCb(struct bufferevent *bev, void *data); 
    static void closeEventCb(struct bufferevent *bev, short events, void *data);

protected:
    virtual void connectionEvent(Conn *conn){}
    virtual void readEvent(Conn *conn){}
    virtual void writeEvent(Conn *conn){}
    virtual void closeEvent(Conn *conn, short events){}
    virtual void errorQuit(const char *str);

public:
    Server(uint32_t  count, bool cpu_affinity = true);
    virtual ~Server();
    void setPort(uint32_t  port){ 
        m_port = port;
    }
    void setThreadCount(uint32_t thread_count){
        m_thread_count = thread_count;
    }
    bool startRun(int backlog);
    void stopRun(timeval *tv);
    bool addSignalEvent(int  sig, void (*ptr)(int, short, void*));
    bool deleteSignalEvent(int  sig);
    event *addTimerEvent(void(*ptr)(int, short, void*),
        timeval tv, bool once);
    bool deleteTimerEvent(event *ev);
};

#endif
