cmake_minimum_required(VERSION 3.15)
project(PowerMinderHostTests CXX)
include(CTest)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(console_logic_tests
    console_logic_tests.cpp
    ../src/console_logic.cpp
)

target_include_directories(console_logic_tests PRIVATE
    ../src
)

target_compile_options(console_logic_tests PRIVATE
    -Wall
    -Werror
)

add_test(NAME console_logic_tests COMMAND console_logic_tests)
