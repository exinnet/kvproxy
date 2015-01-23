#ifndef KVPROXY_LOG_H
#define KVPROXY_LOG_H

#include <string.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <string>
#include <cstdarg>
#include <sys/time.h> 

using namespace std;

class Logger{
private:
    static ofstream log_file;
    int log_level;
public:
    static const int LEVEL_FATAL	= 0;
    static const int LEVEL_ERROR	= 1;
    static const int LEVEL_WARN		= 2;
    static const int LEVEL_INFO		= 3;
    static const int LEVEL_DEBUG	= 4;

    Logger();
    ~Logger();

    void setLevel(int level){
        log_level = level;
    }
    int getLevel(const char *levelname);
    void open(const char *filename);
    int logx(int level, const char *fmt, va_list ap);
    int debug(const char *fmt, ...);
    int info(const char *fmt, ...);
    int warn(const char *fmt, ...);
    int error(const char *fmt, ...);
    int fatal(const char *fmt, ...);
};


void log_open(const char *filename);
int get_log_level(const char * levelname);
void set_log_level(int level);
int log_write(int level, const char *fmt, ...);

#define log_debug(fmt, args...)	\
	log_write(Logger::LEVEL_DEBUG, "%s(%d): " fmt, __FILE__, __LINE__, ##args)
#define log_info(fmt, args...)	\
	log_write(Logger::LEVEL_INFO,  "%s(%d): " fmt, __FILE__, __LINE__, ##args)
#define log_warn(fmt, args...)	\
	log_write(Logger::LEVEL_WARN,  "%s(%d): " fmt, __FILE__, __LINE__, ##args)
#define log_error(fmt, args...)	\
	log_write(Logger::LEVEL_ERROR, "%s(%d): " fmt, __FILE__, __LINE__, ##args)
#define log_fatal(fmt, args...)	\
	log_write(Logger::LEVEL_FATAL, "%s(%d): " fmt, __FILE__, __LINE__, ##args)


#endif
