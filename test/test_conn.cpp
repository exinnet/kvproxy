#include "conn_pool.h"
using namespace std;

int main()
{
    int conn_fd;
    string host;
    uint32_t port;
    int max_size;
    host = "127.0.0.1";
    port = 80;
    max_size = 10;
    ssize_t len;
    string cmd;
    char buffer[10240] = {'\0'};
    int timeout = 1000000;//毫秒

    cmd = "stats";
    ConnPool *conn_pool = ConnPool::getInstance();
    conn_pool->initConnection(host, port, max_size);
    conn_fd = conn_pool->getConnection(1, host, port);
    cout << "conn fd is " << conn_fd << endl;
    len = send(conn_fd, cmd.c_str(), cmd.size(), MSG_DONTWAIT);
    if(len <= 0){
        cout << "send error" << len << endl;
    }else{
        cout << "send success " << len << endl;
    }
    while (1){
        fd_set rdfds;
        struct timeval tv;
        int ret; /* 保存返回值 */
        FD_ZERO(&rdfds); /* 用select函数之前先把集合清零 */
        FD_SET(conn_fd, &rdfds); /* 把要检测的句柄socket加入到集合里 */
        tv.tv_sec = 0;
        tv.tv_usec = timeout; /* 设置select等待的最大时间为1秒加500微秒 */
        cout << "start select " << endl;
        ret = select(conn_fd+1, &rdfds, NULL, NULL, &tv);
        if(ret < 0){
            cout << "select error " << endl;
            return 0;
        }else if (ret == 0){
            cout << "select timeout " << endl;
            return 0;
        }else if (FD_ISSET(conn_fd,&rdfds) <= 0){
            cout << "can not read" << endl;
            return 0;
        }

		len = recv(conn_fd, &buffer, sizeof(buffer), MSG_WAITALL);//MSG_PEEK MSG_DONTWAIT MSG_WAITALL
		if (len > 0)
		{
			cout << "buffer is " << buffer << endl;
            break;
		}else if(len == 0){
			cout << "close connection \n" << endl;
			close(conn_fd);
		}else if (len == -1)
		{
			if (errno == EAGAIN)
			{
				break;
			}else if (errno == EINTR)
			{
				continue;
			}else{
				break;
			}
		}
	}
    return 0;
}
