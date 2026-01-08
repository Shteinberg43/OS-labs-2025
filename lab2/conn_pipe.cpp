#include "Conn.h"
#include <unistd.h>
#include <iostream>
#include <cstdio>
#include <cstdlib>

class ConnPipe : public Conn {
private:
    // fd_down: родитель пишет [1] - ребенок читает [0]
    int fd_down[2]; 
    // fd_up: ребенок пишет [1] - родитель читает [0]
    int fd_up[2];   

    int read_fd = -1;
    int write_fd = -1;

public:
    ConnPipe() {
        // создаем два канала
        if (pipe(fd_down) < 0 || pipe(fd_up) < 0) {
            perror("pipe error");
            exit(1);
        }
    }

    void OnFork(bool is_parent) override {
        if (is_parent) {
            // хост
            close(fd_down[0]); // не читаем из канала "вниз"
            close(fd_up[1]);   // не пишем в канал "вверх"
            
            write_fd = fd_down[1]; // пишем сюда
            read_fd = fd_up[0];    // читаем отсюда
        } else {
            // клиент
            close(fd_down[1]); // не пишем в канал "вниз"
            close(fd_up[0]);   // не читаем из канала "вверх"
            
            read_fd = fd_down[0];  // читаем отсюда
            write_fd = fd_up[1];   // пишем сюда
        }
    }

    bool Read(void* buf, size_t count) override {
        size_t bytes_read = 0;
        char* ptr = (char*)buf;
        while (bytes_read < count) {
            ssize_t res = read(read_fd, ptr + bytes_read, count - bytes_read);
            if (res <= 0) return false; // Ошибка или конец файла
            bytes_read += res;
        }
        return true;
    }

    bool Write(const void* buf, size_t count) override {
        size_t bytes_written = 0;
        const char* ptr = (const char*)buf;
        while (bytes_written < count) {
            ssize_t res = write(write_fd, ptr + bytes_written, count - bytes_written);
            if (res < 0) return false;
            bytes_written += res;
        }
        return true;
    }

    void Close() override {
        if (read_fd != -1) close(read_fd);
        if (write_fd != -1) close(write_fd);
    }
};

// фабрики для host_pipe
Conn* CreateConn() {
    return new ConnPipe();
}