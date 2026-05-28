#pragma once
#include <string>
#include <vector>
#include <mutex>
#include <atomic>
#include <thread>
#include <cstdint>
#include <functional>

struct DiscoveredHost
{
    std::wstring name;
    std::string  ip;
    uint16_t     port;
    std::wstring shareName;
};

class Discovery
{
public:
    static constexpr uint16_t UDP_PORT = 45678;

    using HostCallback = std::function<void(const DiscoveredHost&)>;

    explicit Discovery(HostCallback cb = nullptr);
    ~Discovery();

    void startBroadcasting(const std::wstring& shareName, uint16_t tcpPort,
                            const std::string& bindIp = {});
    void stopBroadcasting();

    void startListening(const std::string& bindIp = {});
    void stopListening();

    std::vector<DiscoveredHost> getHosts() const;

private:
    void broadcastLoop(std::wstring shareName, uint16_t tcpPort, std::string bindIp);
    void listenLoop(std::string bindIp);

    mutable std::mutex          m_mutex;
    std::vector<DiscoveredHost> m_hosts;
    HostCallback                m_callback;

    std::atomic<bool>  m_broadcastRunning { false };
    std::atomic<bool>  m_listenRunning    { false };
    std::thread        m_broadcastThread;
    std::thread        m_listenThread;
};
