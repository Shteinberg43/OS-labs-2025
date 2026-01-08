#include "Conn.h"
#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <fcntl.h>
#include <cstring>
#include <random>
#include <ctime>
#include <cerrno>
#include <sys/select.h>

// структура состояния игры
struct GameState {
    sem_t sem_start_turn; 
    sem_t sem_made_move;  
    sem_t sem_result_ready; 
    sem_t sem_next_round; 

    bool game_running;
};

struct MsgFromClient {
    int number;
};

struct MsgToClient {
    bool is_alive;
    bool game_over;
};

//функция ожидания семафора с таймаутом (5 сек)
bool WaitWithTimeout(sem_t* sem, const char* waiter_name) {
    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) == -1) return false;
    ts.tv_sec += 5;

    if (sem_timedwait(sem, &ts) == -1) {
        if (errno == ETIMEDOUT) {
            std::cerr << "[" << waiter_name << "] Timeout exceeded (5 sec)! Connection lost." << std::endl;
        }
        return false;
    }
    return true;
}

// ввод числа волка
int GetWolfInputOrRandom() {
    std::cout << "Wolf, enter your number (1-100) [You have 3 sec]: " << std::flush;

    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);

    struct timeval tv;
    tv.tv_sec = 3;
    tv.tv_usec = 0;

    // ждем ввода
    int ret = select(STDIN_FILENO + 1, &readfds, NULL, NULL, &tv);

    if (ret > 0) {
        // ввели
        int num;
        if (std::cin >> num) {
            return num;
        }
    } else if (ret == 0) {
        std::cout << "\nTime is up! Picking random number..." << std::endl;
    } else {
        perror("select");
    }

    // если не успели или ошибка - случайное число
    return 1 + std::rand() % 100;
}

// логика козленка (Client)
void ClientLogic(Conn* conn, GameState* state) {
    std::srand(getpid() + time(NULL));
    bool is_alive = true;

    while (true) {
        // ждем начала раунда
        if (!WaitWithTimeout(&state->sem_start_turn, "Kid")) break;
        if (!state->game_running) break;

        // козленок всегда играет автоматически, игрок управляет волком
        int max_val = is_alive ? 100 : 50;
        int my_number = 1 + std::rand() % max_val;

        MsgFromClient msg;
        msg.number = my_number;
        if (!conn->Write(&msg, sizeof(msg))) break;

        sem_post(&state->sem_made_move);

        if (!WaitWithTimeout(&state->sem_result_ready, "Kid")) break;

        MsgToClient response;
        if (!conn->Read(&response, sizeof(response))) break;

        is_alive = response.is_alive;
        sem_post(&state->sem_next_round);

        if (response.game_over) break;
    }
    conn->Close();
}

// логика волка (Host)
void HostLogic(Conn* conn, GameState* state) {
    std::srand(time(NULL));
    int dead_rounds_counter = 0;
    bool kid_is_alive = true;
    int round_num = 1;

    std::cout << "Game started! YOU are the Wolf." << std::endl;

    while (true) {
        std::cout << "\n--- Round " << round_num++ << " ---" << std::endl;

        // вводим число волка
        int wolf_number = GetWolfInputOrRandom();
        
        // даем сигнал Козленку
        sem_post(&state->sem_start_turn);

        // ждем ответ Козленка
        if (!WaitWithTimeout(&state->sem_made_move, "Wolf")) break;

        MsgFromClient msg;
        if (!conn->Read(&msg, sizeof(msg))) break;
        int kid_number = msg.number;
        
        // Логика игры
        int diff = std::abs(wolf_number - kid_number);
        if (kid_is_alive) {
            if (diff > 70) {
                kid_is_alive = false;
                std::cout << "--> Wolf ate the Kid!" << std::endl;
            } else {
                std::cout << "--> Kid hid successfully." << std::endl;
            }
        } else {
            if (diff <= 20) {
                kid_is_alive = true;
                std::cout << "--> Kid resurrected!" << std::endl;
            } else {
                std::cout << "--> Kid remains dead." << std::endl;
            }
        }

        std::cout << "Stats -> Wolf: " << wolf_number << " | Kid: " << kid_number << " (Diff: " << diff << ")" << std::endl;
        std::cout << "Kid Status: " << (kid_is_alive ? "ALIVE" : "DEAD") << std::endl;

        if (!kid_is_alive) dead_rounds_counter++;
        else dead_rounds_counter = 0;

        bool game_over = (dead_rounds_counter >= 2);

        MsgToClient response;
        response.is_alive = kid_is_alive;
        response.game_over = game_over;
        conn->Write(&response, sizeof(response));

        sem_post(&state->sem_result_ready);
        
        if (!WaitWithTimeout(&state->sem_next_round, "Wolf")) break;

        if (game_over) {
            std::cout << "Game Over! You won (Kid dead for 2 turns)." << std::endl;
            state->game_running = false;
            sem_post(&state->sem_start_turn);
            break;
        }
    }
    conn->Close();
}

int main() {
    Conn* conn = CreateConn();
    if (!conn) return 1;

    void* ptr = mmap(NULL, sizeof(GameState), PROT_READ | PROT_WRITE, 
                     MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (ptr == MAP_FAILED) return 1;
    GameState* state = (GameState*)ptr;

    sem_init(&state->sem_start_turn, 1, 0);
    sem_init(&state->sem_made_move, 1, 0);
    sem_init(&state->sem_result_ready, 1, 0);
    sem_init(&state->sem_next_round, 1, 0);
    state->game_running = true;

    pid_t pid = fork();
    if (pid < 0) return 1;

    conn->OnFork(pid > 0);

    if (pid == 0) {
        ClientLogic(conn, state);
        delete conn;
    } else {
        HostLogic(conn, state);
        wait(NULL);
        sem_destroy(&state->sem_start_turn);
        sem_destroy(&state->sem_made_move);
        sem_destroy(&state->sem_result_ready);
        sem_destroy(&state->sem_next_round);
        munmap(ptr, sizeof(GameState));
        delete conn;
    }
    return 0;
}