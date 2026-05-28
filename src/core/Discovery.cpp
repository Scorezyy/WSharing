#include "Discovery.h"
#include "StringUtils.h"
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>

#include <algorithm>
#include <chrono>
#include <sstream>

static constexpr int BROADCAST_INTERVAL_MS = 2000;

// Returns the subnet broadcast address for the best (or specified) adapter.
// outIp is set to the adapter's own IP so we can bind the socket to it.
static bool getAdapterInfo(const std::string& preferIp, std::string& outIp, std::string& outBcast)
{
    ULONG sz = 0;
    GetAdaptersInfo(nullptr, &sz);
    if (sz == 0) return false;
    std::vector<BYTE> buf(sz);
    if (GetAdaptersInfo(reinterpret_cast<IP_ADAPTER_INFO*>(buf.data()), &sz) != NO_ERROR)
        return false;
    auto* p = reinterpret_cast<IP_ADAPTER_INFO*>(buf.data());
    for (; p; p = p->Next) {
        std::string ip = p->IpAddressList.IpAddress.String;
        std::string gw = p->GatewayList.IpAddress.String;
        if (ip == "0.0.0.0" || ip.empty() || gw == "0.0.0.0" || gw.empty()) continue;
        if (!preferIp.empty() && ip != preferIp) continue;
        outIp = ip;
        in_addr a{}, m{};
        inet_pton(AF_INET, ip.c_str(), &a);
        inet_pton(AF_INET, p->IpAddressList.IpMask.String, &m);
        a.S_un.S_addr = (a.S_un.S_addr & m.S_un.S_addr) | (~m.S_un.S_addr);
        char out[INET_ADDRSTRLEN]{};
        inet_ntop(AF_INET, &a, out, sizeof(out));
        outBcast = out;
        return true;
    }
    return false;
}

Discovery::Discovery(HostCallback cb) : m_callback(std::move(cb)) {}

Discovery::~Discovery()
{
    stopBroadcasting();
    stopListening();
}

void Discovery::startBroadcasting(const std::wstring& shareName, uint16_t tcpPort,
                                   const std::string& bindIp)
{
    if (m_broadcastRunning.exchange(true)) return;
    m_broadcastThread = std::thread(&Discovery::broadcastLoop, this, shareName, tcpPort, bindIp);
}

void Discovery::stopBroadcasting()
{
    m_broadcastRunning = false;
    if (m_broadcastThread.joinable()) m_broadcastThread.join();
}

void Discovery::broadcastLoop(std::wstring shareName, uint16_t tcpPort, std::string bindIp)
{
    WSADATA wsa{};
    WSAStartup(MAKEWORD(2, 2), &wsa);

    std::string localIp, bcastAddr;
    if (!getAdapterInfo(bindIp, localIp, bcastAddr))
        bcastAddr = "255.255.255.255";

    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET) { WSACleanup(); return; }

    BOOL bcast = TRUE;
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, reinterpret_cast<char*>(&bcast), sizeof(bcast));

    if (!localIp.empty()) {
        sockaddr_in local{};
        local.sin_family = AF_INET;
        inet_pton(AF_INET, localIp.c_str(), &local.sin_addr);
        bind(sock, reinterpret_cast<sockaddr*>(&local), sizeof(local));
    }

    wchar_t hostbuf[256]{};
    DWORD hostLen = 256;
    GetComputerNameW(hostbuf, &hostLen);
    std::string msg = "WSHARING|" + wide_to_utf8(hostbuf) + "|"
                    + std::to_string(tcpPort) + "|"
                    + wide_to_utf8(shareName);

    sockaddr_in dest{};
    dest.sin_family = AF_INET;
    dest.sin_port   = htons(UDP_PORT);
    inet_pton(AF_INET, bcastAddr.c_str(), &dest.sin_addr);

    while (m_broadcastRunning) {
        sendto(sock, msg.c_str(), static_cast<int>(msg.size()), 0,
               reinterpret_cast<sockaddr*>(&dest), sizeof(dest));
        for (int i = 0; i < BROADCAST_INTERVAL_MS / 100 && m_broadcastRunning; ++i)
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    closesocket(sock);
    WSACleanup();
}

void Discovery::startListening(const std::string& bindIp)
{
    if (m_listenRunning.exchange(true)) return;
    m_listenThread = std::thread(&Discovery::listenLoop, this, bindIp);
}

void Discovery::stopListening()
{
    m_listenRunning = false;
    if (m_listenThread.joinable()) m_listenThread.join();
}

void Discovery::listenLoop(std::string /*bindIp*/)
{
    WSADATA wsa{};
    WSAStartup(MAKEWORD(2, 2), &wsa);

    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET) { WSACleanup(); return; }

    BOOL reuse = TRUE;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<char*>(&reuse), sizeof(reuse));

    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(UDP_PORT);
    addr.sin_addr.s_addr = INADDR_ANY;
    bind(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));

    timeval tv{ 0, 200'000 };
    char buf[512]{};

    while (m_listenRunning) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(sock, &fds);
        if (select(0, &fds, nullptr, nullptr, &tv) <= 0) continue;

        sockaddr_in sender{};
        int slen = sizeof(sender);
        int n = recvfrom(sock, buf, sizeof(buf) - 1, 0,
                         reinterpret_cast<sockaddr*>(&sender), &slen);
        if (n <= 0) continue;
        buf[n] = '\0';

        std::string msg(buf);
        if (msg.rfind("WSHARING|", 0) != 0) continue;

        std::vector<std::string> parts;
        std::istringstream ss(msg);
        std::string tok;
        while (std::getline(ss, tok, '|')) parts.push_back(tok);
        if (parts.size() < 4) continue;

        DiscoveredHost host;
        host.name      = utf8_to_wide(parts[1]);
        host.port      = static_cast<uint16_t>(std::stoi(parts[2]));
        host.shareName = utf8_to_wide(parts[3]);
        char ipbuf[INET_ADDRSTRLEN]{};
        inet_ntop(AF_INET, &sender.sin_addr, ipbuf, sizeof(ipbuf));
        host.ip = ipbuf;

        std::lock_guard<std::mutex> lk(m_mutex);
        auto it = std::find_if(m_hosts.begin(), m_hosts.end(),
            [&](const DiscoveredHost& h) { return h.name == host.name; });
        if (it == m_hosts.end()) {
            m_hosts.push_back(host);
            if (m_callback) m_callback(host);
        } else {
            *it = host;
        }
    }

    closesocket(sock);
    WSACleanup();
}

std::vector<DiscoveredHost> Discovery::getHosts() const
{
    std::lock_guard<std::mutex> lk(m_mutex);
    return m_hosts;
}
