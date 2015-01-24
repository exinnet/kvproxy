# kvproxy
一个KV类型数据库的代理框架。可以通过编写扩展（so动态库）的方式增加对指定协议的支持。默认已经支持memcached的二进制协议和文本协议。

##特性

* 快速
* 轻量级
* 和后端服务器保持持久化连接
* 支持读写分离
* 支持数据的同步和异步复制
* 支持一致性哈希
* 支持failover机制。后端服务不可用时，自动摘除。
* 提供ini格式的配置文件
* 多线程模式，linux平台下支持cpu亲缘性
* 良好的协议扩展性。可以通过
* 默认支持memcached的二进制协议和文本协议

##安装

```sh
$ wget --no-check-certificate https://github.com/exinnet/kvproxy/archive/master.zip
$ unzip master
$ cd kvproxy-master
$ make
$ #optional, install kvproxy in /usr/local/kvproxy
$ sudo make install

```

##启动与关闭
```sh
# 启动
$ ./sbin/kvproxy start

# 关闭
$ ./sbin/kvproxy stop

# 重启
$ ./sbin/kvproxy restart
```

##性能
* 单线程  6000qps
* 开启cpu亲缘性 8个线程  50000qps

##配置
kvproxy的配置文件使用的时ini格式。默认文件在安装目录的etc文件夹下。文件名为 kvproxy.ini。
```ini
[kvproxy]
; 监听的端口号
port=55669
; 开启的线程数。在linux平台下，如果cpu_affinity设置为on，开启的线程数自动为cpu内核数量。
thread_count=2
; 是否开启cpu亲缘性
cpu_affinity=on
; 等待被处理的连接数
backlog=10000
; 从后端服务器读取数据超时时间，单位 毫秒
recv_timeout=100
; 日志文件路径，默认为安装目录 ./log/kvproxy.log
;log_path=/tmp/kvproxy.log
; 错误级别。选项有 fatal error warn info debug。
; fatal 为致命错误。发生致命错误会导致kvproxy进程退出。
; error 为请求错误。发生请求错误会导致本次请求失败。
; warn  为请求警告。发生请求警告说明本次请求过程中出现了异常，可能请求会成功。
; info  为冗余模式。会打印出请求执行过程中的一些冗余信息。
; debug 为调试模式。会打印出请求过程中的一些调试信息。
log_level=warn
; 扩展安装路径。默认为安装目录下的 ./ext
;ext_path=../ext
;unit MB
max_packet=1
failover_threshold = 2
;unit second 
failover_interval = 2
async_size = 50000

[memcached]
extension=memcached.so
hosts=master
;hosts_backup=read
hosts_read=master
sync_str = "+" 

[master]
127.0.0.1:11211="1:50"

[read]
127.0.0.1:11212="2:50"
```
