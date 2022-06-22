cmake_minimum_required(VERSION 3.10)

project(bmsparser)

find_package(Iconv REQUIRED)

add_library(bmsparser SHARED "src/bmsparser.cpp" "src/iconv.cpp" "src/iconv.hpp")

target_include_directories(bmsparser PUBLIC "include/")

target_link_libraries(bmsparser Iconv::Iconv)