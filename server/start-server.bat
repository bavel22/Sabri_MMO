@echo off
for /f "tokens=5" %%a in ('netstat -ano ^| findstr ":3000" ^| findstr "LISTENING"') do taskkill /F /PID %%a 2>nul
cd /d "%~dp0"
cmd /k "node src/index.js"
