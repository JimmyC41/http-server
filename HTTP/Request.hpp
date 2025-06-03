#pragma once

#include "Enum.hpp"
#include "Uri.hpp"
#include "MessageInterface.hpp"

class Request : public MessageInterface
{
private:
    Method m_method;
    Uri m_uri;

public:
    Request() : m_method(Method::GET) {}
    ~Request() = default;

    void setMethod(Method method) {m_method = method; }
    void setUri(const Uri& uri) { m_uri = std::move(uri); }

    Method method() const { return m_method; }
    Uri uri() const { return m_uri; }

    friend std::string toString(const Request& request);
    friend std::string toRequest(const std::string& string);
};