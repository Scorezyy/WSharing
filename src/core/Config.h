#pragma once
#include <string>
#include <cstdint>

class Config
{
public:
    bool         hostEnabled   { false };
    std::wstring sharedFolder;
    std::wstring shareName     { L"WSharing" };

    bool         clientEnabled  { false };
    wchar_t      driveLetter    { L'Z' };
    std::wstring customDriveName { L"WSharing" };
    std::wstring driveIconPath;

    bool         autostart       { false };
    bool         startMinimized  { false };
    std::string  networkInterface;
    int          language        { 0 }; // 0=DE 1=EN

    void load();
    void save() const;

private:
    static std::wstring configFilePath();
};
