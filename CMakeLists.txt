cmake_minimum_required(VERSION 3.16)
project(ggalloc CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

set(COMMON_FLAGS -Wall -Wextra -Werror)

include_directories(${CMAKE_SOURCE_DIR})

add_executable(ggalloc-demo allocator.cpp main_demo.cpp)
target_compile_options(ggalloc-demo PRIVATE ${COMMON_FLAGS})

enable_testing()
add_executable(ggalloc-tests allocator.cpp tests.cpp)
target_compile_options(ggalloc-tests PRIVATE ${COMMON_FLAGS})
add_test(NAME ggalloc-tests COMMAND ggalloc-tests)


add_executable(ggalloc-coaltest tmp/coalesce_test.cpp)
target_compile_options(ggalloc-coaltest PRIVATE ${COMMON_FLAGS})
add_test(NAME ggalloc-coaltest COMMAND ggalloc-coaltest)

add_executable(ggalloc-alias-test allocator.cpp tmp/alias_test.cpp)
target_compile_options(ggalloc-alias-test PRIVATE ${COMMON_FLAGS})
add_test(NAME ggalloc-alias-test COMMAND ggalloc-alias-test)

add_executable(ggalloc-gctest allocator.cpp tmp/gc_test.cpp)
target_compile_options(ggalloc-gctest PRIVATE ${COMMON_FLAGS})
add_test(NAME ggalloc-gctest COMMAND ggalloc-gctest)