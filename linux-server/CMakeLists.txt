cmake_minimum_required(VERSION 3.16)
project(IMServer LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

# ==================== 依赖 ====================
find_package(Boost REQUIRED COMPONENTS system filesystem)
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

# Protobuf
find_package(Protobuf REQUIRED)

# gRPC
find_package(gRPC CONFIG REQUIRED)

# JsonCpp
find_library(JSONCPP_LIB jsoncpp PATHS /usr/lib/x86_64-linux-gnu)
find_path(JSONCPP_INCLUDE_DIR json/json.h PATHS /usr/include/jsoncpp)
if(NOT JSONCPP_LIB OR NOT JSONCPP_INCLUDE_DIR)
    message(FATAL_ERROR "jsoncpp not found. apt install libjsoncpp-dev")
endif()

# MySQL Connector/C++
find_library(MYSQLCPP_LIB mysqlcppconn PATHS /usr/lib/x86_64-linux-gnu)
find_path(MYSQL_INCLUDE_DIR cppconn/driver.h PATHS /usr/include)
if(NOT MYSQLCPP_LIB)
    message(FATAL_ERROR "mysqlcppconn not found. apt install libmysqlcppconn-dev")
endif()

# hiredis
find_library(HIREDIS_LIB hiredis PATHS /usr/lib/x86_64-linux-gnu)
find_path(HIREDIS_INCLUDE_DIR hiredis/hiredis.h PATHS /usr/include)
if(NOT HIREDIS_LIB OR NOT HIREDIS_INCLUDE_DIR)
    message(FATAL_ERROR "hiredis not found. apt install libhiredis-dev")
endif()

# spdlog (header-only)
find_path(SPDLOG_INCLUDE_DIR spdlog/spdlog.h PATHS /usr/include)
if(NOT SPDLOG_INCLUDE_DIR)
    message(FATAL_ERROR "spdlog not found. apt install libspdlog-dev")
endif()

# fmt
find_library(FMT_LIB fmt PATHS /usr/lib/x86_64-linux-gnu)
if(NOT FMT_LIB)
    message(FATAL_ERROR "fmt not found. apt install libfmt-dev")
endif()

# libcrypt (libxcrypt) — bcrypt 用 crypt_r
find_library(CRYPT_LIB crypt)
if(NOT CRYPT_LIB)
    message(FATAL_ERROR "libcrypt not found. apt install libcrypt-dev")
endif()

# 所有可执行文件输出到 server/bin/
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)

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
    ${Protobuf_INCLUDE_DIRS}
)
target_link_libraries(im_common PUBLIC
    Boost::system
    Boost::filesystem
    OpenSSL::Crypto
    OpenSSL::SSL
    Threads::Threads
    ${MYSQLCPP_LIB}
    ${JSONCPP_LIB}
    ${HIREDIS_LIB}
    ${FMT_LIB}
    ${CRYPT_LIB}
    ${Protobuf_LIBRARIES}
)

# Debug 构建启用 debug 日志；Release 构建 SPDLOG_DEBUG 编译期展开为空（零开销）
target_compile_definitions(im_common PUBLIC
    $<$<CONFIG:Debug>:SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG>
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
        ${Protobuf_INCLUDE_DIRS}
    )
    target_link_libraries(${NAME} PRIVATE
        im_common
        gRPC::grpc++
        gRPC::grpc
        ${Protobuf_LIBRARIES}
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
