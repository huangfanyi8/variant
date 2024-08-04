cmake_minimum_required(VERSION 3.20)

project(variant
        LANGUAGES CXX
        DESCRIPTION "cxx14 implementation std::variant")

set(CMAKE_CXX_STANDARD 14)

add_executable(main main.cpp)
target_include_directories(main PUBLIC variant.hpp)
