#pragma once

#include <thread>
#include <random>
#include <atomic>
#include <sys/event.h>

#include <oneapi/tbb/concurrent_hash_map.h>

#include "ListenerSocket.hpp"
#include "ClientContext.hpp"
#include "Router.hpp"

class HTTPServer
{
private:
    static constexpr int kThreadPoolSize = 8;
    static constexpr int kMaxEvents = 10000;

    std::atomic<bool> m_active;
    ListenerSocket m_listenerSocket;
    std::thread m_listenerThread;

    std::mt19937 m_rng;
    std::uniform_int_distribution<int> m_sleepTimes;
    void sleepThread();

    tbb::concurrent_hash_map<int, ClientContext*> m_clientFds;
    size_t m_initializedThreads;
    std::mutex m_initMutex;
    std::condition_variable m_initCondVar;
    std::thread m_workerThreads[kThreadPoolSize];
    int m_workerKqFds[kThreadPoolSize];
    struct kevent m_workerEvents[kThreadPoolSize][kMaxEvents];

    Router m_router;

    // Unregister client from kqueue, delete ctx, close socket
    void killClient(int kqFd, int clientFd, ClientContext* ctx);

public:
    HTTPServer(const std::string& host, int port);
    ~HTTPServer();

    void start();
    void stop();
    void listen();
    void runEventLoop(int workerNum);
    void handleEvent(int kqfd, ClientContext* ctx, const struct kevent& event);
    bool isActive() const { return m_active; }
};