cmake_minimum_required(VERSION 3.25)

project(Mercury VERSION 1.0.2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(MercuryDemo
    main.cpp
    lib/mercury.hpp
)

target_include_directories(MercuryDemo PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/lib
    ${CMAKE_CURRENT_SOURCE_DIR}/tests
)

option(MERCURY_BUILD_TESTS "Build tests for Mercury" ON)
if(MERCURY_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()