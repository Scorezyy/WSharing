#pragma once
#include <string>
#include <vector>
#include <mutex>
#include <chrono>
#include <ctime>

struct LogEntry
{
    enum class Kind { Info, Transfer, Connect, Disconnect, Error };
    Kind        kind;
    std::string timestamp;
    std::string text;
};

class AppLog
{
public:
    static AppLog& get()
    {
        static AppLog instance;
        return instance;
    }

    void add(LogEntry::Kind kind, std::string text)
    {
        LogEntry e;
        e.kind      = kind;
        e.text      = std::move(text);
        e.timestamp = currentTime();
        std::lock_guard<std::mutex> lk(m_mtx);
        if (m_entries.size() >= MAX_ENTRIES)
            m_entries.erase(m_entries.begin());
        m_entries.push_back(std::move(e));
        m_dirty = true;
    }

    std::vector<LogEntry> snapshot()
    {
        std::lock_guard<std::mutex> lk(m_mtx);
        m_dirty = false;
        return m_entries;
    }

    bool dirty() const { return m_dirty; }

    void clear()
    {
        std::lock_guard<std::mutex> lk(m_mtx);
        m_entries.clear();
        m_dirty = true;
    }

private:
    AppLog() = default;

    static std::string currentTime()
    {
        auto now = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(now);
        struct tm tm{};
        localtime_s(&tm, &t);
        char buf[16];
        snprintf(buf, sizeof(buf), "%02d:%02d:%02d", tm.tm_hour, tm.tm_min, tm.tm_sec);
        return buf;
    }

    static constexpr size_t MAX_ENTRIES = 500;
    std::mutex            m_mtx;
    std::vector<LogEntry> m_entries;
    std::atomic<bool>     m_dirty{ false };
};
