#pragma once
#include "TaskScheduler.h"
#include <string>

class Daemon {
private:
    Daemon();
    
    Daemon(const Daemon&) = delete;
    Daemon& operator=(const Daemon&) = delete;

    bool is_running;
    std::string pid_file_path;
    TaskScheduler scheduler;

    // Внутренний метод проверки PID
    void checkPidFile();

public:
    // Получение единственного экземпляра
    static Daemon& getInstance();

    // Обработчик сигналов
    static void signalHandler(int sig);

    // Основной метод запуска
    void run(const std::string& configPath);
};