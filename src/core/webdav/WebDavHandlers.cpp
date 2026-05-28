#include "WebDavHandlers.h"
#include "../AppLog.h"
#pragma comment(lib, "mswsock.lib")
#include <mswsock.h>
#include <windows.h>
#include <algorithm>
#include <filesystem>
#include <sstream>

namespace fs = std::filesystem;

static std::string toRfc1123(const FILETIME& ft)
{
    SYSTEMTIME st{};
    FileTimeToSystemTime(&ft, &st);
    static const char* days[]   = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
    static const char* months[] = {"Jan","Feb","Mar","Apr","May","Jun",
                                   "Jul","Aug","Sep","Oct","Nov","Dec"};
    char buf[64];
    snprintf(buf, sizeof(buf), "%s, %02d %s %04d %02d:%02d:%02d GMT",
             days[st.wDayOfWeek], st.wDay, months[st.wMonth - 1], st.wYear,
             st.wHour, st.wMinute, st.wSecond);
    return buf;
}

static std::string toIso8601(const FILETIME& ft)
{
    SYSTEMTIME st{};
    FileTimeToSystemTime(&ft, &st);
    char buf[32];
    snprintf(buf, sizeof(buf), "%04d-%02d-%02dT%02d:%02d:%02dZ",
             st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
    return buf;
}

static std::string xmlEscape(const std::string& s)
{
    std::string out;
    for (char c : s) {
        switch (c) {
            case '&':  out += "&amp;";  break;
            case '<':  out += "&lt;";   break;
            case '>':  out += "&gt;";   break;
            case '"':  out += "&quot;"; break;
            default:   out += c;
        }
    }
    return out;
}

static std::string buildPropEntry(const std::string& href, bool isDir, uint64_t size,
                                   const FILETIME& created, const FILETIME& modified,
                                   const std::string& name)
{
    std::ostringstream xml;
    xml << "  <D:response>\r\n"
        << "    <D:href>" << xmlEscape(href) << "</D:href>\r\n"
        << "    <D:propstat>\r\n"
        << "      <D:prop>\r\n"
        << "        <D:displayname>" << xmlEscape(name) << "</D:displayname>\r\n"
        << "        <D:creationdate>" << toIso8601(created) << "</D:creationdate>\r\n"
        << "        <D:getlastmodified>" << toRfc1123(modified) << "</D:getlastmodified>\r\n";
    if (isDir) {
        xml << "        <D:resourcetype><D:collection/></D:resourcetype>\r\n";
    } else {
        xml << "        <D:resourcetype/>\r\n"
            << "        <D:getcontentlength>" << size << "</D:getcontentlength>\r\n"
            << "        <D:getcontenttype>application/octet-stream</D:getcontenttype>\r\n";
    }
    xml << "      </D:prop>\r\n"
        << "      <D:status>HTTP/1.1 200 OK</D:status>\r\n"
        << "    </D:propstat>\r\n"
        << "  </D:response>\r\n";
    return xml.str();
}

namespace webdav {

fs::path urlToFs(const std::wstring& root, const std::string& urlPath)
{
    std::string rel = urlPath;
    static const std::string prefix = "/DavWWWRoot";
    if (rel.rfind(prefix, 0) == 0) rel = rel.substr(prefix.size());
    if (rel.empty() || rel == "/") return fs::path(root);
    if (rel[0] == '/') rel = rel.substr(1);
    std::replace(rel.begin(), rel.end(), '/', '\\');
    return fs::path(root) / utf8_to_wide(rel);
}

void handleOptions(SOCKET s, bool keepAlive)
{
    sendResponse(s, 200, "OK", "text/plain", "", {}, keepAlive);
}

void handlePropfind(SOCKET s, const fs::path& fsPath,
                    const std::string& urlPath, int depth, bool keepAlive)
{
    std::error_code ec;
    if (!fs::exists(fsPath, ec)) {
        sendResponse(s, 404, "Not Found", "text/plain", "Not Found", {}, keepAlive);
        return;
    }

    std::ostringstream xml;
    xml << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\r\n"
        << "<D:multistatus xmlns:D=\"DAV:\">\r\n";

    auto addEntry = [&](const fs::path& p) {
        WIN32_FILE_ATTRIBUTE_DATA attr{};
        GetFileAttributesExW(p.c_str(), GetFileExInfoStandard, &attr);
        bool     isDir = (attr.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
        uint64_t size  = isDir ? 0 :
            (static_cast<uint64_t>(attr.nFileSizeHigh) << 32) | attr.nFileSizeLow;

        std::string href;
        if (p == fsPath) {
            href = urlPath;
            if (isDir && (href.empty() || href.back() != '/')) href += '/';
        } else {
            std::string base = urlPath;
            if (base.empty() || base.back() != '/') base += '/';
            href = base + urlEncode(wide_to_utf8(p.filename().wstring()));
            if (isDir) href += '/';
        }
        xml << buildPropEntry(href, isDir, size,
                              attr.ftCreationTime, attr.ftLastWriteTime,
                              wide_to_utf8(p.filename().wstring()));
    };

    addEntry(fsPath);
    if (depth >= 1 && fs::is_directory(fsPath, ec))
        for (auto& e : fs::directory_iterator(fsPath, ec))
            addEntry(e.path());

    xml << "</D:multistatus>\r\n";
    std::string body = xml.str();
    sendResponse(s, 207, "Multi-Status", "application/xml; charset=utf-8", body, {}, keepAlive);
}

void handleGet(SOCKET s, const fs::path& fsPath, bool keepAlive)
{
    std::error_code ec;
    if (!fs::exists(fsPath, ec) || fs::is_directory(fsPath, ec)) {
        sendResponse(s, 404, "Not Found", "text/plain", "Not Found", {}, keepAlive);
        return;
    }

    HANDLE hFile = CreateFileW(fsPath.c_str(), GENERIC_READ, FILE_SHARE_READ,
        nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) {
        sendResponse(s, 403, "Forbidden", "text/plain", "Forbidden", {}, keepAlive);
        return;
    }

    LARGE_INTEGER sz{};
    GetFileSizeEx(hFile, &sz);
    uint64_t size = static_cast<uint64_t>(sz.QuadPart);

    std::ostringstream hdr;
    hdr << "HTTP/1.1 200 OK\r\n"
        << "Server: WSharing/1.0\r\n"
        << "DAV: 1,2\r\n"
        << "Content-Type: application/octet-stream\r\n"
        << "Content-Length: " << size << "\r\n"
        << (keepAlive ? "Connection: keep-alive\r\n" : "Connection: close\r\n")
        << "\r\n";
    sendRaw(s, hdr.str());

    constexpr DWORD CHUNK = 256u * 1024u * 1024u;
    uint64_t remaining = size;
    while (remaining > 0) {
        DWORD toSend = static_cast<DWORD>(std::min<uint64_t>(remaining, CHUNK));
        if (!TransmitFile(s, hFile, toSend, 0, nullptr, nullptr, 0)) break;
        remaining -= toSend;
    }
    CloseHandle(hFile);
}

void handlePut(SOCKET s, const fs::path& fsPath, size_t contentLen, bool keepAlive)
{
    std::error_code ec;
    fs::create_directories(fsPath.parent_path(), ec);

    HANDLE hFile = CreateFileW(fsPath.c_str(), GENERIC_WRITE, 0,
        nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) {
        sendResponse(s, 403, "Forbidden", "text/plain", "Cannot write file", {}, keepAlive);
        constexpr size_t DRAIN = 256 * 1024;
        std::vector<char> tmp(DRAIN);
        size_t rem = contentLen;
        while (rem > 0) {
            int r = recv(s, tmp.data(), static_cast<int>(std::min(rem, DRAIN)), 0);
            if (r <= 0) break;
            rem -= r;
        }
        return;
    }

    constexpr size_t CHUNK = 256 * 1024;
    std::vector<char> buf(CHUNK);
    size_t remaining = contentLen;
    while (remaining > 0) {
        int r = recv(s, buf.data(), static_cast<int>(std::min(remaining, CHUNK)), 0);
        if (r <= 0) break;
        DWORD written{};
        WriteFile(hFile, buf.data(), static_cast<DWORD>(r), &written, nullptr);
        remaining -= r;
    }
    CloseHandle(hFile);
    sendResponse(s, 201, "Created", "text/plain", "", {}, keepAlive);
}

void handleMkcol(SOCKET s, const fs::path& fsPath, bool keepAlive)
{
    std::error_code ec;
    if (fs::exists(fsPath, ec)) {
        sendResponse(s, 405, "Method Not Allowed", "text/plain", "Already exists", {}, keepAlive);
        return;
    }
    fs::create_directories(fsPath, ec);
    sendResponse(s, 201, "Created", "text/plain", "", {}, keepAlive);
}

void handleDelete(SOCKET s, const fs::path& fsPath, bool keepAlive)
{
    std::error_code ec;
    if (!fs::exists(fsPath, ec)) {
        sendResponse(s, 404, "Not Found", "text/plain", "Not Found", {}, keepAlive);
        return;
    }
    fs::remove_all(fsPath, ec);
    sendResponse(s, 204, "No Content", "", "", {}, keepAlive);
}

void handleMove(SOCKET s, const fs::path& fsPath,
                const std::string& destination, const std::wstring& root, bool keepAlive)
{
    auto slash = destination.find("/DavWWWRoot");
    if (slash == std::string::npos) {
        sendResponse(s, 400, "Bad Request", "text/plain", "Bad destination", {}, keepAlive);
        return;
    }
    std::string destRel = destination.substr(slash);
    fs::path destFs = urlToFs(root, urlDecode(destRel));
    std::error_code ec;
    fs::create_directories(destFs.parent_path(), ec);
    fs::rename(fsPath, destFs, ec);

    auto strip = [](const std::string& p) {
        return p.rfind("/DavWWWRoot", 0) == 0 ? p.substr(11) : p;
    };
    AppLog::get().add(LogEntry::Kind::Info,
        "Umbenannt: " + strip(fsPath.generic_string()) + " -> " + strip(destRel));
    sendResponse(s, ec ? 409 : 201, ec ? "Conflict" : "Created", "text/plain", "", {}, keepAlive);
}

} // namespace webdav
