# kvproxy
一个KV类型数据库的代理框架。可以通过编写扩展（so动态库）的方式增加对指定协议的支持。默认已经支持memcached的二进制协议和文本协议。

##特性

* 快速
* 轻量级
* 支持读写分离
* 支持数据的同步和异步复制
* 支持一致性哈希
* 支持failover机制。后端服务不可用时，自动摘除。
* 提供ini格式的配置文件
* 和后端服务器保持持久化连接
* 多线程模式，linux平台下支持cpu亲缘性
* 良好的协议扩展性。
* 默认支持memcached的二进制协议和文本协议

##文档
[kvproxy的master和slave数据主从复制简介](http://www.bo56.com/kvproxy%E7%9A%84%E6%95%B0%E6%8D%AE%E4%B8%BB%E4%BB%8E%E5%A4%8D%E5%88%B6%E7%AE%80%E4%BB%8B/)

[kvproxy配置文件之集群设置](http://www.bo56.com/kvproxy%E9%85%8D%E7%BD%AE%E6%96%87%E4%BB%B6%E4%B9%8B%E9%9B%86%E7%BE%A4%E8%AE%BE%E7%BD%AE/)

##安装
###安装环境
* autoconf版本>2.62     
* automake版本>1.13  
* g++版本>4.4

###安装步骤
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

##查看运行状态
可以通过telnet命令连接到kvproxy的监听端口，然后使用status命令查看kvproxy的运行状况。
```sh
$telnet 127.0.0.1 55669
Trying 127.0.0.1...
Connected to localhost.
Escape character is '^]'.
status

Number of processed requests [0]
Number of requests failed [0]
  - 127.0.0.1:11211 [0]
Number of continuous requests failed [0]
  - 127.0.0.1:11211 [0]
All of offline hosts [0] 
Number of client connection [1]
Number of backend connection [0]
Read timeout of backend connection [100ms]
Size of async queue [0]
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
; 一次请求过程中发生或者接收数据的最大尺寸。单位为M（兆）。如果数据超过指定大小，会导致请求失败。
; 如，你set或者get一个2M的数据，这个值就应该设置为2或者更大。
max_packet=1
; 失败次数超过指定阈值，则自动把有问题的后端服务器给摘除。
failover_threshold = 2
; 检测是否有后端服务器的失败次数大于指定阈值的频率。单位为秒。
failover_interval = 2
; 异步同步队列大小。看情况设置，不要设置太大。
async_size = 50000

;;;;;;;;;;;;;;;
;扩展部分设置 ;
;;;;;;;;;;;;;;;

; 扩展名称
[memcached]
; 扩展文件名。
extension=memcached.so
; 设置默认后端服务器组。
hosts=master
; 设置备份服务器组。所有的写操作命令都会同步到此服务器组一份。如果未设置，将不进行数据的同步或者异步复制。
;hosts_backup=read
; 设置读服务器组。所有的数据读取都从此服务器组读取。用于实现读写分离。
hosts_read=master
; 同步数据复制前缀符号。默认的数据复制为异步复制。如果想数据复制为同步复制，可以把key的前缀设置为指定字符串。
sync_str = "+" 
; 使用的协议类型 binary 二级制协议  text 文本协议
proto = binary

;;;;;;;;;;;;;;;;
; 服务器组设置 ;
;;;;;;;;;;;;;;;;

; 服务器组名
[master]
; ip : 端口号 = "在服务器组中的标示数字id : 权重"
; 同一组中每个后端服务器的标示数字id不能重复 
127.0.0.1:11211="1:50"

[read]
127.0.0.1:11212="1:50"
```

##问题反馈
如果发现了bug，或者有其他问题。可以到以下网址反馈。
https://github.com/exinnet/kvproxy/issues

##贡献者&感谢

* 许长敬 , http://github.com/ugg

