@echo off
chcp 65001 > nul
echo ================================================
echo   Protobuf/GRPC 代码生成脚本
echo ================================================

set PROTO_FILE=message.proto
set PROTOC=E:\ProgramFiles\grpc\grpc\visualpro\third_party\protobuf\Release\protoc.exe
set GRPC_PLUGIN=E:\ProgramFiles\grpc\grpc\visualpro\Debug\grpc_cpp_plugin.exe

echo.
echo 1. 生成GRPC代码...
"%PROTOC%" -I="." --grpc_out="." --plugin=protoc-gen-grpc="%GRPC_PLUGIN%" "%PROTO_FILE%"
if %errorlevel% neq 0 (
    echo 错误: GRPC代码生成失败！
    pause
    exit /b 1
)

echo.
echo 2. 生成Protobuf代码...
"%PROTOC%" --cpp_out=. "%PROTO_FILE%"
if %errorlevel% neq 0 (
    echo 错误: Protobuf代码生成失败！
    pause
    exit /b 1
)

echo.
echo ================================================
echo   代码生成成功！
echo   GRPC和Protobuf文件已生成在当前目录
echo ================================================
echo.
pause