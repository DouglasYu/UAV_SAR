cmake_minimum_required(VERSION 3.5)
project(uav_sar C)

set(CMAKE_C_FLAGS "-pthread -std=gnu99")
set(CMAKE_EXE_LINKER_FLAGS "-pthread")
set(CMAKE_C_COMPILER "arm-linux-gnueabihf-gcc")
set(CMAKE_CXX_COMPILER "arm-linux-gnueabihf-g++")

set(COMMON_CXX_FLAGS "-std=c++11 -pthread")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fprofile-arcs -ftest-coverage")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fprofile-arcs -ftest-coverage -lgcov")

## "uname -m" to auto distinguish Manifold2-G or Manifold2-C
# execute_process(COMMAND uname -m
#         OUTPUT_VARIABLE DEVICE_SYSTEM_ID)

# Set toolchain to be the one on raspberry-pi
set(TOOLCHAIN_NAME arm-linux-gnueabihf-gcc)

file(GLOB_RECURSE MODULE_COMMON_SRC ../common/*.c)
file(GLOB_RECURSE MODULE_HAL_SRC hal/*.c)
file(GLOB_RECURSE MODULE_APP_SRC application/*.c)
file(GLOB_RECURSE MODULE_SRC module/*.c)
file(GLOB_RECURSE UTILS ../module_sample/utils/*.c)
# file(GLOB_RECURSE MODULE_SAMPLE_SRC ../module_sample/*.c)

# include_directories(../module_sample)
include_directories(../common)
include_directories(module)
include_directories(../module_sample)

include_directories(../psdk_lib/include)
link_directories(../psdk_lib/lib/${TOOLCHAIN_NAME})
link_libraries(${CMAKE_CURRENT_LIST_DIR}/../psdk_lib/lib/${TOOLCHAIN_NAME}/libpayloadsdk.a)

add_executable(${PROJECT_NAME}
        ${UTILS}
        ${MODULE_APP_SRC}
        ${MODULE_HAL_SRC}
        ${MODULE_COMMON_SRC}
        ${MODULE_SRC})
target_link_libraries(${PROJECT_NAME} m)

add_custom_command(TARGET ${PROJECT_NAME}
        PRE_LINK COMMAND cmake ..
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
