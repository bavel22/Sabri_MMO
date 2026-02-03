@echo off
taskkill /F /IM node.exe 2>nul
cd /d "%~dp0"
start "MMO Server Console" cmd /k "node src/index.js"
