@echo off
if /i not "%~1"=="_run" (
    cmd /k "%~f0" _run
    exit /b
)

setlocal EnableDelayedExpansion
cd /d "%~dp0"
chcp 65001 >nul

net session >nul 2>&1
if errorlevel 1 (
    echo.
    echo ========================================================
    echo   需要管理员权限
    echo ========================================================
    echo.
    echo 接下来会弹出 UAC 窗口，请点击「是」
    echo 配置会在【新的管理员窗口】中运行
    echo.
    pause
    powershell -NoProfile -Command "Start-Process -FilePath '%~f0' -ArgumentList '_run' -Verb RunAs"
    echo.
    echo 若已在管理员窗口完成配置，按任意键关闭此窗口。
    pause
    exit /b
)

echo.
echo ========================================================
echo   Cardputer PC Monitor - 一键配置（只需运行这一次）
echo ========================================================
echo.
echo 将自动完成：
echo   [1] 安装 Python 依赖
echo   [2] 防火墙放行端口 8765
echo   [3] Agent 后台运行 + 开机自启
echo   [4] LibreHardwareMonitor 自启 + 看门狗（CPU 温度）
echo   [5] 立即启动并验证
echo.
echo 按任意键开始...
pause >nul

echo.
echo [1/5] 安装 Python 依赖...
python -m pip install -r "%~dp0requirements.txt" -q
if errorlevel 1 (
    echo ERROR: pip install failed. Check Python installation.
    pause
    exit /b 1
)
echo OK

echo.
echo [2/5] 防火墙放行 8765...
netsh advfirewall firewall delete rule name="Cardputer Monitor Agent 8765" >nul 2>&1
netsh advfirewall firewall add rule name="Cardputer Monitor Agent 8765" dir=in action=allow protocol=TCP localport=8765 profile=any
if errorlevel 1 (
    echo WARN: Firewall rule may have failed.
) else (
    echo OK
)

echo.
echo [3/5] Agent 开机自启...
call "%~dp0install_autostart.bat" /silent
if errorlevel 1 (
    echo ERROR: Agent autostart failed.
    pause
    exit /b 1
)

echo.
echo [4/5] LHM 自启 + 看门狗（退出后自动拉起）...
call "%~dp0install_lhm_autostart.bat" /silent

echo.
echo [5/5] 重启 Agent 使修复生效...
call "%~dp0restart_agent.bat" /silent

echo.
echo 验证服务...
ping 127.0.0.1 -n 3 >nul
powershell -NoProfile -Command ^
  "$ip=(Get-NetIPAddress -AddressFamily IPv4 | Where-Object { $_.IPAddress -notlike '127.*' -and $_.PrefixOrigin -ne 'WellKnown' } | Select-Object -First 1 -ExpandProperty IPAddress); if(-not $ip){ $ip='YOUR_PC_IP' }; Write-Host ''; Write-Host '========== 配置完成 ==========' -ForegroundColor Green; Write-Host ('PC 局域网 IP: ' + $ip); Write-Host ('Cardputer Agent: http://' + $ip + ':8765/stats'); Write-Host ''; try { $r=Invoke-WebRequest -Uri 'http://127.0.0.1:8765/stats' -UseBasicParsing -TimeoutSec 8; $j=$r.Content | ConvertFrom-Json; Write-Host ('Agent: 运行中 (HTTP ' + $r.StatusCode + ')') -ForegroundColor Green; if($j.cpu.temp_c){ Write-Host ('CPU温度: ' + $j.cpu.temp_c + ' C') -ForegroundColor Green } else { Write-Host 'CPU温度: 等待 LHM 启动（约 10-30 秒）' -ForegroundColor Yellow } } catch { Write-Host 'Agent: 未响应，请再运行 restart_agent.bat' -ForegroundColor Yellow }; try { Invoke-WebRequest -Uri 'http://127.0.0.1:8090/data.json' -UseBasicParsing -TimeoutSec 3 | Out-Null; Write-Host 'LHM:   运行中' -ForegroundColor Green } catch { Write-Host 'LHM:   启动中... Agent 会自动拉起 LHM' -ForegroundColor Yellow }; Write-Host ''; Write-Host '以后无需再操作。LHM 被关闭后约 3 分钟内自动重启。'; Write-Host '================================'"

echo.
echo 配置完成。此窗口可关闭。
pause
