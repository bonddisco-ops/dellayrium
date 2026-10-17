@echo off
rem Dellayrium installer for Windows: copies the VST3 to the system VST3 folder (asks for admin rights)
net session >nul 2>&1
if %errorlevel% neq 0 (
  powershell -Command "Start-Process -FilePath \"%~f0\" -Verb RunAs"
  exit /b
)
cd /d "%~dp0"
set "DEST=%CommonProgramFiles%\VST3"
if not exist "%DEST%" mkdir "%DEST%"
if exist "%DEST%\Dellayrium.vst3" rmdir /s /q "%DEST%\Dellayrium.vst3"
if exist "%DEST%\Dellayrium.vst3" (
  echo ERROR: the old plugin is in use. Close your DAW completely and run this again.
  echo OSHIBKA: staryy plagin zanyat. Polnostyu zakroy DAW i zapusti snova.
  pause
  exit /b 1
)
xcopy "Dellayrium.vst3" "%DEST%\Dellayrium.vst3\" /e /i /y >nul
if errorlevel 1 (
  echo.
  echo ERROR: could not copy. Close your DAW completely and run this again.
  echo OSHIBKA: ne udalos skopirovat. Polnostyu zakroy DAW i zapusti snova.
  pause
  exit /b 1
)
echo Dellayrium 0.1 installed to %DEST%
echo Rescan plugins in your DAW. / Pereskaniruy plaginy v DAW.
pause
