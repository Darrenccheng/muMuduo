#设置版本号，不低于2.5
CMAKE_MINIMUM_REQUIRED(VERSION 2.5)
#设置工程名
project(mymuduo)

#定义配指信息
#设置编译后的类型（动态库）、路径（工程根目录的lib目录下）
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
#设置调试信息(设置c++编译选项)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++11 -fPIC")

#定义参与编译的源文件
aux_source_directory(. SRC_LIST)
#编译生成动态库
add_library(mymuduo SHARED ${SRC_LIST})