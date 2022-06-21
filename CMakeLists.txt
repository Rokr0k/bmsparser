cmake_minimum_required(VERSION 3.10)

project("bmsparser")

add_library(${PROJECT_NAME} SHARED "src/bmsparser.cpp")

target_include_directories(${PROJECT_NAME} PUBLIC "include/")