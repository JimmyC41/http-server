#include <string>

#include "Router.hpp"
#include "Enum.hpp"
#include "Request.hpp"
#include "Response.hpp"
#include "HTTPUtils.hpp"
#include "spdlog/spdlog.h"

void Router::registerHandler(const std::string& path, Method method, RequestHandler callback)
{
    Uri uri(path);
    m_handlers[uri][method] = std::move(callback);
}

Response Router::getResponse(const Request& request)
{
    auto it = m_handlers.find(request.uri());
    if (it == m_handlers.end())
        return Response(StatusCode::NotFound);
    
    auto callbackIt = it->second.find(request.method());
    if (callbackIt == it->second.end())
        return Response(StatusCode::MethodNotAllowed);
    
    return callbackIt->second(request);
}

void Router::populateResponse(ClientContext* request, ClientContext* response)
{
    std::string requestStr(request->buffer), responseStr;
    Request httpRequest;
    Response httpResponse;

    try
    {
        httpRequest = http::utils::toRequest(requestStr);
        httpResponse = getResponse(httpRequest);
    }
    catch(const std::invalid_argument &e)
    {
        httpResponse = Response(StatusCode::BadRequest);
        httpResponse.setContent(e.what());
    }
    catch(const std::logic_error &e)
    {
        httpResponse = Response(StatusCode::HttpVersionNotSupported);
        httpResponse.setContent(e.what());
    }
    catch(const std::exception &e)
    {
        httpResponse = Response(StatusCode::InternalServerError);
        httpResponse.setContent(e.what());
    }
    
    responseStr = http::utils::toString(httpResponse, httpRequest.method() != Method::HEAD);
    memcpy(response->buffer, responseStr.c_str(), k_maxBufferSize);
    response->length = responseStr.length();

    spdlog::info("[fd {}] Response struct created, buffer size = {}",
        request->fd, response->length);
}