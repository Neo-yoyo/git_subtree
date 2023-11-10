cmake_minimum_required(VERSION 3.12)
project(vector)

#активируйте как минимум 17-й стандарт
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(vector
                main.cpp
                vector.h)

# подключите нужные библиотеки, поставив флаги -ltbb и -lpthread
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -ltbb -lpthread")
