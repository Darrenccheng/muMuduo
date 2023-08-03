#pragma once

/*
   此方式让派生类可以调用构造函数和析构函数，但派生类无法进行拷贝构造和拷贝赋值
   delete含义：虽然声明了她，但不能以任何方式调用
*/
class noncopyable{
private:
    noncopyable(const noncopyable&) =delete; //阻止拷贝
    noncopyable& operator=(const noncopyable&) =delete; //阻止赋值
protected: 
    noncopyable() =default;
    ~noncopyable() =default;
};