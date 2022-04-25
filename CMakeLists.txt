cmake_minimum_required(VERSION 3.10)

project("bmsparser")

add_library(${PROJECT_NAME} SHARED "src/bmsparser.cpp")

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)

target_include_directories(${PROJECT_NAME} PUBLIC "include/")