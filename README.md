# Cardputer PC Monitor

Monitor your **Windows PC** from an **M5Stack Cardputer Adv** over Wi‑Fi — CPU, RAM, disk, network, optional GPU and CPU temperature.

<!-- Screenshots: add files to docs/images/ then uncomment:
<p align="center">
  <img src="docs/images/device.jpg" width="280" alt="Device"/>
  <img src="docs/images/cpu-page.jpg" width="280" alt="CPU page"/>
</p>
-->

## Features

- **Wi‑Fi monitoring** — no USB cable after setup
- **PC Agent** — lightweight Python service (`GET /stats` on port 8765)
- **Web setup portal** — connect Cardputer to your Wi‑Fi and set PC IP
- **Optional CPU temp** — via LibreHardwareMonitor (HTTP)
- **Optional GPU** — NVIDIA via `nvidia-smi`
- **One-click PC setup** — firewall, autostart, LHM watchdog (Windows)

## Requirements

| Component | Required |
|-----------|----------|
| M5Stack **Cardputer Adv** | Yes |
| **Windows 10/11** PC on same LAN | Yes |
| Python **3.10+** (Add to PATH) | Yes |
| PlatformIO (VS Code) for firmware upload | Yes |
| LibreHardwareMonitor | Optional (CPU temperature) |
| NVIDIA GPU + drivers | Optional (GPU page) |

## Quick start

### 1. PC — one-time setup

```bat
git clone https://github.com/YOUR_USERNAME/cardputer-pc-monitor.git
cd cardputer-pc-monitor
```

Right-click **`setup_pc_once.bat`** → **Run as administrator**.

It will:

1. Install Python dependencies (`psutil`)
2. Open firewall port **8765**
3. Start Agent in background + **login autostart**
4. Optionally install/configure **LibreHardwareMonitor** + watchdog (CPU temp)
5. Print your **LAN IP** — you need this for step 3

Verify on PC: `http://127.0.0.1:8765/stats`

### 2. Flash firmware

Open this folder in VS Code with PlatformIO → environment **`m5stack-cardputer`** → **Upload**.

### 3. Configure Cardputer

1. Connect to Wi‑Fi hotspot **`PCMonitor-Setup`**
2. Browser: **http://192.168.4.1**
3. Enter your Wi‑Fi SSID/password and PC **Agent IP** (from setup) + port **8765**

## Minimal setup (no autostart / no temperature)

```bat
pip install -r requirements.txt
run_agent.bat
```

Then flash firmware and configure via the portal. CPU temperature will show `--` without LHM.

## Feature matrix

| Data | Source | If missing |
|------|--------|------------|
| CPU / RAM / disk / network | psutil (Agent) | Fix Agent / firewall |
| CPU temperature | LibreHardwareMonitor `:8090` | Shows `--` |
| GPU | `nvidia-smi` | GPU page N/A |
| Autostart | `setup_pc_once.bat` | Run `run_agent.bat` manually |

## Keys (Cardputer)

| Key | Page |
|-----|------|
| **1** | CPU / RAM / temperature |
| **2** | Disk / network / disk I/O |
| **3** | GPU / system |
| **4** | Settings |
| **R** | Refresh (settings page) |
| **Del / Esc** | Home |

Status bar: date/time · **LINK** (connected) / **DOWN** (PC unreachable) · battery · Wi‑Fi

## PC scripts

| Script | Purpose |
|--------|---------|
| `setup_pc_once.bat` | **Recommended** — full one-time setup |
| `run_agent.bat` | Debug (console window) |
| `restart_agent.bat` | Restart Agent |
| `install_autostart.bat` | Agent autostart only |
| `install_lhm_autostart.bat` | LHM + watchdog only |
| `uninstall_all.bat` | Remove autostart tasks |
| `open_firewall.bat` | Firewall rule only |

## Project layout

```
cardputer-pc-monitor/
├── src/                 # Cardputer firmware (ESP32-S3)
├── platformio.ini
├── pc_monitor_agent.py  # Windows Agent
├── setup_pc_once.bat    # PC one-click setup
├── requirements.txt
└── docs/
    ├── TROUBLESHOOTING.md
    └── images/          # Screenshots for README
```

## Troubleshooting

See [docs/TROUBLESHOOTING.md](docs/TROUBLESHOOTING.md).

## License

MIT — see [LICENSE](LICENSE).
