cmake_minimum_required(VERSION 4.2)
project(expression_evaluator)

set(CMAKE_CXX_STANDARD 20)

add_executable(expression_evaluator main.cpp)
target_include_directories(expression_evaluator PRIVATE ${CMAKE_SOURCE_DIR}/include)

add_executable(manual_expression_evaluator main_manual.cpp)
