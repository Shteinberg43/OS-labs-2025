#include "Conn.h"
#include <sys/mman.h>
#include <cstring>
#include <iostream>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>

class ConnMmap : public Conn {
private:
    void* shared_mem;
    const size_t SIZE = 1024; // размер буфера для сообщений

public:
    ConnMmap() {
        // MAP_ANONYMOUS | MAP_SHARED - память видна родственным процессам
        shared_mem = mmap(NULL, SIZE, PROT_READ | PROT_WRITE,
                          MAP_SHARED | MAP_ANONYMOUS, -1, 0);
        
        if (shared_mem == MAP_FAILED) {
            perror("mmap anonymous failed");
            exit(1);
        }
    }

    // в mmap копируем данные в общую память
    bool Read(void* buf, size_t count) override {
        if (count > SIZE) return false;
        std::memcpy(buf, shared_mem, count);
        return true;
    }

    bool Write(const void* buf, size_t count) override {
        if (count > SIZE) return false;
        std::memcpy(shared_mem, buf, count);
        return true;
    }

    void Close() override {
        munmap(shared_mem, SIZE);
    }
};

// фабрика для host_mmap
Conn* CreateConn() {
    return new ConnMmap();
}