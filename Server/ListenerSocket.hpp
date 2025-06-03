#pragma once

#include <string>

class ListenerSocket
{
private:
    static constexpr int kBacklogSize = 1000;
    int m_fd{0};

public:
    ListenerSocket(const std::string& host, int port = 8080);
    ~ListenerSocket();
    int fd() const { return m_fd; }
    void listen();
};