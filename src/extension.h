#ifndef KVPROXY_EXT_H
#define KVPROXY_EXT_H

#include "config.h"
#include "dlfcn.h"
#include "stdint.h"
#include "util.h"
#include <map>
#include <set>
#include <string>
#include <iostream>
#include <list>

#define EXT_VER 20140911

#define PARSE_REQ_PARAM get_conf_t get_conf, const char * const origin_req, int size, int * position, req_list_t * req_data
#define CREATE_REQ_PARAM get_conf_t get_conf, req_ptr_list_t * const req_data, char * req_data_to_backend, int * length
#define CREATE_REQ_ASYNC_PARAM get_conf_t get_conf, comm_list_t * const req_data, char * req_data_to_backend, int * length
#define PARSE_RESP_PARAM get_conf_t get_conf, const char * const origin_resp, int size, int * position, resp_list_t * resp_data
#define CREATE_RESP_PARAM get_conf_t get_conf, resp_list_t * const resp_data, char * resp_data_to_client, int * length, int * size
#define PARSE_REQ(ext_name) ext_ret_t ext_name##_parse_req(PARSE_REQ_PARAM)
#define CREATE_REQ(ext_name) ext_ret_t ext_name##_create_req(CREATE_REQ_PARAM)
#define CREATE_REQ_ASYNC(ext_name) ext_ret_t ext_name##_create_req_async(CREATE_REQ_ASYNC_PARAM)
#define PARSE_RESP(ext_name) ext_ret_t ext_name##_parse_resp(PARSE_RESP_PARAM)
#define CREATE_RESP(ext_name) ext_ret_t ext_name##_create_resp(CREATE_RESP_PARAM)
#define EXT_VERSION(ext_name) int ext_name##_ext_version()

using namespace std;

typedef enum {
    AGAIN = 1,
    SUCCESS,
    ERROR,
    IGNORE
} ext_ret_t;

typedef struct {
    const char *key;
    uint16_t key_len;
    const char *content;
    uint32_t content_len;
    bool update;
    bool each;
    char ext[8];
} req_t;

typedef struct {
    const char * content;
    uint32_t content_len;
    char ext[8];
} resp_t;

typedef struct {
    string content;
    char ext[8];
} comm_t;

typedef list<req_t> req_list_t;
typedef list<req_t *> req_ptr_list_t;
typedef list<resp_t> resp_list_t;
typedef list<resp_t *> resp_ptr_list_t;
typedef list<comm_t> comm_list_t;

typedef string (*get_conf_t)(string section, string key);
typedef ext_ret_t (*parse_req_t)(PARSE_REQ_PARAM);
typedef ext_ret_t (*create_req_t)(CREATE_REQ_PARAM);
typedef ext_ret_t (*create_req_async_t)(CREATE_REQ_ASYNC_PARAM);
typedef ext_ret_t (*parse_resp_t)(PARSE_RESP_PARAM);
typedef ext_ret_t (*create_resp_t)(CREATE_RESP_PARAM);
typedef int (*ext_version_t)();

class Extension
{
private:
    static map<string,parse_req_t> parse_req;
    static map<string,create_req_t> create_req;
    static map<string,create_req_async_t> create_req_async;
    static map<string,parse_resp_t> parse_resp;
    static map<string,create_resp_t> create_resp;
    static map<string,ext_version_t> ext_version;
    static string ext_path;
public:
    Extension(){};
    ~Extension(){};
    void setExtPath(string path);
    bool load(string ext_name, string filename);
    parse_req_t findParseReq(string ext_name);
    create_req_t findCreateReq(string ext_name);
    create_req_async_t findCreateReqAsync(string ext_name);
    parse_resp_t findParseResp(string ext_name);
    create_resp_t findCreateResp(string ext_name);
    ext_version_t findExtVersion(string ext_name);
};

#endif
