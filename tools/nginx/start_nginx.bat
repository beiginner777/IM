@echo off
chcp 65001 >nul
title Nginx 启动

set NGINX_DIR=%~dp0

echo ============================================
echo   Nginx 启动脚本
echo ============================================

:: 检查 Nginx 目录
if not exist "%NGINX_DIR%\nginx.exe" (
    echo [错误] 找不到 nginx.exe，请确认路径: %NGINX_DIR%
    pause
    exit /b 1
)

:: 检查是否已在运行
tasklist /fi "imagename eq nginx.exe" 2>nul | find /i "nginx.exe" >nul
if %errorlevel% equ 0 (
    echo [提示] Nginx 已在运行，执行 reload 重载配置...
    cd /d "%NGINX_DIR%"
    nginx -s reload
    if %errorlevel% equ 0 (
        echo [成功] 配置已重载
    ) else (
        echo [失败] 重载配置失败，请检查配置语法
    )
    pause
    exit /b 0
)

:: 检查配置语法
cd /d "%NGINX_DIR%"
nginx -t
if %errorlevel% neq 0 (
    echo [错误] Nginx 配置语法有误，请检查 conf\im.conf
    pause
    exit /b 1
)

:: 启动
echo.
echo 启动 Nginx...
start nginx
timeout /t 2 /nobreak >nul

:: 验证
tasklist /fi "imagename eq nginx.exe" 2>nul | find /i "nginx.exe" >nul
if %errorlevel% equ 0 (
    echo [成功] Nginx 已启动，监听 8100 端口
    echo.
    echo 路由规则:
    echo   /loginAddr  /fe_login  /getVerifyCode  /registerUserAddr → AuthServer :8080
    echo   /products  /rank  /profile  /balance  /orders  /order/*  → SeckillServer :8101
    echo   /recharge  /buy/*  （秒杀接口，10r/s 限流）          → SeckillServer :8101
    echo.
    echo 测试: http://127.0.0.1:8100/products
) else (
    echo [失败] Nginx 启动失败
)

pause
