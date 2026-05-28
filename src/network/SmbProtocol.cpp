#include "SmbProtocol.h"
#pragma comment(lib, "netapi32.lib")
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <lm.h>
#include <string>

static constexpr wchar_t kSmbUser[] = L"WSharingUser";
static constexpr wchar_t kSmbPass[] = L"Ws!7kL2#mP9xQ@4z";

bool SmbProtocol::shareExists(const std::wstring& name)
{
    LPBYTE buf{};
    NET_API_STATUS st = NetShareGetInfo(nullptr,
        const_cast<LPWSTR>(name.c_str()), 0, &buf);
    if (buf) NetApiBufferFree(buf);
    return st == NERR_Success;
}

bool SmbProtocol::start(const std::wstring& folder, const std::wstring& shareName)
{
    stop();

    // Create local user WSharingUser
    USER_INFO_1 ui{};
    ui.usri1_name     = const_cast<LPWSTR>(kSmbUser);
    ui.usri1_password = const_cast<LPWSTR>(kSmbPass);
    ui.usri1_priv     = USER_PRIV_USER;
    ui.usri1_flags    = UF_SCRIPT | UF_PASSWD_CANT_CHANGE | UF_DONT_EXPIRE_PASSWD;
    DWORD parm = 0;
    NetUserAdd(nullptr, 1, reinterpret_cast<LPBYTE>(&ui), &parm);
    USER_INFO_1003 pw{};
    pw.usri1003_password = const_cast<LPWSTR>(kSmbPass);
    NetUserSetInfo(nullptr, kSmbUser, 1003, reinterpret_cast<LPBYTE>(&pw), &parm);

    // Grant NTFS access
    std::string folderA(folder.begin(), folder.end());
    std::string cmd = "icacls \"" + folderA + "\" /grant WSharingUser:(OI)(CI)RX /T /Q";
    WinExec(cmd.c_str(), SW_HIDE);

    // Remove stale share
    NetShareDel(nullptr, const_cast<LPWSTR>(shareName.c_str()), 0);

    // Create share with Everyone full control
    SECURITY_DESCRIPTOR sd{};
    InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
    SetSecurityDescriptorDacl(&sd, TRUE, nullptr, FALSE);
    SHARE_INFO_502 si{};
    si.shi502_netname             = const_cast<LPWSTR>(shareName.c_str());
    si.shi502_type                = STYPE_DISKTREE;
    si.shi502_remark              = const_cast<LPWSTR>(L"");
    si.shi502_max_uses            = static_cast<DWORD>(-1);
    si.shi502_path                = const_cast<LPWSTR>(folder.c_str());
    si.shi502_security_descriptor = &sd;
    NET_API_STATUS st = NetShareAdd(nullptr, 502, reinterpret_cast<LPBYTE>(&si), &parm);
    if (st != NERR_Success) return false;

    // Enable SMB firewall
    WinExec("netsh advfirewall firewall set rule group=\"File and Printer Sharing\" new enable=yes", SW_HIDE);

    // Server performance tuning
    {
        HKEY hk;
        const wchar_t* kSrv = L"SYSTEM\\CurrentControlSet\\Services\\LanmanServer\\Parameters";
        if (RegCreateKeyExW(HKEY_LOCAL_MACHINE, kSrv, 0, nullptr,
                            REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, nullptr, &hk, nullptr) == ERROR_SUCCESS) {
            auto setDW = [&](const wchar_t* n, DWORD v) {
                RegSetValueExW(hk, n, 0, REG_DWORD,
                               reinterpret_cast<const BYTE*>(&v), sizeof(v));
            };
            setDW(L"RequireSecuritySignature", 0); // disable signing overhead
            setDW(L"EnableSecuritySignature",  0);
            setDW(L"IRPStackSize",             32);  // default 15, max 50
            setDW(L"SizReqBuf",               65536); // larger request buffer
            setDW(L"MaxWorkItems",            8192);
            setDW(L"MaxFreeConnections",      100);
            setDW(L"MinFreeConnections",       32);
            setDW(L"EnableOplocks",            1);
            setDW(L"CachedOpenLimit",         10);
            RegCloseKey(hk);
        }
    }

    m_shareName = shareName;
    m_running   = true;
    return true;
}

void SmbProtocol::stop()
{
    if (!m_running && m_shareName.empty()) return;
    NetShareDel(nullptr, const_cast<LPWSTR>(m_shareName.c_str()), 0);
    m_running   = false;
    m_shareName.clear();
}
