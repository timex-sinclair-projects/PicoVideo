@echo off
setlocal
if defined VSCMD_VER goto run
for /f "usebackq delims=" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "PV_VS=%%i"
if not defined PV_VS exit /b 2
call "%PV_VS%\Common7\Tools\VsDevCmd.bat" -arch=x64 >nul
if errorlevel 1 exit /b 1
:run
python "%~dp0run.py" --cc cl %*
exit /b %errorlevel%
