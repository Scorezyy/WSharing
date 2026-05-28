#pragma once
#include <string>
#include <vector>
#include <cstdint>

// Mounts and unmounts a remote WebDAV share as a Windows drive letter.
// Requires the Windows WebClient service to be running.
class DriveMounter
{
public:
    // Ensures the WebClient service is running and enables basic auth over HTTP.
    static bool prepareSystem();

    // Returns all drive letters (D-Z) not currently in use.
    static std::vector<wchar_t> getAvailableLetters();

    // Maps \\host@port\DavWWWRoot to driveLetter:
    // label    — custom display name shown in Explorer (empty = default)
    // iconPath — path to .ico or executable with embedded icon (empty = default)
    static bool mount(const std::string& host, uint16_t port,
                      wchar_t driveLetter, const std::wstring& shareName,
                      const std::wstring& label    = {},
                      const std::wstring& iconPath = {});

    // Removes the drive mapping and cleans up registry entries.
    static bool unmount(wchar_t driveLetter);

    // Returns true if driveLetter is currently mapped.
    static bool isMounted(wchar_t driveLetter);

private:
    static void applyLabel (const std::string& host, uint16_t port, const std::wstring& label);
    static void applyIcon  (wchar_t letter, const std::wstring& iconPath);
    static void clearLabel (const std::string& host, uint16_t port);
    static void clearIcon  (wchar_t letter);
};
