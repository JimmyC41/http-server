#include "Server.hpp"
#include "ServerUtils.hpp"
#include "Logger.hpp"
#include "ClientContext.hpp"
#include "Router.hpp"

HTTPServer::HTTPServer(const std::string& host, int port)
try
    : m_active(false)
    , m_listenerSocket(host, port)
    , m_initializedThreads(0)
    , m_rng(std::chrono::steady_clock::now().time_since_epoch().count())
    , m_sleepTimes(10, 100)
{
    spdlog::info("HTTPServer construction successful");
}
catch (const std::exception& ex)
{
    spdlog::error("Failed to construct HTTPServer: {}", ex.what());
    throw;
}

HTTPServer::~HTTPServer()
{
    if (m_active)
        stop();
    spdlog::info("Destructor for HTTPServer called.");
}

void HTTPServer::stop()
{
    spdlog::info("Stopping server: cleaning up threads and closing server socket.");

    if (!m_active)
    {
        spdlog::info("stop() called, but m_active = false");
        return;
    }
    
    m_active.store(false);

    spdlog::info("Active clients on shutdown: {}", m_clientFds.size());
    for (auto& entry : m_clientFds)
    {
        close(entry.first);
        spdlog::info("Closing active fd {}", entry.first);
    }

    // spdlog::info("Closing listener socket fd {}", m_listenerSocket.fd());
    close(m_listenerSocket.fd());

    // spdlog::info("Joining listener thread");
    m_listenerThread.join();

    // spdlog::info("Joining worker threads");
    for (int i = 0; i < kThreadPoolSize; i++)
        m_workerThreads[i].join();
    
    // spdlog::info("Closing worker kqueue fds");
    for (int i = 0; i < kThreadPoolSize; i++)
        close(m_workerKqFds[i]);
}

void HTTPServer::start()
{
    try
    {
        m_listenerSocket.listen();
    }
    catch(const std::exception& ex)
    {
        spdlog::error("Failed to construct HTTPServer: {}", ex.what());
        throw;
    }

    // Set up KQueue
    for (int i = 0; i < kThreadPoolSize; i++)
    {
        if ((m_workerKqFds[i] = kqueue()) < 0)
            throw std::runtime_error("Failed to create kqueue fd for worker");
        
        // spdlog::info("[fd {}] Created a new kq instance", m_workerKqFds[i]);
    }

    // Setup callbacks for HTTP handling
    m_router.registerHandler("/hello", Method::GET, [](const Request&)
    {
        Response res(StatusCode::Ok);
        res.setContent("Hello, Optiver!");
        return res;
    });
    
    // Setup threads
    m_active.store(true);

    m_listenerThread = std::thread(&HTTPServer::listen, this);

    for (int i = 0; i < kThreadPoolSize; i++)
    {
        m_workerThreads[i] = std::thread(&HTTPServer::runEventLoop, this, i);
    }

    {
        std::unique_lock<std::mutex> lock(m_initMutex);
        m_initCondVar.wait(lock, [this]
        {
            return m_initializedThreads == kThreadPoolSize + 1;
        });
    }
    // spdlog::info("All threads initialized, HTTPServer::start completed");
}

void HTTPServer::listen()
{
    spdlog::info("[fd {}] Listener socket thread started", m_listenerSocket.fd());

    {
        std::lock_guard<std::mutex> lock(m_initMutex);
        m_initializedThreads++;
        m_initCondVar.notify_one();
    }

    ClientContext* clientData;
    sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);
    int clientFd = 0;
    int workerNum = 0;  // the worker thread to register the client fd with
    bool accepting = true;

    while (m_active.load())
    {
        if (accepting)
        {
            // spdlog::info("Accept failed. Sleeping for random time.");
            std::this_thread::sleep_for(std::chrono::microseconds(m_sleepTimes(m_rng)));
        }

        clientFd = accept(m_listenerSocket.fd(), (sockaddr*)&clientAddr, &clientLen);
        if (clientFd < 0)
        {
            accepting = false;
            continue;
        }

        accepting = true;
        server::utils::setNonBlocking(clientFd);
        clientData = new ClientContext();
        clientData->fd = clientFd;

        m_clientFds.insert({clientFd, clientData});
        spdlog::info("m_clientFd size: {}", m_clientFds.size());

        spdlog::info("[fd {}] New client connection accepted", clientFd);

        server::utils::registerKqFd(m_workerKqFds[workerNum],
            clientFd, true, false, clientData);

        workerNum++;
        if (workerNum == HTTPServer::kThreadPoolSize) workerNum = 0;
    }
}

