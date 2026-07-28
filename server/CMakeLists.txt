cmake_minimum_required(VERSION 3.16)
project(IMServer LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

# ==================== 依赖 ====================
find_package(Boost REQUIRED COMPONENTS system)
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

# Protobuf (直接用 Linux 系统路径，避免 CMake 找 /mnt 下的 Windows 版本)
set(PROTOBUF_LIB /usr/lib/x86_64-linux-gnu/libprotobuf.so)
set(PROTOBUF_INCLUDE_DIR /usr/include)
find_program(PROTOC_EXE protoc)

find_package(gRPC CONFIG REQUIRED)

# 所有 find_library/find_path 加 PATHS 限制，防止从 /mnt 找 Windows 版本
set(LINUX_LIB_PATHS /usr/lib/x86_64-linux-gnu /usr/lib)
set(LINUX_INC_PATHS /usr/include /usr/local/include)

# JsonCpp (apt 装的 libjsoncpp-dev，headers 在 /usr/include/jsoncpp)
find_library(JSONCPP_LIB jsoncpp PATHS ${LINUX_LIB_PATHS} NO_DEFAULT_PATH)
find_path(JSONCPP_INCLUDE_DIR json/json.h
    PATHS /usr/include/jsoncpp ${LINUX_INC_PATHS})
if(NOT JSONCPP_LIB OR NOT JSONCPP_INCLUDE_DIR)
    message(FATAL_ERROR "jsoncpp not found. apt install libjsoncpp-dev")
endif()

# MySQL Connector/C++
find_library(MYSQLCPP_LIB mysqlcppconn PATHS ${LINUX_LIB_PATHS} NO_DEFAULT_PATH)
find_path(MYSQL_INCLUDE_DIR cppconn/driver.h PATHS ${LINUX_INC_PATHS})
if(NOT MYSQLCPP_LIB)
    message(FATAL_ERROR "mysqlcppconn not found. apt install libmysqlcppconn-dev")
endif()

# hiredis
find_library(HIREDIS_LIB hiredis PATHS ${LINUX_LIB_PATHS} NO_DEFAULT_PATH)
find_path(HIREDIS_INCLUDE_DIR hiredis/hiredis.h PATHS ${LINUX_INC_PATHS})
if(NOT HIREDIS_LIB OR NOT HIREDIS_INCLUDE_DIR)
    message(FATAL_ERROR "hiredis not found. apt install libhiredis-dev")
endif()

# spdlog (header-only)
find_path(SPDLOG_INCLUDE_DIR spdlog/spdlog.h PATHS ${LINUX_INC_PATHS})
if(NOT SPDLOG_INCLUDE_DIR)
    message(FATAL_ERROR "spdlog not found. apt install libspdlog-dev")
endif()

# fmt
find_library(FMT_LIB fmt PATHS ${LINUX_LIB_PATHS} NO_DEFAULT_PATH)
if(NOT FMT_LIB)
    message(FATAL_ERROR "fmt not found. apt install libfmt-dev")
endif()

# ==================== common 静态库 ====================
file(GLOB COMMON_SRC
    common/*.cpp common/*.cc
    common/crypto/*.cpp common/crypto/*.c
)
list(FILTER COMMON_SRC EXCLUDE REGEX ".*_test\\.cpp$")

add_library(im_common STATIC ${COMMON_SRC})
target_include_directories(im_common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/common
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${MYSQL_INCLUDE_DIR}
    ${JSONCPP_INCLUDE_DIR}
    ${HIREDIS_INCLUDE_DIR}
    ${SPDLOG_INCLUDE_DIR}
    ${PROTOBUF_INCLUDE_DIR}
)
target_link_libraries(im_common PUBLIC
    Boost::system
    OpenSSL::Crypto
    Threads::Threads
    ${MYSQLCPP_LIB}
    ${JSONCPP_LIB}
    ${HIREDIS_LIB}
    ${FMT_LIB}
)

# ==================== 宏：构建服务 ====================
function(add_im_service NAME)
    file(GLOB_RECURSE SRC "${NAME}/*.cpp" "${NAME}/*.cc")
    list(FILTER SRC EXCLUDE REGEX ".*_test\\.cpp$")

    add_executable(${NAME} ${SRC})
    target_include_directories(${NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/${NAME}
        ${CMAKE_CURRENT_SOURCE_DIR}/common
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${MYSQL_INCLUDE_DIR}
        ${JSONCPP_INCLUDE_DIR}
    )
    target_link_libraries(${NAME} PRIVATE
        im_common
        gRPC::grpc++
        gRPC::grpc
        ${PROTOBUF_LIB}
        ${FMT_LIB}
    )
endfunction()

# ==================== 各服务 ====================
add_im_service(AuthServer)
add_im_service(StatusServer)
add_im_service(ChatServer1)
add_im_service(ChatServer2)
add_im_service(ResourceServer)
add_im_service(SeckillServer)
