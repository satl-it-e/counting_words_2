cmake_minimum_required(VERSION 3.5)
project(counting_words_2)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS -pthread)

find_package(Threads REQUIRED)


add_executable(counting_words_2 test_main.cpp additional.cpp time_measure.h)

target_link_libraries(counting_words_2  Threads::Threads)
