#pragma once

#include <string>
#include <unordered_map>

#include "Enum.hpp"

class MessageInterface
{
protected:
    Version m_version;
    std::unordered_map<std::string, std::string> m_headers;
    std::string m_content;
    int m_contentLength = 0;

public:
    MessageInterface() : m_version(Version::HTTP_1_1) {}
    virtual ~MessageInterface() = default;

    void setHeader(const std::string& key, const std::string& value) { m_headers[key] = value; }
    void removeHeader(const std::string& key) { m_headers.erase(key); }
    void clearHeader() { m_headers.clear(); }
    void setContent(const std::string& body) { m_content = body; m_contentLength = body.length(); }
    void clearContent() { m_content.clear(); }

    Version version() const { return m_version; }
    std::string header(const std::string& key);
    std::unordered_map<std::string, std::string> headers() const { return m_headers; }
    std::string content() const { return m_content; }
    int contentLength() const { return m_content.length(); }

};