#pragma once
#include <cstddef>

class Conn {
public:
    virtual ~Conn() = default;

    // чтение и запись
    virtual bool Read(void* buf, size_t count) = 0;
    virtual bool Write(const void* buf, size_t count) = 0;

    // метод вызывается сразу после fork
    // is_parent = true для Хоста (волка), false для Клиента (козленка)
    virtual void OnFork(bool is_parent) {} 

    virtual void Close() {}
};

// фабричная функция, которая будет реализована в conn_.cpp
Conn* CreateConn();