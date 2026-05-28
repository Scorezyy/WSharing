#include "Lang.h"
#include <windows.h>
#include <shlobj.h>
#include <string>
#include <fstream>
#include <sstream>

Lang& Lang::instance()
{
    static Lang inst;
    return inst;
}

static std::string nextString(const std::string& s, size_t& i)
{
    while (i < s.size() && s[i] != '"') ++i;
    if (i >= s.size()) return {};
    ++i;
    std::string r;
    r.reserve(64);
    while (i < s.size() && s[i] != '"') {
        if (s[i] == '\\' && i + 1 < s.size()) {
            ++i;
            switch (s[i]) {
                case '"':  r += '"';  break;
                case '\\': r += '\\'; break;
                case '/':  r += '/';  break;
                case 'n':  r += '\n'; break;
                case 'r':  r += '\r'; break;
                case 't':  r += '\t'; break;
                case 'u': {
                    if (i + 4 < s.size()) {
                        unsigned cp = std::stoul(s.substr(i + 1, 4), nullptr, 16);
                        i += 4;
                        if (cp < 0x80) {
                            r += (char)cp;
                        } else if (cp < 0x800) {
                            r += (char)(0xC0 | (cp >> 6));
                            r += (char)(0x80 | (cp & 0x3F));
                        } else {
                            r += (char)(0xE0 | (cp >> 12));
                            r += (char)(0x80 | ((cp >> 6) & 0x3F));
                            r += (char)(0x80 | (cp & 0x3F));
                        }
                    }
                    break;
                }
                default: r += s[i]; break;
            }
        } else {
            r += s[i];
        }
        ++i;
    }
    if (i < s.size()) ++i;
    return r;
}

#define FIELD(name) if (key == #name) out.name = val; else
void Lang::parseInto(const std::string& content, Strings& out)
{
    size_t i = 0;
    while (i < content.size() && content[i] != '{') ++i;
    if (i < content.size()) ++i;

    while (i < content.size()) {
        while (i < content.size() && content[i] != '"' && content[i] != '}') ++i;
        if (i >= content.size() || content[i] == '}') break;
        std::string key = nextString(content, i);
        while (i < content.size() && content[i] != ':') ++i;
        ++i;
        std::string val = nextString(content, i);

        FIELD(langName)
        FIELD(hostingActive)
        FIELD(notHosting)
        FIELD(navHost)
        FIELD(navClient)
        FIELD(navSettings)
        FIELD(navLog)
        FIELD(hostSubtitle)
        FIELD(folder)
        FIELD(browse)
        FIELD(shareName)
        FIELD(startSharing)
        FIELD(stopSharing)
        FIELD(sharingActive)
        FIELD(clientSubtitle)
        FIELD(foundHosts)
        FIELD(searchingHosts)
        FIELD(driveLetter)
        FIELD(displayName)
        FIELD(connect)
        FIELD(disconnect)
        FIELD(connectedTo)
        FIELD(settingsTitle)
        FIELD(settingsSubtitle)
        FIELD(startWithWindows)
        FIELD(startWithWindowsHint)
        FIELD(startMinimized)
        FIELD(startMinimizedHint)
        FIELD(networkAdapter)
        FIELD(adapterAuto)
        FIELD(adapterHint)
        FIELD(driveIcon)
        FIELD(driveIconHint)
        FIELD(language)
        FIELD(settingsSaved)
        FIELD(logSubtitle)
        FIELD(autoScroll)
        FIELD(clearLog)
        FIELD(noEvents)
        FIELD(selectFolderFirst)
        FIELD(settingUpShare)
        FIELD(hostStarted)
        FIELD(hostStartFailed)
        FIELD(shareStartFailed)
        FIELD(hostStopped)
        FIELD(shareStopped)
        FIELD(connectedToPrefix)
        FIELD(connectionFailed)
        FIELD(smbConnectionFailed)
        FIELD(disconnectedFrom)
        FIELD(driveDisconnected)
        (void)0;
    }
}
#undef FIELD

std::wstring Lang::langDir()
{
    wchar_t buf[MAX_PATH] = {};
    SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, SHGFP_TYPE_CURRENT, buf);
    return std::wstring(buf) + L"\\WSharing\\languages";
}

void Lang::extractResource(const char* resName, const std::wstring& path)
{
    if (GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES) return; // already exists
    HRSRC hRes = FindResourceA(nullptr, resName, MAKEINTRESOURCEA(10)); // RT_RCDATA=10
    if (!hRes) return;
    HGLOBAL hGlob = LoadResource(nullptr, hRes);
    if (!hGlob) return;
    DWORD size = SizeofResource(nullptr, hRes);
    const char* data = static_cast<const char*>(LockResource(hGlob));
    if (!data || size == 0) return;
    HANDLE hFile = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) return;
    DWORD written;
    WriteFile(hFile, data, size, &written, nullptr);
    CloseHandle(hFile);
}

void Lang::init()
{
    std::wstring dir = langDir();
    // Create directory structure
    std::wstring appdata = dir.substr(0, dir.rfind(L'\\'));
    CreateDirectoryW(appdata.c_str(), nullptr);
    CreateDirectoryW(dir.c_str(), nullptr);

    // Extract built-ins if missing
    extractResource("GERMAN_JSON",  dir + L"\\german.json");
    extractResource("ENGLISH_JSON", dir + L"\\english.json");

    // Scan all .json files
    m_files.clear();
    m_names.clear();

    WIN32_FIND_DATAW fd;
    HANDLE hFind = FindFirstFileW((dir + L"\\*.json").c_str(), &fd);
    if (hFind == INVALID_HANDLE_VALUE) return;
    do {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
        std::wstring fullPath = dir + L"\\" + fd.cFileName;
        // Read file to get langName
        HANDLE hFile = CreateFileW(fullPath.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
        std::string content;
        if (hFile != INVALID_HANDLE_VALUE) {
            DWORD size = GetFileSize(hFile, nullptr);
            if (size && size != INVALID_FILE_SIZE) {
                content.resize(size);
                DWORD read;
                ReadFile(hFile, &content[0], size, &read, nullptr);
                content.resize(read);
            }
            CloseHandle(hFile);
        }
        // Extract langName
        std::string displayName;
        size_t pos = content.find("\"langName\"");
        if (pos != std::string::npos) {
            pos += 10;
            Strings tmp;
            parseInto("{\"langName\":" + content.substr(pos, content.find('\n', pos) - pos) + "}", tmp);
            displayName = tmp.langName;
        }
        if (displayName.empty()) {
            // Fallback: use filename stem
            std::wstring stem = fd.cFileName;
            auto dot = stem.rfind(L'.');
            if (dot != std::wstring::npos) stem.resize(dot);
            displayName.assign(stem.begin(), stem.end());
        }
        m_files.push_back(fullPath);
        m_names.push_back(displayName);
    } while (FindNextFileW(hFind, &fd));
    FindClose(hFind);
}

void Lang::load(int langIdx)
{
    if (langIdx < 0 || langIdx >= (int)m_files.size()) return;
    HANDLE hFile = CreateFileW(m_files[langIdx].c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) return;
    DWORD size = GetFileSize(hFile, nullptr);
    std::string content;
    if (size && size != INVALID_FILE_SIZE) {
        content.resize(size);
        DWORD read;
        ReadFile(hFile, &content[0], size, &read, nullptr);
        content.resize(read);
    }
    CloseHandle(hFile);
    m_str = Strings{};
    parseInto(content, m_str);
}
