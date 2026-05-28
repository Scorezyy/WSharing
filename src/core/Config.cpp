#include "Config.h"
#include "StringUtils.h"
#include <shlobj.h>
#include <fstream>
#include <map>

using IniMap = std::map<std::string, std::string>;

static IniMap parseIni(const std::wstring& path)
{
    IniMap result;
    std::ifstream file(path);
    if (!file.is_open()) return result;

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == ';' || line[0] == '#' || line[0] == '[') continue;
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key   = line.substr(0, eq);
        std::string value = line.substr(eq + 1);
        result[key] = value;
    }
    return result;
}

static bool ini_bool(const IniMap& m, const std::string& k, bool def)
{
    auto it = m.find(k);
    return it == m.end() ? def : (it->second == "1");
}

static int ini_int(const IniMap& m, const std::string& k, int def)
{
    auto it = m.find(k);
    return it == m.end() ? def : std::stoi(it->second);
}

static std::string ini_str(const IniMap& m, const std::string& k, const std::string& def = {})
{
    auto it = m.find(k);
    return it == m.end() ? def : it->second;
}

std::wstring Config::configFilePath()
{
    wchar_t buf[MAX_PATH]{};
    SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, 0, buf);
    std::wstring dir = std::wstring(buf) + L"\\WSharing";
    CreateDirectoryW(dir.c_str(), nullptr);
    return dir + L"\\config.ini";
}

void Config::load()
{
    IniMap m = parseIni(configFilePath());

    hostEnabled    = ini_bool(m, "hostEnabled",    false);
    sharedFolder   = utf8_to_wide(ini_str(m, "sharedFolder"));
    shareName      = utf8_to_wide(ini_str(m, "shareName", "WSharing"));

    clientEnabled  = ini_bool(m, "clientEnabled",  false);
    std::string dl = ini_str(m, "driveLetter", "Z");
    driveLetter    = dl.empty() ? L'Z' : static_cast<wchar_t>(dl[0]);
    customDriveName = utf8_to_wide(ini_str(m, "customDriveName", "WSharing"));
    driveIconPath   = utf8_to_wide(ini_str(m, "driveIconPath"));

    autostart         = ini_bool(m, "autostart",         false);
    startMinimized    = ini_bool(m, "startMinimized",    false);
    networkInterface  = ini_str (m, "networkInterface");
}

void Config::save() const
{
    std::ofstream file(configFilePath());
    file << "[WSharing]\n"
         << "hostEnabled="    << (hostEnabled    ? 1 : 0) << "\n"
         << "sharedFolder="   << wide_to_utf8(sharedFolder) << "\n"
         << "shareName="      << wide_to_utf8(shareName) << "\n"
         << "clientEnabled="  << (clientEnabled  ? 1 : 0) << "\n"
         << "driveLetter="    << static_cast<char>(driveLetter) << "\n"
         << "customDriveName=" << wide_to_utf8(customDriveName) << "\n"
         << "driveIconPath="  << wide_to_utf8(driveIconPath) << "\n"
         << "autostart="         << (autostart      ? 1 : 0) << "\n"
         << "startMinimized="    << (startMinimized ? 1 : 0) << "\n"
         << "networkInterface="  << networkInterface          << "\n";
}
