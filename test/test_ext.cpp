#include "extension.h"

int main(){
    Extension ext;
    parse_req_t ptr_parse_req;
    ext_ret rt;
    map<string,string> data;
    char cmd[20] = {'\0'};
    char key[500] = {'\0'};
    uint32_t expires = 0;
    bool update = false;
    char req_data[1024] = {'\0'};
    ext.setExtPath("../src/ext/demo/");
    ext.load("demo");
    ptr_parse_req = ext.findParseReq("demo");
    if(ptr_parse_req != NULL){
        rt = ptr_parse_req(&data, cmd, key, expires, update, req_data);
        cout << "cmd " << cmd << endl;
        cout << "data[key] " << data["key"] << endl;
    }else{
        cout << "no find parse req" << endl;
    }
}
