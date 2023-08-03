#pragma once
#include <string>

#include "noncopyable.h"

// LOG_INFO("%s %d", arg1, arg2)
#define LOG_INFO(logMsgFormat,...) \
   do{\
    Logger& logger = Logger::instance();\
    logger.setLogLevel(INFO);/*把枚举当做实参传给整形实参*/  \
    char buf[1024]={0};\
    snprintf(buf,1024,logMsgFormat,##__VA_ARGS__);\
    logger.log(buf);\
   }while(0)

#define LOG_ERROR(logMsgFormat,...) \
   do{\
    Logger& logger = Logger::instance();\
    logger.setLogLevel(ERROR);/*把枚举当做实参传给整形实参*/  \
    char buf[1024]={0};\
    snprintf(buf,1024,logMsgFormat,##__VA_ARGS__);\
    logger.log(buf);\
   }while(0)

//出现了错误，不可挽回的错误，无法继续执行
#define LOG_FATAL(logMsgFormat,...) \
   do {\
    Logger& logger = Logger::instance();\
    logger.setLogLevel(FATAL);/*把枚举当做实参传给整形实参*/  \
    char buf[1024]={0};\
    snprintf(buf,1024,logMsgFormat,##__VA_ARGS__);\
    logger.log(buf);\
    exit(-1);\
   } while(0)

#ifndef MUDEUBG
#define LOG_DEBUG(logMsgFormat,...) \
   do{\
    Logger& logger = Logger::instance();\
    logger.setLogLevel(DEBUG);/*把枚举当做实参传给整形实参*/  \
    char buf[1024]={0};\
    snprintf(buf,1024,logMsgFormat,##__VA_ARGS__);\
    logger.log(buf);\
   }while(0)
#else
#define LOG_DEBUG(logMsgFormat,...)
#endif

//定义日志级别
enum LogLevel
{
   INFO,    //普通信息
   ERROR,   //错误信息
   FATAL,   //影响执行的core信息
   DEBUG,   //调试信息
};

//日志类，不可拷贝构造与拷贝赋值
class Logger : noncopyable
{
public:
   //获取日志实例对象
   static Logger& instance();
   //设置日志等级
   void setLogLevel(int level);
   //写日志,msg为日志信息
   void log(std::string msg);
private:
   int Loglevel_;//日志等级
   Logger(){}
};