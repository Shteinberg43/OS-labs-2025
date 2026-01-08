#!/bin/bash
g++ -std=c++17 -Wall -Werror main.cpp Daemon.cpp -o lab1_daemon

if [ $? -eq 0 ]; then
    echo "Build successful! Run ./lab1_daemon config.txt"
else
    echo "Build failed."
fi