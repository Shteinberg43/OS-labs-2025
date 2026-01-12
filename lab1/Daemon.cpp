#include "Daemon.h"
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <fstream>
#include <csignal>
#include <syslog.h>
#include <filesystem>

namespace fs = std::filesystem;

Daemon::Daemon() : is_running(false) {}

Daemon& Daemon::getInstance() {
    static Daemon instance;
    return instance;
}

// Реализация проверки PID
void Daemon::checkPidFile() {
    pid_file_path = "/tmp/lab1_daemon.pid"; 
    
    std::ifstream pidFile(pid_file_path);
    if (pidFile.is_open()) {
        int old_pid;
        pidFile >> old_pid;
        pidFile.close();
        
        // Проверяем через /proc, жив ли процесс
        if (fs::exists("/proc/" + std::to_string(old_pid))) {
            syslog(LOG_WARNING, "Daemon already running (PID: %d). Stopping it.", old_pid);
            kill(old_pid, SIGTERM); 
            sleep(2); 
        }
    }
    
    // Записываем свой PID
    std::ofstream newPid(pid_file_path);
    if (newPid.is_open()) {
        newPid << getpid();
        newPid.close();
    } else {
        syslog(LOG_ERR, "Failed to write PID file");
    }
}

// Реализация обработчика сигналов
void Daemon::signalHandler(int sig) {
    switch(sig) {
        case SIGHUP:
            syslog(LOG_INFO, "Received SIGHUP. Reloading config.");
            getInstance().scheduler.reloadConfig();
            break;
        case SIGTERM:
            syslog(LOG_INFO, "Received SIGTERM. Exiting.");
            getInstance().is_running = false;
            fs::remove(getInstance().pid_file_path);
            break;
    }
}

// Реализация основного цикла запуска
void Daemon::run(const std::string& configPath) {

    pid_t pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) exit(EXIT_SUCCESS);

    if (setsid() < 0) exit(EXIT_FAILURE);

    signal(SIGHUP, signalHandler);
    signal(SIGTERM, signalHandler);
    signal(SIGCHLD, SIG_IGN);

    umask(0);
    chdir("/");
    
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    openlog("lab1_daemon", LOG_PID, LOG_DAEMON);
    syslog(LOG_INFO, "Daemon started");

    checkPidFile();
    
    scheduler.setConfigPath(configPath);
    scheduler.reloadConfig();
    
    is_running = true;

    while(is_running) {
        scheduler.process();
        sleep(1);
    }

    closelog();
}