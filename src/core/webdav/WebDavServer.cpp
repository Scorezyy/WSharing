#include "WebDavServer.h"
#include "WebDavHandlers.h"
#include "../AppLog.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <thread>

namespace fs = std::filesystem;

WebDavServer::~WebDavServer() { stop(); }

bool WebDavServer::start(const std::wstring& rootFolder, uint16_t port)
{
    if (m_running) stop();
    m_root = rootFolder;
    m_port = port;

    WSADATA wsa{};
    WSAStartup(MAKEWORD(2, 2), &wsa);

    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) return false;

    BOOL reuse = TRUE;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<char*>(&reuse), sizeof(reuse));

    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0 ||
        listen(sock, SOMAXCONN) != 0) {
        closesocket(sock);
        return false;
    }

    m_listenSock   = static_cast<uintptr_t>(sock);
    m_running      = true;
    m_acceptThread = std::thread(&WebDavServer::acceptLoop, this);
    return true;
}

void WebDavServer::stop()
{
    m_running = false;
    if (m_listenSock != static_cast<uintptr_t>(~0)) {
        closesocket(static_cast<SOCKET>(m_listenSock));
        m_listenSock = static_cast<uintptr_t>(~0);
    }
    if (m_acceptThread.joinable()) m_acceptThread.join();
    for (int i = 0; i < 300 && m_activeClients.load() > 0; ++i)
        Sleep(10);
    WSACleanup();
}

void WebDavServer::acceptLoop()
{
    SOCKET ls = static_cast<SOCKET>(m_listenSock);
    timeval tv{ 0, 200'000 };

    while (m_running) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(ls, &fds);
        if (select(0, &fds, nullptr, nullptr, &tv) <= 0) continue;

        SOCKET client = accept(ls, nullptr, nullptr);
        if (client == INVALID_SOCKET) continue;

        sockaddr_in peer{};
        int peerLen = sizeof(peer);
        getpeername(client, reinterpret_cast<sockaddr*>(&peer), &peerLen);
        char ipBuf[INET_ADDRSTRLEN]{};
        inet_ntop(AF_INET, &peer.sin_addr, ipBuf, sizeof(ipBuf));
        AppLog::get().add(LogEntry::Kind::Connect, std::string("Verbindung von ") + ipBuf);

        ++m_activeClients;
        std::thread([this, client]() {
            handleClient(static_cast<uintptr_t>(client));
            --m_activeClients;
        }).detach();
    }
}

void WebDavServer::handleClient(uintptr_t rawSock)
{
    SOCKET s = static_cast<SOCKET>(rawSock);

    BOOL noDelay = TRUE;
    setsockopt(s, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<char*>(&noDelay), sizeof(noDelay));
    constexpr int BUF = 4 * 1024 * 1024;
    setsockopt(s, SOL_SOCKET, SO_SNDBUF, reinterpret_cast<const char*>(&BUF), sizeof(BUF));
    setsockopt(s, SOL_SOCKET, SO_RCVBUF, reinterpret_cast<const char*>(&BUF), sizeof(BUF));
    DWORD timeout = 30000;
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<char*>(&timeout), sizeof(timeout));
    setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<char*>(&timeout), sizeof(timeout));

    auto display = [](const std::string& p) {
        return p.rfind("/DavWWWRoot", 0) == 0 ? p.substr(11) : p;
    };

    try {
        while (m_running) {
            HttpRequest req;
            if (!parseRequest(s, req)) break;

            bool ka = req.keepAlive;
            fs::path fsPath = webdav::urlToFs(m_root, req.path);

            if      (req.method == "OPTIONS")  webdav::handleOptions(s, ka);
            else if (req.method == "PROPFIND") webdav::handlePropfind(s, fsPath, req.path, req.depth, ka);
            else if (req.method == "GET")      { AppLog::get().add(LogEntry::Kind::Transfer, "Download: " + display(req.path)); webdav::handleGet(s, fsPath, ka); }
            else if (req.method == "PUT")      { AppLog::get().add(LogEntry::Kind::Transfer, "Upload:   " + display(req.path)); webdav::handlePut(s, fsPath, req.contentLen, ka); }
            else if (req.method == "MKCOL")    { AppLog::get().add(LogEntry::Kind::Info, "Ordner erstellt: " + display(req.path)); webdav::handleMkcol(s, fsPath, ka); }
            else if (req.method == "DELETE")   { AppLog::get().add(LogEntry::Kind::Info, "Geloescht: " + display(req.path)); webdav::handleDelete(s, fsPath, ka); }
            else if (req.method == "MOVE")     webdav::handleMove(s, fsPath, req.destination, m_root, ka);
            else                               sendResponse(s, 501, "Not Implemented", "text/plain", "Method not supported", {}, ka);

            if (!ka) break;
        }
    } catch (...) {}

    closesocket(s);
}
