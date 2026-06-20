@echo off
REM ============================================================
REM  build_signal.bat
REM  Auto kill -> build -> relaunch the TheSignal editor.
REM  Used by Claude in Nwiro (no direct shell) via
REM  Scripts/Python/trigger_build.py, which spawns this as a
REM  detached process so it survives the editor being killed.
REM ============================================================

setlocal EnableDelayedExpansion

set "PROJECT=C:\Users\Ommei\workspace\TheSignal\TheSignal.uproject"
set "ENGINE_BUILD=C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat"
set "ENGINE_EDITOR=C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe"
set "LOG=C:\Users\Ommei\workspace\TheSignal\Scripts\last_build.log"
set "FLAG=C:\Users\Ommei\workspace\TheSignal\Scripts\last_build.status"

REM Give the spawning process time to flush its MCP response and exit cleanly.
timeout /t 3 /nobreak >nul

REM Kill the editor + LiveCoding + any cmd headless instance so the DLL can be written.
taskkill /F /IM UnrealEditor.exe >nul 2>&1
taskkill /F /IM LiveCodingConsole.exe >nul 2>&1
taskkill /F /IM UnrealEditor-Cmd.exe >nul 2>&1

REM Wait for the OS to release DLL handles.
timeout /t 5 /nobreak >nul

echo === BUILD STARTED %DATE% %TIME% === > "%LOG%"
echo RUNNING > "%FLAG%"

call "%ENGINE_BUILD%" TheSignalEditor Win64 Development "%PROJECT%" -WaitMutex >> "%LOG%" 2>&1
set BUILD_RESULT=%ERRORLEVEL%

echo === BUILD EXITED %BUILD_RESULT% AT %DATE% %TIME% === >> "%LOG%"

if %BUILD_RESULT% EQU 0 (
    echo OK > "%FLAG%"
    echo BUILD OK >> "%LOG%"
) else (
    echo FAIL %BUILD_RESULT% > "%FLAG%"
    echo BUILD FAILED %BUILD_RESULT% >> "%LOG%"
)

REM Relaunch editor regardless — on fail, dev sees old DLL state instead of nothing.
start "" "%ENGINE_EDITOR%" "%PROJECT%"

endlocal
exit /b %BUILD_RESULT%
