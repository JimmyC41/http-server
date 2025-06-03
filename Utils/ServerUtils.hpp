#pragma once

#include <string>
#include <chrono>
#include <csignal>
#include <sys/types.h>
#include <sys/event.h> // kqueue
#include <sys/time.h> // struct timespec
#include <unistd.h> // close()
#include <stdexcept> // runtime_error
#include <cstring> // c_str
#include <fcntl.h> // fcntl
#include <arpa/inet.h> // htons(), inet_pton()
#include <netinet/in.h> // sockaddr_in
#include <spdlog/spdlog.h>

namespace server::utils
{

void setNonBlocking(int fd);
sockaddr_in createSockAddr(const std::string& host, int port);
void registerKqFd(int kqfd, int fd, bool read, bool write, void* udata);
void unregisterKqFd(int kqFd, int fd, bool read, bool write); 

}