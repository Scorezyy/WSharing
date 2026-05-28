# WSharing

A fast, zero-config LAN file sharing app for Windows. Share any folder over SMB and mount it as a drive letter on other machines — no cloud, no account, no setup. Just plug in and play.

![Platform](https://img.shields.io/badge/platform-Windows-blue)
![C++](https://img.shields.io/badge/language-C%2B%2B20-informational)
![License](https://img.shields.io/badge/license-MIT-green)

---

## Features

- **SMB file sharing** — uses native Windows network shares, the same protocol Explorer and Steam use
- **Works with Steam** — mount the share as a drive letter and point Steam to it, games run like they're local
- **One-click hosting** — select a folder, click Start, done
- **Auto-discovery** — hosts broadcast over UDP, clients see them instantly, no IP typing needed
- **Clean UAC** — one admin prompt at startup, nothing after that. No CMD windows, no PowerShell
- **Performance tuned** — SMB signing disabled, large buffers, bandwidth throttling off out of the box
- **Custom drive label & icon** — set the display name and `.ico` shown in Explorer
- **System tray** — minimizes to tray, optional autostart with Windows
- **Dark UI** — ImGui + DirectX 11, no external runtime required

---

## Requirements

| | |
|---|---|
| OS | Windows 10 or later (64-bit) |
| Build | Visual Studio 2022 Build Tools, CMake ≥ 3.20, Git |

> WSharing runs with admin rights (one UAC prompt on launch). This is required to create Windows network shares via the Win32 API.

---

## Build

```bat
git clone https://github.com/Scorezyy/WSharing.git
cd WSharing
build.bat
```

Output: `output\WSharing.exe`

CMake fetches [Dear ImGui v1.91.6](https://github.com/ocornut/imgui) automatically.

---

## Usage

### Hosting a folder

1. Launch WSharing — accept the UAC prompt (one time)
2. Go to the **Host** tab
3. Select a folder, set a share name
4. Click **Freigabe starten**

WSharing creates the SMB share, sets up the local user account, grants NTFS permissions and enables the firewall rule automatically.

### Connecting from another machine

1. Launch WSharing on the client machine
2. Go to the **Client** tab — discovered hosts appear automatically
3. Select a host, choose a drive letter, click **Verbinden**

The folder mounts as a standard Windows network drive. Open it in Explorer, add it to Steam as a library folder, or use it however you want.

### Steam

1. Connect and mount the share (e.g. as `Z:`)
2. In Steam → Settings → Storage → Add Drive → select `Z:`
3. Done — install and run games directly from the network share

---

## Project Structure

```
src/
├── core/
│   ├── AppLog.h                  # Thread-safe in-app log
│   ├── Config.cpp/h              # Persistent settings (INI)
│   ├── Discovery.cpp/h           # UDP host broadcasting & discovery
│   ├── StringUtils.h             # UTF-8 / wide string helpers
│   └── TrayIcon.cpp/h            # System tray integration
├── design/
│   └── Colors.h                  # ImGui color palette
├── network/
│   ├── DriveMounter.cpp/h        # SMB drive mount/unmount via WNetAddConnection2
│   └── SmbProtocol.cpp/h         # NetShareAdd/Del, user setup, perf tweaks
├── ui/
│   ├── ClientPage.cpp
│   ├── HostPage.cpp
│   ├── LogPage.cpp
│   ├── SettingsPage.cpp
│   ├── Sidebar.cpp
│   └── Toast.cpp
├── window/
│   └── DX11Window.cpp            # Win32 window + DirectX 11 swap chain
├── App.cpp/h                     # Application entry & orchestration
└── main.cpp
```

---

## License

MIT
