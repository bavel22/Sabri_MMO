@echo off
taskkill /F /IM node.exe 2>nul
cd /d "%~dp0"
cmd /k "node src/index.js"
