#include "DriveMounter.h"
#pragma comment(lib, "mpr.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "shell32.lib")
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <winnetwk.h>
#include <shlobj.h>
#include <string>

static void setRegistryDword(HKEY hive, const wchar_t* subkey,
                              const wchar_t* name, DWORD value)
{
    HKEY key{};
    if (RegCreateKeyExW(hive, subkey, 0, nullptr, 0,
                        KEY_SET_VALUE, nullptr, &key, nullptr) == ERROR_SUCCESS) {
        RegSetValueExW(key, name, 0, REG_DWORD,
                       reinterpret_cast<const BYTE*>(&value), sizeof(value));
        RegCloseKey(key);
    }
}

static bool ensureServiceRunning(const wchar_t* serviceName)
{
    SC_HANDLE scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!scm) return false;

    SC_HANDLE svc = OpenServiceW(scm, serviceName,
                                  SERVICE_QUERY_STATUS | SERVICE_START);
    if (!svc) { CloseServiceHandle(scm); return false; }

    SERVICE_STATUS_PROCESS ssp{};
    DWORD needed{};
    QueryServiceStatusEx(svc, SC_STATUS_PROCESS_INFO,
                         reinterpret_cast<LPBYTE>(&ssp), sizeof(ssp), &needed);

    bool started = (ssp.dwCurrentState == SERVICE_RUNNING);
    if (!started) {
        StartServiceW(svc, 0, nullptr);
        for (int i = 0; i < 50 && !started; ++i) {
            Sleep(100);
            QueryServiceStatusEx(svc, SC_STATUS_PROCESS_INFO,
                                 reinterpret_cast<LPBYTE>(&ssp), sizeof(ssp), &needed);
            started = (ssp.dwCurrentState == SERVICE_RUNNING);
        }
    }

    CloseServiceHandle(svc);
    CloseServiceHandle(scm);
    return started;
}

bool DriveMounter::prepareSystem()
{
    setRegistryDword(
        HKEY_LOCAL_MACHINE,
        L"SYSTEM\\CurrentControlSet\\Services\\WebClient\\Parameters",
        L"BasicAuthLevel",
        2);

    setRegistryDword(
        HKEY_LOCAL_MACHINE,
        L"SYSTEM\\CurrentControlSet\\Services\\WebClient\\Parameters",
        L"FileSizeLimitInBytes",
        0xFFFFFFFF);

    return ensureServiceRunning(L"WebClient");
}

std::vector<wchar_t> DriveMounter::getAvailableLetters()
{
    DWORD used = GetLogicalDrives();
    std::vector<wchar_t> result;
    for (wchar_t c = L'D'; c <= L'Z'; ++c) {
        if (!(used & (1u << (c - L'A'))))
            result.push_back(c);
    }
    return result;
}

bool DriveMounter::mount(const std::string& host, uint16_t port,
                          wchar_t driveLetter, const std::wstring& /*shareName*/,
                          const std::wstring& label,
                          const std::wstring& iconPath)
{
    // Make sure WebClient is running before attempting to mount
    prepareSystem();

    // Clear any stale mapping on this letter first
    if (isMounted(driveLetter))
        unmount(driveLetter);

    std::wstring uncPath;
    uncPath.reserve(64);
    uncPath += L"\\\\";
    for (char c : host) uncPath += static_cast<wchar_t>(c);
    uncPath += L"@";
    uncPath += std::to_wstring(port);
    uncPath += L"\\DavWWWRoot";

    wchar_t localDrive[3]{driveLetter, L':', L'\0'};

    NETRESOURCEW nr{};
    nr.dwType       = RESOURCETYPE_DISK;
    nr.lpLocalName  = localDrive;
    nr.lpRemoteName = const_cast<LPWSTR>(uncPath.c_str());
    nr.lpProvider   = nullptr;

    DWORD result = WNetAddConnection2W(&nr, nullptr, nullptr, CONNECT_UPDATE_PROFILE);
    if (result != NO_ERROR && result != ERROR_ALREADY_ASSIGNED)
        return false;

    if (!label.empty())    applyLabel(host, port, label);
    if (!iconPath.empty()) applyIcon(driveLetter, iconPath);

    // Notify Explorer so changes appear immediately
    SHChangeNotify(SHCNE_DRIVEADD, SHCNF_PATH, localDrive, nullptr);
    return true;
}

