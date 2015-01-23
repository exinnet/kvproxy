#include "config.h"
#include "log.h"

string Config::conf_path;
CONF_T Config::conf;
string Config::ext_name;

void Config::setConfFile(string file_path){
    conf_path = file_path;
}

void Config::loadConf(){
    ifstream cnf_file;
    char line[200];
    string cur_sec;
    string cur_key;
    string cur_val;
    int process = PARSE_INIT;
    int val_quote = 0; 
    cnf_file.open(conf_path.c_str());
    if(!cnf_file.is_open()){
        cerr << "fail to open config file. path is " << conf_path << endl;
        log_error("fail to open config file. path is %s" , conf_path.c_str());
        exit(EXIT_FAILURE);
    }
    while(cnf_file.good() && !cnf_file.eof()){
        memset(line,'\0',sizeof(line));
        val_quote = 0;
        process = PARSE_INIT;
        cur_key.clear();
        cur_val.clear();
        cnf_file.getline(line,sizeof(line));
        for(int i = PARSE_INIT; i < sizeof(line); i++){
            if(process == PARSE_INIT && line[i] == ';'){
                break;
            }else if(process == PARSE_INIT && line[i] <= 32){
                continue;
            }else if(process == PARSE_INIT && line[i] == '['){
                process = PARSE_SECTION;
                cur_sec.clear();
            }else if(process == PARSE_SECTION && line[i] == ']'){
                break;
            }else if(process == PARSE_SECTION){
                cur_sec += line[i];
            }else if(process == PARSE_INIT && line[i] > 32){
                process = PARSE_KEY;
                cur_key.clear();
                cur_key += line[i];
            }else if(process == PARSE_KEY && line[i] == '='){
                process = PARSE_VALUE;
            }else if(process == PARSE_KEY && line[i] > 32 ){
                cur_key += line[i];
            }else if(process == PARSE_VALUE && line[i] <= 32 && val_quote == 0){
                continue;
            }else if(process == PARSE_VALUE && line[i] == '"' && val_quote == 0){
                val_quote = 1;
                continue;
            }else if(process == PARSE_VALUE && line[i] == '"' && val_quote == 1){
                break;
            }else if(process == PARSE_VALUE){
                cur_val += line[i];
            }else if(line[i] == '\0'){
                break;
            }
        }
        if(!cur_key.empty() && !cur_val.empty()){
            conf[cur_sec][cur_key] = cur_val;
        }
        if (cur_sec != "kvproxy" && cur_key == "extension"){
            ext_name = cur_sec;
        }
    }
    if(!cnf_file.eof()){
        cerr << "fail to parse config file. path is " << conf_path << endl;
        log_error("fail to parse config file. path is %s" , conf_path.c_str());
        exit(EXIT_FAILURE);
    }
}

string Config::getConfStr(string section, string key){
    if(section.empty() || key.empty() || conf[section][key].empty()){
        return "";
    }else{
        return conf[section][key];
    }
}

map<string, string> Config::getConfStr(string section){
    map<string, string> ret;
    map<string, string>::iterator it;
    if(section.empty() || conf[section].empty()){
        return ret;
    }else{
        for(it = conf[section].begin(); it != conf[section].end(); it++){
            ret[it->first] = it->second;
        }
    }
    return ret;
}

int Config::getConfInt(string section, string key){
    int val;
    if(section.empty() || key.empty() || conf[section][key].empty()){
        return 0;
    }else{
        sscanf(conf[section][key].c_str(),"%d",&val);
        return val;
    }
}

map<string, int> Config::getConfInt(string section){
    int val;
    map<string, int> ret;
    CONF_T::mapped_type::iterator it;
    
    if(section.empty() || conf[section].empty()){
        return ret;
    }else{
        for(it = conf[section].begin(); it != conf[section].end(); it++){
            sscanf(conf[section][it->first].c_str(),"%d",&val);
            ret[it->first] = val;
        }
        return ret;
    }
}

bool Config::getConfBool(string section, string key){
    string val;
    if(section.empty() || key.empty() || conf[section][key].empty()){
        return false;
    }
    val = conf[section][key];
    if(val == "on" || val == "1" || val == "yes"){
        return true;
    }else{
        return false;
    }
}

map<string, bool> Config::getConfBool(string section){
    string val;
    map<string, bool> ret;
    CONF_T::mapped_type::iterator it;

    if(section.empty() || conf[section].empty()){
        return ret;
    }
    for(it = conf[section].begin(); it != conf[section].end(); it++){
        val = conf[section][it->first];
        if(val == "on" || val == "1" || val == "yes"){
            ret[it->first] = true;
        }else{
            ret[it->first] = false;
        }
    }
    return ret;
}

string Config::getExtName(){
    return ext_name;
}