void HTTPServer::runEventLoop(int workerNum)
{
    // spdlog::info("[fd {}] Worker thread started", m_workerKqFds[workerNum]);
    {
        std::lock_guard<std::mutex> lock(m_initMutex);
        m_initializedThreads++;
        m_initCondVar.notify_one();
    }

    ClientContext* data;
    int kqFd = m_workerKqFds[workerNum];
    struct timespec timeout{0, 0};
    bool looping = true;

    while (m_active.load())
    {
        if (!looping)
        {
            // spdlog::info("[fd {}] No kevents to process. Sleeping for random time.", kqFd);
            std::this_thread::sleep_for(std::chrono::microseconds(m_sleepTimes(m_rng)));
        }
        
        int noEvents = kevent(
            kqFd,
            nullptr,                    // changelist (no changes)
            0,                          // no changes
            m_workerEvents[workerNum],  // returned events stored in the worker's event array
            kMaxEvents,
            &timeout                    // 0 trimeout
        );

        if (noEvents <= 0)
        {
            looping = false;
            continue;
        }

        looping = true;
        spdlog::info("[fd {}] Worker thread received {} events", kqFd, noEvents);
        for (int i = 0; i < noEvents; i++)
        {
            const struct kevent& event = m_workerEvents[workerNum][i];
            data = reinterpret_cast<ClientContext*>(event.udata);

            // Socket was closed by peer, or error occured
            if ((event.flags & EV_EOF) || (event.flags & EV_ERROR))
                killClient(kqFd, data->fd, data);

            // If we receive read or write notification
            else if (event.filter == EVFILT_READ || event.filter == EVFILT_WRITE)
                handleEvent(kqFd, data, event); // TODO

            // Fallback for unexpected event
            else
                killClient(kqFd, data->fd, data);
        }
    }
}

void HTTPServer::handleEvent(int kqFd, ClientContext* ctx, const struct kevent& event)
{
    int clientFd = ctx->fd;
    ClientContext* requestCtx{ nullptr };
    ClientContext* responseCtx{ nullptr };

    // Peer closed connection, early kill
    if ((event.flags & EV_EOF) || (event.flags & EV_ERROR))
    {
        killClient(kqFd, clientFd, ctx);
        return;
    }
    
    // Handle reads
    if (event.filter == EVFILT_READ)
    {
        requestCtx = ctx;
        ssize_t bytesRead = recv(clientFd, requestCtx->buffer, k_maxBufferSize, 0);

        spdlog::info("[fd {}] Read notification, bytesRead = {}",
            clientFd, bytesRead);
        
        // recv succesful
        if (bytesRead > 0)
        {
            // Parse a HTTP response into a ConnectionContext
            responseCtx = new ClientContext(); 
            responseCtx->fd = clientFd;
            m_router.populateResponse(requestCtx, responseCtx);

            // Register for writes
            server::utils::unregisterKqFd(kqFd, clientFd, true, false);
            delete requestCtx;
            server::utils::registerKqFd(kqFd, clientFd, false, true, responseCtx);
        }

        // Peer has closed
        else if (bytesRead == 0)
            killClient(kqFd, clientFd, requestCtx);
        
        // Fatal error
        else if (bytesRead < 0 && (errno != EAGAIN || errno != EWOULDBLOCK))
            killClient(kqFd, clientFd, requestCtx);
    }

    // Handle writes
    else if (event.filter == EVFILT_WRITE)
    {
        responseCtx = ctx;
        size_t bytesSent = send(
            clientFd,
            responseCtx->buffer + responseCtx->cursor,  // offset into buffer    
            responseCtx->length,                        // bytes to send
            0
        );
        
        spdlog::info("[fd {}] Write notification, response buffer = {}, bytesSent = {}",
            clientFd, responseCtx->length, bytesSent);
        
        if (bytesSent >= 0)
        {
            // Still more bytes to send 
            if (responseCtx->length > bytesSent)
            {
                // Advance cursor and re-arm for writes
                responseCtx->cursor += bytesSent;
                responseCtx->length -= bytesSent;
                server::utils::registerKqFd(kqFd, clientFd, false, true, responseCtx);
                spdlog::info("[fd {}] More bytes to write, re-armed for write notifications", clientFd);
            }
            
            // Finished sending, go back to reading
            else
            {
                requestCtx = new ClientContext();
                requestCtx->fd = clientFd;

                // Register for reads
                server::utils::unregisterKqFd(kqFd, clientFd, false, true);
                delete responseCtx;
                server::utils::registerKqFd(kqFd, clientFd, true, false, requestCtx);
                spdlog::info("[fd {}] Finished writing, re-armed for read notifications", clientFd);
            }
        }

        // Non fatal error, try again
        else if (errno == EAGAIN || errno == EWOULDBLOCK)
            server::utils::registerKqFd(kqFd, clientFd, false, true, responseCtx);
        
        // bytesSent < 0, fatal error
        else
            killClient(kqFd, clientFd, responseCtx);
    }

    // Unexpected filter
    else
        killClient(kqFd, clientFd, ctx);
}

void HTTPServer::killClient(int kqFd, int clientFd, ClientContext* ctx)
{
    m_clientFds.erase(clientFd);
    server::utils::unregisterKqFd(kqFd, clientFd, true, true);
    close(clientFd);
    delete ctx;
}