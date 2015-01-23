#include "extension.h"

map<string,parse_req_t> Extension::parse_req;
map<string,create_req_t> Extension::create_req;
map<string,create_req_async_t> Extension::create_req_async;
map<string,parse_resp_t> Extension::parse_resp;
map<string,create_resp_t> Extension::create_resp;
map<string,ext_version_t> Extension::ext_version;
string Extension::ext_path;

void Extension::setExtPath(string path){
    ext_path = path;
}
bool Extension::load(string ext_name, string filename){
    string ext_full_path;
    string func_name;
    void *handle;
    void *sym;

    if(ext_path.empty()){
        cerr << "please specify extension path " << endl;
        return false;
    }

    ext_full_path = ext_path + "/" + filename;
    handle = dlopen(ext_full_path.c_str(), RTLD_LAZY);
    if(!handle){
        cerr << "fail to load " << dlerror() << endl;
        return false;
    }

    func_name.clear();
    func_name = ext_name + "_parse_req";
    sym = dlsym(handle, func_name.c_str());
    if(sym != NULL){
        parse_req[ext_name] = (parse_req_t)sym;
    }else{
        cerr << "fail to load sym " << func_name << endl;
    }

    func_name.clear();
    func_name = ext_name + "_create_req";
    sym = dlsym(handle, func_name.c_str());
    if(sym != NULL){
        create_req[ext_name] = (create_req_t)sym;
    }else{
        cerr << "fail to load sym " << func_name << endl;
    }

    func_name.clear();
    func_name = ext_name + "_create_req_async";
    sym = dlsym(handle, func_name.c_str());
    if(sym != NULL){
        create_req_async[ext_name] = (create_req_async_t)sym;
    }else{
        cerr << "fail to load sym " << func_name << endl;
    }
    
    func_name.clear();
    func_name = ext_name + "_parse_resp";
    sym = dlsym(handle, func_name.c_str());
    if(sym != NULL){
        parse_resp[ext_name] = (parse_resp_t)sym;
    }else{
        cerr << "fail to load sym " << func_name << endl;
    }

    func_name.clear();
    func_name = ext_name + "_create_resp";
    sym = dlsym(handle, func_name.c_str());
    if(sym != NULL){
        create_resp[ext_name] = (create_resp_t)sym;
    }else{
        cerr << "fail to load sym " << func_name << endl;
    }

    func_name.clear();
    func_name = ext_name + "_ext_version";
    sym = dlsym(handle, func_name.c_str());
    if(sym != NULL){
        ext_version[ext_name] = (ext_version_t)sym;
    }else{
        cerr << "fail to load sym " << func_name << endl;
    }

    return true;
    //dlclose(handle);
}

parse_req_t Extension::findParseReq(string ext_name){
    map<string,parse_req_t>::iterator it;
    it = parse_req.find(ext_name);
    if(it == parse_req.end()){
        return NULL;
    }
    return it->second;
}

create_req_t Extension::findCreateReq(string ext_name){
    map<string,create_req_t>::iterator it;
    it = create_req.find(ext_name);
    if(it == create_req.end()){
        return NULL;
    }
    return it->second;
}

create_req_async_t Extension::findCreateReqAsync(string ext_name){
    map<string,create_req_async_t>::iterator it;
    it = create_req_async.find(ext_name);
    if(it == create_req_async.end()){
        return NULL;
    }
    return it->second;
}

parse_resp_t Extension::findParseResp(string ext_name){
    map<string,parse_resp_t>::iterator it;
    it = parse_resp.find(ext_name);
    if(it == parse_resp.end()){
        return NULL;
    }
    return it->second;
}

create_resp_t Extension::findCreateResp(string ext_name){
    map<string,create_resp_t>::iterator it;
    it = create_resp.find(ext_name);
    if(it == create_resp.end()){
        return NULL;
    }
    return it->second;
}

ext_version_t Extension::findExtVersion(string ext_name){
    map<string,ext_version_t>::iterator it;
    it = ext_version.find(ext_name);
    if(it == ext_version.end()){
        return NULL;
    }
    return it->second;
}
