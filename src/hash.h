#ifndef KVPROXY_HASH_H
#define KVPROXY_HASH_H

#include <string.h>
#include <stdlib.h>
#include <iostream>
#include <utility>
#include <map>
#include <set>
#include <string>
#include <cstddef>
#include <stdint.h>
#include <stdio.h>
#include <pthread.h>
#include "log.h"

using namespace std;



class ConsistentHash
{
private:
    //the key is crc32 key, the value is a pair, host_port and alias index
    typedef map<uint32_t, pair<string,uint32_t> > hash_type;
    typedef hash_type::key_type key_type;
    typedef hash_type::mapped_type mapped_type;
    typedef hash_type::iterator iterator;
    hash_type _hash;
    hash_type _hash_tmp;
    pthread_mutex_t _lock;

public:
    ConsistentHash();
    ~ConsistentHash();
    bool createVNodes(map<pair<string,uint32_t>,pair<uint32_t,uint32_t> > hosts);
    void setHosts(map<pair<string,uint32_t>,pair<uint32_t,uint32_t> > hosts, bool force = false);
    bool delHost(uint32_t alias_index);
    bool addHost(string host_port, uint32_t alias_index, uint32_t index, uint32_t weight);
    pair<string,uint32_t> find(string key);
    pair<string,uint32_t> find(const char *key, uint16_t key_len);
    map<uint32_t, pair<string,uint32_t> > getAll();
    inline uint32_t getSize(){
        return _hash.size();
    }
};

uint32_t crc32(uint32_t crc, const char *buf, uint32_t len);

#endif
