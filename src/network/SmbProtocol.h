#pragma once
#include <string>
#include <cstdint>

// Host-side SMB sharing via Windows built-in File Sharing.
// Uses PowerShell New-SmbShare / Remove-SmbShare (elevated once on first use).
class SmbProtocol
{
public:
    bool start(const std::wstring& folder, const std::wstring& shareName);
    void stop();
    bool isRunning() const { return m_running; }

    const std::wstring& shareName() const { return m_shareName; }

private:
    bool       m_running  { false };
    std::wstring m_shareName;

    static bool shareExists(const std::wstring& name);
};
