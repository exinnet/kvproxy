#ifndef EXPROXY_EXT_MEMCACHED_H
#define EXPROXY_EXT_MEMCACHED_H

#include "extension.h"
#include "util.h"
#include "config.h"
#include <arpa/inet.h>
#include "protocol_binary.h"

#define PROTO_BINARY false

extern "C" {
    struct memcached_header{
        int keylen;
        int extlen;
        int bodylen;
        int opcode;
    };
    ext_ret_t memcached_parse_req(const char * const origin_req, int size, int * position, req_list_t * req_data);
    ext_ret_t memcached_create_req(req_ptr_list_t * const req_data, char * req_data_to_backend, int * length);
    ext_ret_t memcached_create_req_async(comm_list_t * const req_data, char * req_data_to_backend, int * length);
    ext_ret_t memcached_parse_resp(const char * const origin_resp, int size, int * position, resp_list_t * resp_data);
    ext_ret_t memcached_create_resp(resp_list_t * const resp_data, char * resp_data_to_client, int * length, int * size);
    int memcached_ext_version();

    static bool is_binary_protocol(const char * data, int size);
    static int try_read_request_binary(const char * req_data, int size, memcached_header * header);
    static int try_read_response_binary(const char * resp_data, int size, memcached_header * header);
    static int try_read_request_text(const char * req_data, int size, memcached_header * header);
    static int try_read_response_text(const char * resp_data, int size, memcached_header * header);
    static uint32_t get_opcode(const char * data);

}
#endif
