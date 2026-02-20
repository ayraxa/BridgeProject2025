# BridgeProject2025

A compact full-stack bridge control prototype that connects a web dashboard, Python relay server, and ESP32 firmware to control bridge actions (open/close/boom/traffic lights) and stream logs back to the UI.

## Result video

[Watch the result here](https://drive.google.com/file/d/1wgJqTQo_XsuASP4O4tT1N7xiM9ameyk_/view)

---

## Highlights
- Embedded Web integration: ESP32 firmware + Python middleware + browser UI
- Networking: HTTP endpoints for control + TCP socket connection to the ESP32
- Feedback loop: commands out, logs/status back to the dashboard
- Clear separation of components: firmware / server / frontend split into folders

---

## Tech stack
- ESP32 / Arduino (C++)
- Python(relay server)
- HTML/CSS/JS (control dashboard)

---
## Project structure
```txt
BridgeProject2025/
├── .vscode/
│   └── settings.json            # VS Code project settings
├── esp/
│   └── main.ino                 # ESP32 firmware (bridge control logic)
├── main/
│   └── frontend/
│       ├── index.html           # Web dashboard (controls + log viewer)
│       └── styles.css           # Dashboard styling
├── src/
│   └── bridgebridge.py          # Python relay server (HTTP + TCP to ESP32 + logs)
├── .gitignore
└── README.md
```
## Features
- Dashboard commands:
  - Bridge: `open`, `close`, `shutdown`
  - Boom gate: `boom_open`, `boom_close`
  - Traffic lights: `traffic_green`, `traffic_yellow`, `traffic_red`, `traffic_off`
  - Modes: `mode_manual`, `mode_auto`
- Live log output in the UI (polls the server for updates)

---

## Quick start

### 1) Configure network (hotspot + IP)
- Put your laptop/PC on the same network the ESP32 will join.
- In `esp/main.ino`, set the Wi-Fi SSID + password to match your hotspot/router.
- Upload once, then check your hotspot/router device list to find the ESP32’s local IP address.
- In `src/bridgebridge.py`, set `ESP_IP = "<ESP32_IP_ADDRESS>"` to that address (so the server knows where to forward commands).

### 2) Flash the ESP32
Upload `esp/main.ino` using Arduino IDE / PlatformIO (after updating Wi-Fi settings above).

### 3) Run the Python relay server
From the repo root:
```bash
python src/bridgebridge.py
```

### 4) Open the dashboard
Serve main/frontend/index.html with a static server (recommended: VS Code Live Server) or a link to it also shows up in the terminal once it starts.

## How it works

The dashboard sends commands to the Python server at /command and fetches logs from /log.

The Python server forwards commands to the ESP32 over a TCP socket and relays ESP32 messages back into the log stream.