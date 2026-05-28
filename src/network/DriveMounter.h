#pragma once
#include <string>
#include <vector>
#include <cstdint>

class DriveMounter
{
public:
    // Prepares WebClient service for WebDAV mounts (sets registry values, restarts service).
    static bool prepareSystem();

    static std::vector<wchar_t> getAvailableLetters();

    // WebDAV: maps \\host@port\DavWWWRoot to driveLetter:
    static bool mount(const std::string& host, uint16_t port,
                      wchar_t driveLetter, const std::wstring& shareName,
                      const std::wstring& label    = {},
                      const std::wstring& iconPath = {});

    // SMB: maps \\host\shareName to driveLetter:
    static bool mountSmb(const std::string& host, const std::wstring& shareName,
                         wchar_t driveLetter,
                         const std::wstring& label    = {},
                         const std::wstring& iconPath = {});

    static bool unmount(wchar_t driveLetter);
    static bool isMounted(wchar_t driveLetter);

private:
    static void applyLabel (const std::string& host, uint16_t port, const std::wstring& label);
    static void applyLabelSmb(const std::string& host, const std::wstring& shareName, const std::wstring& label);
    static void applyIcon  (wchar_t letter, const std::wstring& iconPath);
    static void clearLabel (const std::string& host, uint16_t port);
    static void clearLabelSmb(const std::string& host, const std::wstring& shareName);
    static void clearIcon  (wchar_t letter);
};
