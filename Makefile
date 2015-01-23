PREFIX=/usr/local/kvproxy

$(shell sh build.sh 1>&2)
include build_config.mk

all:
	@echo
	@echo "##### init folders ... #####"
	mkdir -p ./ext
	mkdir -p ./ect
	mkdir -p ./bin
	mkdir -p ./sbin
	mkdir -p ./log
	@echo "##### init folders finished #####"
	@echo
	@echo "##### building kvproxy ... #####"
	cd ./src; ${MAKE}
	@echo "##### building kvproxy finished #####"
	@echo
	@echo "##### building extension [memcached] ... #####"
	cd ./src/ext/memcached; ${MAKE}; ${MAKE} install
	@echo "##### building extension [memcached] finished #####"
	@echo
	@echo "##### building finished. #####"
	@echo
	@echo "##### if you want to install kvproxy to folder[${PREFIX}], you should run the command [make install] #####"
	@echo

install:
	mkdir -p ${PREFIX}
	mkdir -p ${PREFIX}/log
	cp -r ./bin ${PREFIX}
	cp -r ./sbin ${PREFIX}
	cp -r ./etc ${PREFIX}
	cp -r ./ext ${PREFIX}
	chmod 755 ${PREFIX}
	chmod +x -R ${PREFIX}/sbin
	chmod 777 ${PREFIX}/log
	@echo "install to folder $(PREFIX)  finished"

clean:
	cd "${LIBEVENT_PATH}"; ${MAKE} clean
	cd ./src; ${MAKE} clean
	cd ./src/ext/memcached; ${MAKE} clean
	rm -f ./build_config.mk
