#pragma once

#include <utility>
#include <map>
#include <functional>

#include "ClientContext.hpp"
#include "Request.hpp"
#include "Response.hpp"
#include "Uri.hpp"

using RequestHandler = std::function<Response(const Request&)>;

class Router
{
private:
    std::map<Uri, std::map<Method, RequestHandler>> m_handlers;
    Response getResponse(const Request& request);

public:
    Router() = default;
    void registerHandler(const std::string& path, Method method, RequestHandler callback);
    void populateResponse(ClientContext* request, ClientContext* response);
};