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

typedef ext_ret_t (*parse_req_t)(const char * const, int, int *,  req_list_t *);
typedef ext_ret_t (*create_req_t)(req_ptr_list_t * const, char *, int *);
typedef ext_ret_t (*create_req_async_t)(comm_list_t * const, char *, int *);
typedef ext_ret_t (*parse_resp_t)(const char * const, int, int *, resp_list_t *);
typedef ext_ret_t (*create_resp_t)(resp_list_t *, char *, int *, int *);
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
