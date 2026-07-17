@echo off
chcp 65001 >nul
cd /d "%~dp0"

echo ============================================
echo   Starting MySQL Master-Slave Containers...
echo ============================================
echo.

docker-compose up -d

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [ERROR] Failed to start MySQL containers.
    pause
    exit /b %ERRORLEVEL%
)

echo.
echo MySQL containers started successfully:
echo   mysql-master : 127.0.0.1:3307 (root / 123456 / JerryChat)
echo   mysql-slave  : 127.0.0.1:3308 (root / 123456)
echo.
pause
