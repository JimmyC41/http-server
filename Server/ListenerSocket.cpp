#include <sys/socket.h> // socket()
#include <arpa/inet.h> // htons(), inet_pton()
#include <unistd.h> // close()
#include <spdlog/spdlog.h>
#include "ListenerSocket.hpp"
#include "ServerUtils.hpp"

ListenerSocket::ListenerSocket(const std::string& host, int port)
{
    if ((m_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        throw std::runtime_error("Failed to create a TCP socket");

    server::utils::setNonBlocking(m_fd);
    auto addr = server::utils::createSockAddr(host, port);

    if (bind(m_fd, (sockaddr*)&addr, sizeof(addr)) < 0)
        throw std::runtime_error("Failed to bind socket to address");

    spdlog::info("[fd {}] Socket bound to {}:{}", m_fd, host, port);
}

ListenerSocket::~ListenerSocket()
{
    if (m_fd >= 0)
        close(m_fd);
}

void ListenerSocket::listen()
{
    if (::listen(m_fd, kBacklogSize) < 0)
        spdlog::error("Sever socket listen failed: {} ({})",
            strerror(errno), errno);
}