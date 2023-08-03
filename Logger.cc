#include "Logger.h"
#include "Timestamp.h"

#include <iostream>

//设置唯一的对象
Logger& Logger::instance()
{
     static Logger logger;
     return logger;
}

//设置日志等级
void Logger::setLogLevel(int level)
{
    Loglevel_=level;
}

//写日志,msg为日志信息
void Logger::log(std::string msg)
{
    switch(Loglevel_)
    {
    case INFO:
        std::cout<<"[INFO]";
        break;
    case ERROR:
        std::cout<<"[INFO]";
        break;
    case FATAL:
        std::cout<<"[INFO]";
        break;
    case DEBUG:
        std::cout<<"[INFO]";
        break;
    default:
       break;
    }

    //打印日志信息
    std::cout << Timestamp::now().toString() << " : " << msg <<std::endl;
}