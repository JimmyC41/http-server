#pragma once

#include <string>
#include <algorithm>

class Uri
{
private:
    std::string m_path;
    std::string m_scheme;
    std::string m_host;
    std::uint16_t m_port;

public:
    Uri() = default;
    explicit Uri(const std::string& path) : m_path(path) { setPathToLowercase(); }
    ~Uri() = default;

    bool operator<(const Uri& other) const { return m_path < other.m_path; }
    bool operator==(const Uri& other) const { return m_path == other.m_path; }

    void setPath(const std::string& path)
    {
        m_path = std::move(path);
        setPathToLowercase();
    }

    std::string path() const { return m_path; }
    std::string scheme() const { return m_scheme; }
    std::string host() const { return m_host; }
    std::uint16_t port() const { return m_port; }

private:
    void setPathToLowercase()
    {
        std::transform(m_path.begin(), m_path.end(), m_path.begin(),
                            [](char c) { return tolower(c); });
    }
};