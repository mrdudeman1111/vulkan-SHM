CMAKE_MINIMUM_REQUIRED(VERSION 3.18.1)
project("External Image example")

file(GLOB SOURCES
    ${CMAKE_SOURCE_DIR}/*.cpp)

include_directories(${CMAKE_SOURCE_DIR})

