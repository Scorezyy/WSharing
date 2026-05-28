# WSharing

A lightweight LAN file-sharing app for Windows. Share any folder over WebDAV and mount it as a drive letter on other machines — no cloud, no account, just your local network.

![Platform](https://img.shields.io/badge/platform-Windows-blue)
![C++](https://img.shields.io/badge/language-C%2B%2B20-informational)
![License](https://img.shields.io/badge/license-MIT-green)

---

## Features

- **Host** — share any local folder over HTTP/WebDAV with one click
- **Client** — mount a remote share as a Windows drive letter (e.g. `Z:`)
- **Auto-discovery** — hosts announce themselves over UDP; clients see them instantly
- **Fast transfers** — zero-copy file serving via `TransmitFile`, 4 MB socket buffers, HTTP keep-alive
- **Custom drive label & icon** — set a display name and `.ico` shown in Explorer
- **System tray** — minimizes to tray, optional autostart with Windows
- **Dark UI** — ImGui + DirectX 11, no external runtime required

---

## Requirements

| | |
|---|---|
| OS | Windows 10 or later (64-bit) |
| Service | Windows **WebClient** service (for drive mounting) |
| Build | Visual Studio 2022 Build Tools, CMake ≥ 3.20, Git |

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

1. Go to the **Host** tab
2. Select a folder and set a share name
3. Click **Start** — the share is now accessible on your LAN

### Connecting from another machine

1. Go to the **Client** tab
2. Discovered hosts appear automatically; select one
3. Choose a drive letter and click **Connect**

The share mounts as a standard Windows network drive and opens in Explorer.

---

## Project Structure

```
src/
├── core/
│   ├── webdav/
│   │   ├── WebDavHandlers.cpp/h  # Request handlers (GET, PUT, PROPFIND, ...)
│   │   ├── WebDavHttp.h          # HTTP parsing, URL codec, send helpers
│   │   ├── WebDavServer.cpp/h    # TCP accept loop, connection dispatch
│   ├── AppLog.h                  # Thread-safe in-app log
│   ├── Config.cpp/h              # Persistent settings
│   ├── Discovery.cpp/h           # UDP host broadcasting & discovery
│   ├── StringUtils.h             # UTF-8 / wide string helpers
│   └── TrayIcon.cpp/h            # System tray integration
├── design/
│   └── Colors.h                  # ImGui color palette
├── network/
│   └── DriveMounter.cpp/h        # Drive mount/unmount via WNetAddConnection2
├── ui/
│   ├── ClientPage.cpp            # Client tab UI
│   ├── HostPage.cpp              # Host tab UI
│   ├── LogPage.cpp               # Log tab UI
│   ├── SettingsPage.cpp          # Settings tab UI
│   ├── Sidebar.cpp               # Navigation sidebar
│   └── Toast.cpp                 # Status toast notifications
├── window/
│   └── DX11Window.cpp            # Win32 window + DirectX 11 swap chain
├── App.cpp/h                     # Application entry & orchestration
└── main.cpp
```

---

## License

MIT
