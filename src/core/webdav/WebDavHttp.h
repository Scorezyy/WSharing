#pragma once
#include "../StringUtils.h"
#pragma comment(lib, "ws2_32.lib")
#include <winsock2.h>
#include <ws2tcpip.h>
#include <algorithm>
#include <sstream>
#include <string>
#include <vector>

inline std::string urlDecode(const std::string& s)
{
    std::string out;
    out.reserve(s.size());
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '%' && i + 2 < s.size()) {
            char hex[3]{ s[i+1], s[i+2], '\0' };
            out += static_cast<char>(std::strtol(hex, nullptr, 16));
            i += 2;
        } else if (s[i] == '+') {
            out += ' ';
        } else {
            out += s[i];
        }
    }
    return out;
}

inline std::string urlEncode(const std::string& s)
{
    std::string out;
    out.reserve(s.size() * 3);
    for (unsigned char c : s) {
        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~' || c == '/') {
            out += static_cast<char>(c);
        } else {
            char buf[4];
            snprintf(buf, sizeof(buf), "%%%02X", c);
            out += buf;
        }
    }
    return out;
}

struct HttpRequest
{
    std::string method;
    std::string path;
    std::string rawPath;
    int         depth      { 0 };
    size_t      contentLen { 0 };
    std::string destination;
    bool        keepAlive  { true };
};

inline bool recvLine(SOCKET s, std::string& line)
{
    line.clear();
    char c;
    while (true) {
        int r = recv(s, &c, 1, 0);
        if (r <= 0) return false;
        if (c == '\r') continue;
        if (c == '\n') return true;
        line += c;
    }
}

inline bool parseRequest(SOCKET s, HttpRequest& req)
{
    std::string line;
    if (!recvLine(s, line) || line.empty()) return false;

    std::istringstream first(line);
    std::string version;
    first >> req.method >> req.rawPath >> version;
    req.path = urlDecode(req.rawPath);

    auto q = req.path.find('?');
    if (q != std::string::npos) req.path = req.path.substr(0, q);

    while (recvLine(s, line) && !line.empty()) {
        auto col = line.find(':');
        if (col == std::string::npos) continue;
        std::string key   = line.substr(0, col);
        std::string value = line.substr(col + 2);
        std::transform(key.begin(), key.end(), key.begin(), ::tolower);

        if (key == "depth")          req.depth       = (value == "1") ? 1 : 0;
        if (key == "content-length") req.contentLen  = std::stoull(value);
        if (key == "destination")    req.destination = value;
        if (key == "connection") {
            std::string v = value;
            std::transform(v.begin(), v.end(), v.begin(), ::tolower);
            if (v.find("close") != std::string::npos) req.keepAlive = false;
        }
    }
    return true;
}

inline void sendRaw(SOCKET s, const std::string& data)
{
    size_t sent = 0;
    while (sent < data.size()) {
        int r = send(s, data.c_str() + sent, static_cast<int>(data.size() - sent), 0);
        if (r <= 0) break;
        sent += r;
    }
}

inline void sendResponse(SOCKET s, int code, const std::string& phrase,
                         const std::string& contentType, const std::string& body,
                         const std::string& extra = {}, bool keepAlive = true)
{
    std::ostringstream hdr;
    hdr << "HTTP/1.1 " << code << " " << phrase << "\r\n"
        << "Server: WSharing/1.0\r\n"
        << "DAV: 1,2\r\n"
        << "MS-Author-Via: DAV\r\n"
        << "Allow: OPTIONS,GET,PUT,DELETE,PROPFIND,MKCOL,MOVE\r\n";
    if (!contentType.empty())
        hdr << "Content-Type: " << contentType << "\r\n";
    hdr << "Content-Length: " << body.size() << "\r\n";
    if (!extra.empty()) hdr << extra;
    hdr << (keepAlive ? "Connection: keep-alive\r\n" : "Connection: close\r\n")
        << "\r\n";
    sendRaw(s, hdr.str());
    if (!body.empty()) sendRaw(s, body);
}
