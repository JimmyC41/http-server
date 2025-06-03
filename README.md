# KQueue HTTP/1.1 Server

A multi-threaded, [KQueue](https://man.freebsd.org/cgi/man.cgi?kqueue) HTTP/1.1 server written in C++, inspired by the [Reactor Pattern](https://en.wikipedia.org/wiki/Reactor_pattern) and [epoll-http](https://github.com/geoboom/epoll-http). It is highly concurrent, supporting 10K simultaneous clients at a throughput of 30K+ requests-per-second (RQS).

## Prerequisites

- C++ compiler with C++23 support
- CMake Version 4.0.2
- Google Test
- spdlog

On MacOS with Homebrew:
```
brew install cmake googletest tbb
```

## Building and Usage

Configure the root level CMakeLists.txt and build.
```
cmake -S . -B build/
cd build
make
./main
```

You can use curl to exercise the API. By default, the server listens on port 8080 and servers an endpoint "/GET" and returns an 200 OK response with body "Hello, Optiver!".

```
curl -i http:://localhost::8080/get
```

In Server::start(), you may register additional endpoints:

```
m_router.registerHandler("/hello", Method::GET, [](const Request&)
{
    Response res(StatusCode::Ok);
    res.setContent("Hello, Optiver!");
    return res;
});
```

To shutdown the server:

```
quit
```

This will join the listener and worker threads, and close any open sockets.

> [!WARNING]
> For a graceful shutdown of threads and open sockets, remember to run the quit command!

## Benchmarking

To benchmark throughput, I used [wrk](https://github.com/wg/wrk), a HTTP benchmarking tool.

To run a benchmark for 30 seconds and keeping 10,000 HTTP concurrent connections:

```
brew install wrk
wrk -10k -d30s http://localhost:8080/hello
```

Alternatively, you can use the sh script provided.

```
./benchmark.sh
```

For example, my benchmark showed a throughput of 30K+ requests-per-second (RQS) for 10K simultaneous connections over a duration of 30 seconds.

![Benchmark Result](https://github.com/JimmyC41/http-server/blob/main/Results.png?raw=true)

## Logging

For debugging and error logs, I used an asynchronous logger with a rotating file sink from [spdlog](https://github.com/gabime/spdlog), a fast C++ logging library. Log files can be found under build/logs/server.

> [!WARNING]
> Remember to clear the logs/ directory after each benchmark.

## How is Asynchronous I/O achieved?

The server relies on [Kqueue](https://en.wikipedia.org/wiki/Kqueue), an OS event notification interface in MacOS, for asynchronous networking I/O.

Under the hood, a single thread to listen for new TCP connections on a socket. On establishing a new TCP connection, the file descriptor is assigned to a worker in a thread pool, each initialized with their own kqueue instance. 

```
[info] Creating HTTPServer
[info] [fd 4] Socket bound to 127.0.0.1:8080
[info] Calling server.start()
[info] [fd 4] Listener socket thread started
[info] [fd 5] New client connection accepted
```

The worker thread then registers the connection with kqueue for new read events. 

```
[info] [fd 5] New client connection accepted
[info] [fd 5] Client registered for reads with worker thread fd 6
```

When the client sends a HTTP request, the worker thread is notified. The thread processes the request and registers the file descriptor for writes, parsing the raw HTTP response as a pointer to kqueue. Then, the client is registered for writes.

```
[info] [fd 6] Worker thread received 1 events
[info] [fd 5] Read notification, bytesRead = 82
[info] Response struct created, buffer size = 54
[info] [fd 5] Unregistered write notifications for worker fd 6
[info] [fd 5] Client registered for reads with worker thread fd 6
```

When the file descriptor is ready for writes, the worker thread sends. This hands off our raw HTTP response to the kernel's TCP stack. Thereafter, the file descriptor is registered for additional requests.

```
[info] [fd 6] Worker thread received 1 events
[info] [fd 5] Write notification, response buffer = 54, bytesSent = 54
[info] [fd 5] Unregistered write notifications for worker fd 6
[info] [fd 5] Client registered for reads with worker thread fd 6
[info] [fd 5] Finished writing, re-armed for read notifications
```

## References

HTTP parsing logic was borrowed from this [http-server](https://github.com/trungams/http-server), an epoll-based (Linux) HTTP server.
