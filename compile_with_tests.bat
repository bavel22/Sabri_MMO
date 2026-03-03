@echo off
echo Closing UE5 Editor...
taskkill /f /im "SabriMMO.exe" /t 2>nul
taskkill /f /im "UnrealEditor.exe" /t 2>nul
timeout /t 3 /nobreak >nul

echo Compiling SabriMMO with UITests...
"C:\UE_5.7\Engine\Build\BatchFiles\Build.bat" SabriMMOEditor Win64 Development -Project="C:\Sabri_MMO\client\SabriMMO\SabriMMO.uproject" -WaitMutex

echo.
echo Compilation complete! You can now open the UE5 editor.
echo ASabriMMOUITests should appear in Content Browser under C++ Classes.
pause
