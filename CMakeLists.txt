cmake_minimum_required(VERSION 3.20)
project(WSharing VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY         ${CMAKE_SOURCE_DIR}/output)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG   ${CMAKE_SOURCE_DIR}/output)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/output)

include(FetchContent)

#ImGui
FetchContent_Declare(imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.91.6
)
FetchContent_MakeAvailable(imgui)

set(IMGUI_SOURCES
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_dx11.cpp
)

#Application sources
set(APP_SOURCES
    src/main.cpp
    src/App.cpp
    src/window/DX11Window.cpp
    src/ui/Sidebar.cpp
    src/ui/HostPage.cpp
    src/ui/ClientPage.cpp
    src/ui/SettingsPage.cpp
    src/ui/LogPage.cpp
    src/ui/Toast.cpp
    src/core/Config.cpp
    src/core/Discovery.cpp
    src/network/DriveMounter.cpp
    src/core/webdav/WebDavServer.cpp src/core/webdav/WebDavHandlers.cpp
    src/core/TrayIcon.cpp
)

add_executable(WSharing WIN32 ${APP_SOURCES} ${IMGUI_SOURCES})

target_include_directories(WSharing PRIVATE
    src/
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)

target_link_libraries(WSharing PRIVATE
    ws2_32
    mpr
    dwmapi
    d3d11
    dxgi
    shell32
    ole32
    uuid
    shlwapi
    advapi32
    iphlpapi
)

target_compile_definitions(WSharing PRIVATE
    _WIN32_WINNT=0x0A00
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    UNICODE
    _UNICODE
    _CRT_SECURE_NO_WARNINGS
)

target_compile_options(WSharing PRIVATE /W3 /WX-)