@echo off
setlocal EnableDelayedExpansion
cd /d "%~dp0"

set "SILENT="
if /i "%~1"=="/silent" set "SILENT=1"
if /i "%~1"=="/nopause" set "SILENT=1"

echo ========================================
echo  Cardputer PC Monitor - Install Autostart
echo ========================================
echo.

where pythonw >nul 2>&1
if errorlevel 1 (
    echo ERROR: pythonw not found. Install Python and add to PATH.
    if not defined SILENT pause
    exit /b 1
)

set "PROJECT_DIR=%~dp0"
if "%PROJECT_DIR:~-1%"=="\" set "PROJECT_DIR=%PROJECT_DIR:~0,-1%"

echo Installing scheduled task (logon + auto-restart on failure)...
powershell -NoProfile -ExecutionPolicy Bypass -Command ^
  "$ErrorActionPreference='Stop';" ^
  "$dir='%PROJECT_DIR%';" ^
  "$py=(Get-Command pythonw).Source;" ^
  "$script=Join-Path $dir 'pc_monitor_agent.py';" ^
  "$name='Cardputer PC Monitor Agent';" ^
  "Unregister-ScheduledTask -TaskName $name -Confirm:$false -ErrorAction SilentlyContinue;" ^
  "$action=New-ScheduledTaskAction -Execute $py -Argument ('\"{0}\"' -f $script) -WorkingDirectory $dir;" ^
  "$trigger=New-ScheduledTaskTrigger -AtLogOn;" ^
  "$trigger.Delay='PT30S';" ^
  "$settings=New-ScheduledTaskSettingsSet -AllowStartIfOnBatteries -DontStopIfGoingOnBatteries -StartWhenAvailable -ExecutionTimeLimit ([TimeSpan]::Zero) -RestartCount 999 -RestartInterval (New-TimeSpan -Minutes 1);" ^
  "Register-ScheduledTask -TaskName $name -Action $action -Trigger $trigger -Settings $settings -Description 'Cardputer Wi-Fi PC Monitor Agent (background)' -Force | Out-Null;" ^
  "Write-Host 'Task created:' $name"

if errorlevel 1 (
    echo ERROR: Failed to create scheduled task.
    if not defined SILENT pause
    exit /b 1
)

echo Starting agent now...
schtasks /Run /TN "Cardputer PC Monitor Agent" >nul 2>&1
ping 127.0.0.1 -n 4 >nul

powershell -NoProfile -Command ^
  "try { $r = Invoke-WebRequest -Uri 'http://127.0.0.1:8765/stats' -UseBasicParsing -TimeoutSec 5; Write-Host ('Agent OK - HTTP ' + $r.StatusCode) } catch { Write-Host ('Agent starting... if FAIL persists, run restart_agent.bat') }"

echo.
echo Done. Agent runs hidden at logon (~30s after login).
echo Memory ~30MB, idle CPU near 0%%.
echo Remove autostart: uninstall_autostart.bat
echo.
if not defined SILENT pause
