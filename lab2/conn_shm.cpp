#include "Conn.h"
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstring>
#include <iostream>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>

class ConnShm : public Conn {
private:
    void* shared_mem = nullptr;
    const size_t SIZE = 1024;
    const char* SHM_NAME = "/lab2_game_memory"; // имя объекта в /dev/shm

public:
    ConnShm() {
        // в конструкторе ничего не делаем, инициализация будет в OnFork
    }

    // ТЗ требует открывать память в каждом процессе отдельно
    void OnFork(bool is_parent) override {
        int fd = -1;

        if (is_parent) {
            // родитель создает память
            shm_unlink(SHM_NAME); // удаляем старую, если была
            fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
            if (fd < 0) { perror("shm_open parent"); exit(1); }
            
            // Задаем размер
            if (ftruncate(fd, SIZE) == -1) { perror("ftruncate"); exit(1); }
        } else {
            // ребенок открывает существующую
            // задержка, чтобы родитель успел создать
            usleep(100000); // 0.1 с
            fd = shm_open(SHM_NAME, O_RDWR, 0666);
            if (fd < 0) { perror("shm_open child"); exit(1); }
        }

        // отображаем в память
        shared_mem = mmap(NULL, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        close(fd);

        if (shared_mem == MAP_FAILED) {
            perror("mmap shm failed");
            exit(1);
        }
    }

    bool Read(void* buf, size_t count) override {
        if (!shared_mem) return false;
        std::memcpy(buf, shared_mem, count);
        return true;
    }

    bool Write(const void* buf, size_t count) override {
        if (!shared_mem) return false;
        std::memcpy(shared_mem, buf, count);
        return true;
    }

    void Close() override {
        if (shared_mem) munmap(shared_mem, SIZE);
        // удаляем объект из системы 
    }
};

// фабрика для host_shm
Conn* CreateConn() {
    return new ConnShm();
}