bool DriveMounter::unmount(wchar_t driveLetter)
{
    wchar_t localDrive[3]{driveLetter, L':', L'\0'};

    // Retrieve the remote path so we can clean up the label registry key
    wchar_t remotePath[512]{};
    DWORD   remoteLen = 512;
    WNetGetConnectionW(localDrive, remotePath, &remoteLen);

    DWORD result = WNetCancelConnection2W(localDrive, CONNECT_UPDATE_PROFILE, TRUE);

    clearIcon(driveLetter);

    // Notify Explorer
    SHChangeNotify(SHCNE_DRIVEREMOVED, SHCNF_PATH, localDrive, nullptr);

    return result == NO_ERROR || result == ERROR_NOT_CONNECTED;
}

bool DriveMounter::isMounted(wchar_t driveLetter)
{
    DWORD drives = GetLogicalDrives();
    int   bit    = driveLetter - L'A';
    return (drives & (1u << bit)) != 0;
}

void DriveMounter::applyLabel(const std::string& host, uint16_t port,
                               const std::wstring& label)
{
    // HKCU\...\MountPoints2\##host@port#DavWWWRoot  ->  _LabelFromReg
    std::wstring keyPath = L"Software\\Microsoft\\Windows\\CurrentVersion"
                           L"\\Explorer\\MountPoints2\\##";
    for (char c : host) keyPath += static_cast<wchar_t>(c);
    keyPath += L"@" + std::to_wstring(port) + L"#DavWWWRoot";

    HKEY key{};
    if (RegCreateKeyExW(HKEY_CURRENT_USER, keyPath.c_str(), 0, nullptr, 0,
                        KEY_SET_VALUE, nullptr, &key, nullptr) == ERROR_SUCCESS) {
        RegSetValueExW(key, L"_LabelFromReg", 0, REG_SZ,
            reinterpret_cast<const BYTE*>(label.c_str()),
            static_cast<DWORD>((label.size() + 1) * sizeof(wchar_t)));
        RegCloseKey(key);
    }
}

void DriveMounter::applyIcon(wchar_t letter, const std::wstring& iconPath)
{
    // HKCU\...\Explorer\DriveIcons\<letter>\DefaultIcon  ->  <path>
    std::wstring keyPath = L"Software\\Microsoft\\Windows\\CurrentVersion"
                           L"\\Explorer\\DriveIcons\\";
    keyPath += letter;
    keyPath += L"\\DefaultIcon";

    HKEY key{};
    if (RegCreateKeyExW(HKEY_CURRENT_USER, keyPath.c_str(), 0, nullptr, 0,
                        KEY_SET_VALUE, nullptr, &key, nullptr) == ERROR_SUCCESS) {
        RegSetValueExW(key, nullptr, 0, REG_SZ,
            reinterpret_cast<const BYTE*>(iconPath.c_str()),
            static_cast<DWORD>((iconPath.size() + 1) * sizeof(wchar_t)));
        RegCloseKey(key);
    }
}

void DriveMounter::clearLabel(const std::string& host, uint16_t port)
{
    std::wstring keyPath = L"Software\\Microsoft\\Windows\\CurrentVersion"
                           L"\\Explorer\\MountPoints2\\##";
    for (char c : host) keyPath += static_cast<wchar_t>(c);
    keyPath += L"@" + std::to_wstring(port) + L"#DavWWWRoot";

    HKEY key{};
    if (RegOpenKeyExW(HKEY_CURRENT_USER, keyPath.c_str(), 0, KEY_SET_VALUE, &key) == ERROR_SUCCESS) {
        RegDeleteValueW(key, L"_LabelFromReg");
        RegCloseKey(key);
    }
}

void DriveMounter::clearIcon(wchar_t letter)
{
    std::wstring keyPath = L"Software\\Microsoft\\Windows\\CurrentVersion"
                           L"\\Explorer\\DriveIcons\\";
    keyPath += letter;

    RegDeleteTreeW(HKEY_CURRENT_USER, keyPath.c_str());
}