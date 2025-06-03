#pragma once

#include "MessageInterface.hpp"
#include "Enum.hpp"

#include <string>

class Response : public MessageInterface
{
private:
    StatusCode m_statusCode;

public:
    Response() : m_statusCode(StatusCode::Ok) {}
    Response(StatusCode code) : m_statusCode(code) {}
    ~Response() = default;

    StatusCode statusCode() const { return m_statusCode; }
    void setStatusCode(StatusCode code) { m_statusCode = code; }

    friend std::string toString(const Response& request, bool sendBody);
    // friend std::string toResponse(const std::string& string);
};