#!/bin/bash

set -euo pipefail

absPidPath="/var/run/daemon.pid"

if [[ ! -f "$absPidPath" ]]; then
  sudo touch "$absPidPath"
fi

sudo chmod ugo+rw "$absPidPath"

CPPFLAGS="-std=c++17 -Wall -Wextra -Werror -O2"
SOURCES=(config.cpp daemon.cpp main.cpp)

if g++ $CPPFLAGS -o daemon "${SOURCES[@]}"; then
  exit 0
fi

g++ $CPPFLAGS -o daemon "${SOURCES[@]}" -lstdc++fs