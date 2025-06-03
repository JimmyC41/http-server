#include "ServerUtils.hpp"

namespace server::utils
{

void setNonBlocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);

    if (flags == -1)
        throw std::runtime_error("Failed to get socket flag");
    
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
        throw std::runtime_error("Failed to set non-blocking TCP socket");
}


sockaddr_in createSockAddr(const std::string& host, int port)
{
    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, host.c_str(), &addr.sin_addr) <= 0)
        throw std::runtime_error("inet_pton failed to populate the sockaddr_in struct");

    return addr;
}

void registerKqFd(int kqfd, int fd, bool wantsRead, bool wantsWrite, void* udata)
{
    struct kevent changeList[2];
    int nChanges = 0;

    if (wantsRead)
        EV_SET(
            &changeList[nChanges++],    // ptr to new kevent slot
            fd,                         // fd to monitor
            EVFILT_READ,                // 'readable' filter
            EV_ADD | EV_ENABLE,         // add if missing
            0,                          // no filter-specific flags
            0,                          // no filter-specific data
            udata                       // udata
        );
    
    if (wantsWrite)
        EV_SET(
            &changeList[nChanges++],
            fd,
            EVFILT_WRITE,               // 'writable' filter
            EV_ADD | EV_ENABLE,
            0,
            0,
            udata
        );

    if (nChanges > 0)
        if (kevent(kqfd, changeList, nChanges, nullptr, 0, nullptr) < 0)
            throw std::runtime_error("kevent register failed");
        
    spdlog::info(
        "[fd {}] Client registered for {} with worker thread fd {}",
        fd, wantsRead ? "reads" : "writes", kqfd);
}

void unregisterKqFd(int kqFd, int fd, bool read, bool write)
{
    struct kevent change;

    // Ignore ENOENT, when we try to delete a filter that doesn't exist

    if (read)
    {
        EV_SET(&change,
                fd,
                EVFILT_READ,
                EV_DELETE,
                0,
                0,
                nullptr);
        
        if (kevent(kqFd, &change, 1, nullptr, 0, nullptr) < 0)
        {
            if (errno != ENOENT)
                throw std::runtime_error("kevent unregister read failed");
        }
        else
            spdlog::info("[fd {}] Unregistered read notifications for worker fd {}", fd, kqFd);
    }

    if (write)
    {
        EV_SET(&change,
                fd,
                EVFILT_WRITE,
                EV_DELETE,
                0,
                0,
                nullptr);
        
        if (kevent(kqFd, &change, 1, nullptr, 0, nullptr) < 0)
        {
            if (errno != ENOENT)
                throw std::runtime_error("kevent unregister write failed");
        }
        else
            spdlog::info("[fd {}] Unregistered write notifications for worker fd {}", fd, kqFd);
    }    
}

}