#include "Daemon.h"
#include <iostream>
#include <filesystem>

int main(int argc, char* argv[]) {
    // Путь к конфигу по умолчанию
    std::string configPath = "config.txt";

    if (argc > 1) {
        configPath = argv[1];
    } else {
        if (!std::filesystem::exists(configPath)) {
            configPath = std::filesystem::current_path() / "config.txt";
        }
    }

    try {
        if (std::filesystem::exists(configPath)) {
            configPath = std::filesystem::absolute(configPath);
        } else {
            std::cerr << "Error: Config file not found at " << configPath << std::endl;
            return 1;
        }
    } catch (...) {
        std::cerr << "Error resolving config path" << std::endl;
        return 1;
    }

    // Запуск демона
    Daemon::getInstance().run(configPath);

    return 0;
}