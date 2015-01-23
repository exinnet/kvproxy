#include "log.h"
using namespace std;

int main()
{
    string filename;
    int level;
    filename = "./test.log";
    log_open(filename.c_str());
    level = get_log_level("debug");
    set_log_level(level);
    log_error("%s"," some errors");
    return 0;
}
