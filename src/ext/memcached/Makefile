CXX=g++

OUTPUT=./

LIB_SUFFIX_NAME=so
TARGET=memcached.${LIB_SUFFIX_NAME}

CPPFLAGS= -g -O2 -fPIC -shared\
	-I../../

SOURCE_FILES=./memcached.cpp
HEADER_FILES=./memcached.h ./protocol_binary.h

all:$(SOURCE_FILES) $(HEADER_FILES)
	$(CXX) -o $(TARGET) $(SOURCE_FILES)  $(CPPFLAGS) 
clean:
	rm -rf $(TARGET)
install: 
	cp $(TARGET) ../../../ext/
uninstall: 
	rm -f ../../../ext/$(TARGET)
