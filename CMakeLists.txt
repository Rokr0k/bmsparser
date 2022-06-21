cmake_minimum_required(VERSION 3.10)

project(bmsparser)

add_library(bmsparser SHARED "src/bmsparser.cpp")

target_include_directories(bmsparser PUBLIC "include/")