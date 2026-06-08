cmake_minimum_required(VERSION 4.2)
project(expression_evaluator)

set(CMAKE_CXX_STANDARD 20)

add_executable(expression_evaluator main.cpp)
target_include_directories(expression_evaluator PRIVATE ${CMAKE_SOURCE_DIR}/include)

add_executable(expression_evaluator_is_this_bison main_manual.cpp)
target_include_directories(expression_evaluator_is_this_bison PRIVATE ${CMAKE_SOURCE_DIR}/include)
