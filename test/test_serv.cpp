#include "serv.h"
#include "log.h"
#include "conn_pool.h"
#include <set>
#include <vector>
using namespace std;

#define BUF_SIZE 10240000

class KvProxy : public Server
{
private:
    vector<Conn*> vec;
protected:
    void readEvent(Conn *conn);
    void writeEvent(Conn *conn);
    void connectionEvent(Conn *conn);
    void closeEvent(Conn *conn, short events);
public:
    KvProxy(int count) : Server(count) { };
    ~KvProxy();
  
    static void quitCb(int sig, short events, void *data);
    static void timeOutCb(int id, int short events, void *data);
};
KvProxy::~KvProxy(){
    printf("destruct \n");
}
void KvProxy::readEvent(Conn *conn)
{
    char buf[BUF_SIZE];
    int len = 0;
    len = conn->getReadBufLen();
    len = len > BUF_SIZE ? BUF_SIZE: len;
    conn->CopyReadBuf(buf, len);
    conn->clearReadBuf(len);
    conn->addToWriteBuf(buf, strlen(buf)); 

}

void KvProxy::writeEvent(Conn *conn)
{

}

void KvProxy::connectionEvent(Conn *conn)
{
  KvProxy *me = (KvProxy*)conn->getThread()->tcp_connect;
  printf("new connection: %d\n", conn->getFd());
  me->vec.push_back(conn);
}

void KvProxy::closeEvent(Conn *conn, short events)
{
  printf("connection closed: %d\n", conn->getFd());
}

void KvProxy::quitCb(int sig, short events, void *data)
{ 
  printf("Catch the SIGINT signal, quit in one second\n");
  KvProxy *me = (KvProxy*)data;
  timeval tv = {1, 0};
  me->stopRun(&tv);
}

void KvProxy::timeOutCb(int id, short events, void *data)
{
  KvProxy *me = (KvProxy*)data;
  char temp[33] = "hello, world\n";
  for(int i=0; i<me->vec.size(); i++)
    me->vec[i]->addToWriteBuf(temp, strlen(temp));
}

int main()
{
        #if defined(SIGPIPE) && defined(SIG_IGN)
                signal(SIGPIPE, SIG_IGN);
        #endif
  printf("pid: %d\n", getpid());
  KvProxy server(7);
  server.addSignalEvent(SIGINT, KvProxy::quitCb);
  timeval tv = {10, 0};
  server.addTimerEvent(KvProxy::timeOutCb, tv, false);
  server.setPort(2111);
  server.startRun();
  printf("done\n");
  
  return 0;
}
