#pragma once
#include <string>
#include <atomic>
#include <thread>
#include <cstdint>

class WebDavServer
{
public:
    WebDavServer() = default;
    ~WebDavServer();

    bool start(const std::wstring& rootFolder, uint16_t port);
    void stop();
    bool isRunning() const { return m_running.load(); }

private:
    void acceptLoop();
    void handleClient(uintptr_t sock);

    std::wstring       m_root;
    uint16_t           m_port          { 45679 };
    std::atomic<bool>  m_running       { false };
    std::atomic<int>   m_activeClients { 0 };
    uintptr_t          m_listenSock    { static_cast<uintptr_t>(~0) };
    std::thread        m_acceptThread;
};
