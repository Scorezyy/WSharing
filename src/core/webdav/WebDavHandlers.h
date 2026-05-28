#pragma once
#include "WebDavHttp.h"
#include <filesystem>

namespace webdav {

std::filesystem::path urlToFs(const std::wstring& root, const std::string& urlPath);

void handleOptions (SOCKET s, bool keepAlive);
void handlePropfind(SOCKET s, const std::filesystem::path& fsPath,
                    const std::string& urlPath, int depth, bool keepAlive);
void handleGet     (SOCKET s, const std::filesystem::path& fsPath, bool keepAlive);
void handlePut     (SOCKET s, const std::filesystem::path& fsPath,
                    size_t contentLen, bool keepAlive);
void handleMkcol   (SOCKET s, const std::filesystem::path& fsPath, bool keepAlive);
void handleDelete  (SOCKET s, const std::filesystem::path& fsPath, bool keepAlive);
void handleMove    (SOCKET s, const std::filesystem::path& fsPath,
                    const std::string& destination, const std::wstring& root, bool keepAlive);

} // namespace webdav
