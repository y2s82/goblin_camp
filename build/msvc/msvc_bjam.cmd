@echo off
:: Called from MSVC project files to ensure we are in the correct directory.
setlocal
cd %~p0
cd ..\..
bjam %*
endlocal
