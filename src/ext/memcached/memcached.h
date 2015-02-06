#ifndef EXPROXY_EXT_MEMCACHED_H
#define EXPROXY_EXT_MEMCACHED_H

#include "extension.h"
#include "util.h"
#include "config.h"
#include <arpa/inet.h>
#include "protocol_binary.h"

extern "C" {
    struct memcached_header{
        int keylen;
        int extlen;
        int bodylen;
        int opcode;
    };
    PARSE_REQ(memcached);
    CREATE_REQ(memcached);
    CREATE_REQ_ASYNC(memcached);
    PARSE_RESP(memcached);
    CREATE_RESP(memcached);
    EXT_VERSION(memcached);

    static bool is_binary_protocol(const char * data, int size);
    static int try_read_request_binary(const char * req_data, int size, memcached_header * header);
    static int try_read_response_binary(const char * resp_data, int size, memcached_header * header);
    static int try_read_request_text(const char * req_data, int size, memcached_header * header);
    static int try_read_response_text(const char * resp_data, int size, memcached_header * header);
    static uint32_t get_opcode(const char * data);

}
#endif
