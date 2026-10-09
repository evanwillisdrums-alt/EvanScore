@echo off
setlocal
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Set up Virtual Drumline.ps1"
if errorlevel 1 pause
