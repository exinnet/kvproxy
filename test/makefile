kvproxy:../src/kvproxy.cpp ../src/extension.cpp ../src/extension.h ../src/hash.cpp ../src/hash.h ../src/serv.cpp ../src/serv.h  ../src/conn_pool.cpp ../src/conn_pool.h ../src/log.cpp ../src/log.h ../src/config.cpp ../src/config.h ../src/util.cpp ../src/util.h
	g++ ../src/kvproxy.cpp ../src/extension.cpp ../src/hash.cpp ../src/serv.cpp ../src/conn_pool.cpp ../src/log.cpp ../src/util.cpp ../src/config.cpp -I ../src/ -I /home/admin/libevent2/include -L /home/admin/libevent2/lib -ldl -levent -Wall -g -O2 -o kvproxy
hash:../src/hash.cpp ../src/hash.h ./test_hash.cpp
	g++ ../src/hash.cpp ./test_hash.cpp -I ../src/ -g -o hash
config:../src/config.cpp ../src/config.h ../src/log.h ./test_config.cpp
	g++ ../src/config.cpp  ../src/log.cpp  ./test_config.cpp -I ../src/ -g -o config
serv:../src/serv.cpp ../src/serv.h ./test_serv.cpp
	g++ ../src/serv.cpp ./test_serv.cpp -I ../src/ -L /home/admin/libevent2/lib  -levent -g -o serv
ext:../src/extension.cpp ../src/extension.h ./test_ext.cpp
	g++ ../src/extension.cpp ./test_ext.cpp -I ../src/ -ldl -g -o ext
util:../src/util.cpp ../src/util.h ./test_util.cpp
	g++ ../src/util.cpp ./test_util.cpp -I ../src/ -g -o util
conn:../src/conn_pool.cpp ../src/conn_pool.h ./test_conn.cpp
	g++ ../src/conn_pool.cpp ./test_conn.cpp ../src/log.cpp -I ../src/ -lpthread -g -o conn
log:../src/log.cpp ../src/log.h ./test_log.cpp
	g++ ../src/log.cpp ./test_log.cpp -I ../src/ -g -o log
clean:
	rm core.*
