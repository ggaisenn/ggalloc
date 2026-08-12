cmake_minimum_required(VERSION 3.16)
project(ggalloc CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

set(COMMON_FLAGS -Wall -Wextra -Werror)

add_executable(ggalloc-demo allocator.cpp main_demo.cpp)
target_compile_options(ggalloc-demo PRIVATE ${COMMON_FLAGS})