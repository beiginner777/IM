cmake_minimum_required(VERSION 3.16)
project(IMServer LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

# ==================== 依赖 ====================
find_package(Boost REQUIRED COMPONENTS system)
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

# Protobuf (手动找，apt 装的 libprotobuf-dev 无 cmake config)
find_library(PROTOBUF_LIB protobuf)
find_path(PROTOBUF_INCLUDE_DIR google/protobuf/message.h)
if(NOT PROTOBUF_LIB OR NOT PROTOBUF_INCLUDE_DIR)
    message(FATAL_ERROR "protobuf not found. apt install libprotobuf-dev")
endif()
find_program(PROTOC_EXE protoc)

find_package(gRPC CONFIG REQUIRED)

# JsonCpp (apt 装的 libjsoncpp-dev，headers 在 /usr/include/jsoncpp)
find_library(JSONCPP_LIB jsoncpp)
find_path(JSONCPP_INCLUDE_DIR json/json.h
    HINTS /usr/include/jsoncpp)
if(NOT JSONCPP_LIB OR NOT JSONCPP_INCLUDE_DIR)
    message(FATAL_ERROR "jsoncpp not found. apt install libjsoncpp-dev")
endif()

# MySQL Connector/C++
find_library(MYSQLCPP_LIB mysqlcppconn)
find_path(MYSQL_INCLUDE_DIR cppconn/driver.h)
if(NOT MYSQLCPP_LIB)
    message(FATAL_ERROR "mysqlcppconn not found. apt install libmysqlcppconn-dev")
endif()

# hiredis
find_library(HIREDIS_LIB hiredis)
find_path(HIREDIS_INCLUDE_DIR hiredis/hiredis.h)
if(NOT HIREDIS_LIB OR NOT HIREDIS_INCLUDE_DIR)
    message(FATAL_ERROR "hiredis not found. apt install libhiredis-dev")
endif()

# spdlog (header-only)
find_path(SPDLOG_INCLUDE_DIR spdlog/spdlog.h)
if(NOT SPDLOG_INCLUDE_DIR)
    message(FATAL_ERROR "spdlog not found. apt install libspdlog-dev")
endif()

# fmt
find_library(FMT_LIB fmt)
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
