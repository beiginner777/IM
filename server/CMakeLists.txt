cmake_minimum_required(VERSION 3.16)
project(IMServer LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

# ==================== 依赖 ====================
find_package(Boost REQUIRED COMPONENTS system)
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)
find_package(Protobuf REQUIRED)
find_package(gRPC CONFIG REQUIRED)
find_package(JsonCpp REQUIRED)

# MySQL Connector/C++ (如果 apt 装的 libmysqlcppconn-dev)
find_library(MYSQLCPP_LIB mysqlcppconn)
find_path(MYSQL_INCLUDE_DIR cppconn/driver.h)
if(NOT MYSQLCPP_LIB)
    message(FATAL_ERROR "mysqlcppconn not found. apt install libmysqlcppconn-dev")
endif()

# ==================== Proto 搜索路径 ====================
# 每个服务的 message.proto/grpc 是用 protoc 预编译的 .pb.cc/.grpc.pb.cc
# Docker 构建时重新生成, 开发时直接用预编译的

# ==================== common 静态库 ====================
file(GLOB COMMON_SRC
    common/*.cpp
    common/crypto/*.cpp
    common/crypto/*.c
)
list(FILTER COMMON_SRC EXCLUDE REGEX ".*_test\\.cpp$") # 排除测试文件

add_library(im_common STATIC ${COMMON_SRC})
target_include_directories(im_common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/common
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${MYSQL_INCLUDE_DIR}
    ${JSONCPP_INCLUDE_DIRS}
)
target_link_libraries(im_common PUBLIC
    Boost::system
    OpenSSL::Crypto
    Threads::Threads
    ${MYSQLCPP_LIB}
    jsoncpp
)

# ==================== 宏：构建服务 ====================
function(add_im_service NAME)
    cmake_parse_arguments(ARG "" "PROTO_DIR" "" ${ARGN})

    file(GLOB_RECURSE SRC "${NAME}/*.cpp")
    # 排除 _test.cpp 和 .pb.cc/.grpc.pb.cc(单独添加)
    list(FILTER SRC EXCLUDE REGEX ".*_test\\.cpp$")

    add_executable(${NAME} ${SRC})
    target_include_directories(${NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/${NAME}
        ${CMAKE_CURRENT_SOURCE_DIR}/common
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${MYSQL_INCLUDE_DIR}
    )
    target_link_libraries(${NAME} PRIVATE
        im_common
        gRPC::grpc++
        gRPC::grpc
        protobuf::libprotobuf
    )
endfunction()

# ==================== 各服务 ====================
add_im_service(AuthServer)
add_im_service(StatusServer)
add_im_service(ChatServer1)
add_im_service(ChatServer2)
add_im_service(ResourceServer)
add_im_service(SeckillServer)
