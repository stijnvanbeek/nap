cmake_minimum_required(VERSION 3.18.4)

project(test)

file(GLOB_RECURSE SOURCES src/*.cpp)
file(GLOB_RECURSE HEADERS src/*.h src/*.hpp)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})
