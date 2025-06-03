#pragma once

#include <utility>

constexpr size_t k_maxBufferSize = 4096;

struct ClientContext
{
    int fd;
    size_t length;
    size_t cursor;
    char buffer[k_maxBufferSize];

    ClientContext() : fd(0), length(0), cursor(0), buffer() {}
};