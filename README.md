# kvproxy
一个KV类型数据库的代理框架。可以通过编写扩展（so）的方式增加对指定协议的支持。默认已经支持memcached协议。

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
* 良好的协议扩展性
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
