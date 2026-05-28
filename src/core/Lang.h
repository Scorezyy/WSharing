#pragma once
#include "../design/Strings.h"
#include <vector>
#include <string>

class Lang
{
public:
    static Lang& instance();
    // Call once at startup: extracts built-ins to AppData, scans folder
    void init();
    // Load language by index into the scanned list
    void load(int langIdx);
    const Strings& current() const { return m_str; }
    int count() const { return (int)m_files.size(); }
    // Display name of language at index (from "langName" key or filename)
    const std::string& nameAt(int i) const { return m_names[i]; }

private:
    Strings m_str;
    std::vector<std::wstring> m_files; // full paths to .json files
    std::vector<std::string>  m_names; // display names

    void parseInto(const std::string& content, Strings& out);
    static std::wstring langDir();
    static void extractResource(const char* resName, const std::wstring& path);
};
