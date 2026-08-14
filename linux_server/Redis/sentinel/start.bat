@echo off
title Redis Sentinel Cluster (1 Master + 2 Slave + 3 Sentinel)

echo ==========================================
echo   Redis Sentinel Cluster Setup
echo   1 Master :6380 / 2 Slave :6381-6382
echo   3 Sentinel :26379-26381
echo ==========================================

:: 创建数据目录
if not exist "%~dp0data\master"   mkdir "%~dp0data\master"
if not exist "%~dp0data\slave-1"  mkdir "%~dp0data\slave-1"
if not exist "%~dp0data\slave-2"  mkdir "%~dp0data\slave-2"
if not exist "%~dp0data\sentinel-1" mkdir "%~dp0data\sentinel-1"
if not exist "%~dp0data\sentinel-2" mkdir "%~dp0data\sentinel-2"
if not exist "%~dp0data\sentinel-3" mkdir "%~dp0data\sentinel-3"

:: 启动集群
docker-compose -f "%~dp0docker-compose.yml" up -d

echo.
echo ==========================================
echo   Cluster started. Verify:
echo     docker-compose -f sentinel/docker-compose.yml ps
echo.
echo   Test Sentinel:
echo     redis-cli -p 26379 SENTINEL master mymaster
echo ==========================================
pause
