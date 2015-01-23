#ifndef KVPROXY_CONFIG_H
#define KVPROXY_CONFIG_H

#include <string>
#include <iostream>
#include <fstream>
#include <map>
#include "log.h"
#include "util.h"

#define PARSE_INIT 0
#define PARSE_SECTION 1
#define PARSE_KEY 2
#define PARSE_VALUE 3

using namespace std;

typedef std::map<std::string,std::map<std::string,std::string> > CONF_T;

class Config
{
private:
    static string conf_path;
    static CONF_T conf;
    static string ext_name;

public:
    static void setConfFile(string file_path);
    static void loadConf();
    static string getConfStr(string section, string key);
    static map<string, string> getConfStr(string section);
    static int getConfInt(string section, string key);
    static map<string, int> getConfInt(string section);
    static bool getConfBool(string section, string key);
    static map<string, bool> getConfBool(string section);
    static string getExtName();
};

#endif
