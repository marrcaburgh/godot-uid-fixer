cmake_minimum_required(VERSION 3.10.0)
project(godot-uid-fixer VERSION 1.4 LANGUAGES CXX)
include_directories(includes)
add_executable(${PROJECT_NAME} source/main.cpp)
