#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <fstream>
#include <ctime>
#include <syslog.h>
#include <algorithm>

namespace fs = std::filesystem;

// Структура задачи: откуда, куда, интервал, когда запускали
struct Task {
    std::string source;
    std::string dest;
    int interval;
    time_t last_run;
};

class TaskScheduler {
private:
    std::vector<Task> tasks;
    std::string configPath;

public:
    void setConfigPath(const std::string& path) {
        configPath = path;
    }

    // Чтение конфигурации
    void reloadConfig() {
        tasks.clear();
        std::ifstream file(configPath);
        
        if (!file.is_open()) {
            syslog(LOG_ERR, "Cannot open config file: %s", configPath.c_str());
            return;
        }

        std::string src, dst;
        int interval;
        while (file >> src >> dst >> interval) {
            Task t;
            t.source = src;
            t.dest = dst;
            t.interval = interval;
            t.last_run = 0; // выполнить при первой возможности
            tasks.push_back(t);
        }
        file.close();
        syslog(LOG_INFO, "Config reloaded. Tasks loaded: %lu", tasks.size());
    }

    // Основной процесс: проверка таймеров и перемещение
    void process() {
        time_t now = time(NULL);

        for (auto& task : tasks) {

            if (difftime(now, task.last_run) >= task.interval) {
                
                if (fs::exists(task.source) && fs::is_directory(task.source)) {
                    
                    if (!fs::exists(task.dest)) {
                        fs::create_directories(task.dest);
                    }

                    for (const auto& entry : fs::directory_iterator(task.source)) {
                        if (entry.is_regular_file()) {
                            try {
                                auto fileName = entry.path().filename();
                                auto destPath = fs::path(task.dest) / fileName;
                                
                                fs::rename(entry.path(), destPath);
                                
                                syslog(LOG_INFO, "Moved file %s to %s", 
                                       entry.path().c_str(), destPath.c_str());
                            } catch (const std::exception& e) {
                                syslog(LOG_ERR, "Error moving file: %s", e.what());
                            }
                        }
                    }
                }
                task.last_run = now;
            }
        }
    }
};