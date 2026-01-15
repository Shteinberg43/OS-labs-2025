cmake_minimum_required(VERSION 3.10)
project(lab2_var1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -pthread")

# вариант 1 (Mmap Anonymous)

add_executable(host_mmap host.cpp conn_mmap.cpp)
target_link_libraries(host_mmap rt)

# вариант 3 (Shared Memory POSIX)
add_executable(host_shm host.cpp conn_shm.cpp)
target_link_libraries(host_shm rt)

# вариант 5 (Pipes)
add_executable(host_pipe host.cpp conn_pipe.cpp)
target_link_libraries(host_pipe rt)