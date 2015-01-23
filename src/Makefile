include ../build_config.mk

SOURCE_FILES=./kvproxy.cpp ./extension.cpp ./hash.cpp ./serv.cpp ./conn_pool.cpp ./log.cpp ./config.cpp ./util.cpp
HEADER_FILES=./kvproxy.h ./extension.h ./hash.h ./serv.h ./conn_pool.h ./log.h ./config.h ./util.h

all:${SOURCE_FILES} ${HEADER_FILES}
	${CXX} ${SOURCE_FILES} -I ./ ${CFLAGS} ${PLATFORM_CLIBS} ${CLIBS}  -o ../bin/kvproxy
clean:
	rm -f ../bin/kvproxy
