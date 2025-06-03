#include <iterator>
#include <algorithm>
#include <sstream>

#include "HTTPUtils.hpp"
#include "Request.hpp"
#include "Response.hpp"

namespace http::utils
{

std::string toString(Method method)
{
    switch (method)
    {
    case Method::GET:
        return "GET";
    case Method::HEAD:
        return "HEAD";
    case Method::POST:
        return "POST";
    case Method::PUT:
        return "PUT";
    case Method::DELETE:
        return "DELETE";
    case Method::CONNECT:
        return "CONNECT";
    case Method::OPTIONS:
        return "OPTIONS";
    case Method::TRACE:
        return "TRACE";
    case Method::PATCH:
        return "PATCH";
    default:
        return std::string();
  }
}

std::string toString(Version version)
{
    switch (version) {
    case Version::HTTP_0_9:
        return "HTTP/0.9";
    case Version::HTTP_1_0:
        return "HTTP/1.0";
    case Version::HTTP_1_1:
        return "HTTP/1.1";
    case Version::HTTP_2_0:
        return "HTTP/2.0";
    default:
        return std::string();
    }
}

std::string toString(StatusCode code)
{
    switch (code) {
    case StatusCode::Continue:
        return "Continue";
    case StatusCode::Ok:
        return "OK";
    case StatusCode::Accepted:
        return "Accepted";
    case StatusCode::MovedPermanently:
        return "Moved Permanently";
    case StatusCode::Found:
        return "Found";
    case StatusCode::BadRequest:
        return "Bad Request";
    case StatusCode::Forbidden:
        return "Forbidden";
    case StatusCode::NotFound:
        return "Not Found";
    case StatusCode::MethodNotAllowed:
        return "Method Not Allowed";
    case StatusCode::ImATeapot:
        return "I'm a Teapot";
    case StatusCode::InternalServerError:
        return "Internal Server Error";
    case StatusCode::NotImplemented:
        return "Not Implemented";
    case StatusCode::BadGateway:
        return "Bad Gateway";
    default:
        return std::string();
  }
}

Method toMethod(const std::string& string)
{
    std::string methodStr;
    std::transform(string.begin(), string.end(),
                    std::back_inserter(methodStr),
                    [](char c) { return toupper(c); });
    
    if (methodStr == "GET")
        return Method::GET;
    else if (methodStr == "HEAD")
        return Method::HEAD;
    else if (methodStr == "POST")
        return Method::POST;
    else if (methodStr == "PUT")
        return Method::PUT;
    else if (methodStr == "DELETE")
        return Method::DELETE;
    else if (methodStr == "CONNECT")
        return Method::CONNECT;
    else if (methodStr == "OPTIONS")
        return Method::OPTIONS;
    else if (methodStr == "TRACE")
        return Method::TRACE;
    else if (methodStr == "PATCH")
        return Method::PATCH;
    else
        throw std::invalid_argument("Unexpected HTTP method");
}

Version toVersion(const std::string& string)
{
    std::string versionStr;
    std::transform(string.begin(), string.end(),
                    std::back_inserter(versionStr),
                    [](char c) { return toupper(c); });
    
    if (versionStr == "HTTP/0.9")
        return Version::HTTP_0_9;
    else if (versionStr == "HTTP/1.0")
        return Version::HTTP_1_0;
    else if (versionStr == "HTTP/1.1")
        return Version::HTTP_1_1;
    else if (versionStr == "HTTP/2" ||
                versionStr == "HTTP/2.0")
        return Version::HTTP_2_0;
    else
        throw std::invalid_argument("Unexpected HTTP version");
}

std::string toString(const Request& request)
{
    std::ostringstream oss;
    oss << toString(request.method()) << ' ';
    oss << request.uri().path() << ' ';
    oss << toString(request.version()) << "\r\n";
    for (const auto& p : request.headers())
        oss << p.first << ": " << p.second << "\r\n";
    oss << "\r\n";
    oss << request.content();
    return oss.str();
}

std::string toString(const Response& response, bool sendBody)
{
    std::ostringstream oss;
    oss << toString(response.version()) << ' ';
    oss << static_cast<int>(response.statusCode()) << ' ';
    oss << toString(response.statusCode()) << "\r\n";
    if (sendBody)
        oss << "Content-Length: " << response.contentLength() << "\r\n";
    for (const auto& p : response.headers())
        oss << p.first << ": " << p.second << "\r\n";
    oss << "\r\n";
    if (sendBody)
        oss << response.content();
    return oss.str();
}

Request toRequest(const std::string& string)
{
    std::string startLine, headerLines, messageBody;
    std::istringstream iss;
    Request request;
    std::string line, method, path, version;
    std::string key, value;
    Uri uri;
    size_t lpos = 0, rpos = 0;

    rpos = string.find("\r\n", lpos);
    if (rpos == std::string::npos)
        throw std::invalid_argument("Could not find request start line");

    startLine = string.substr(lpos, rpos - lpos);
    lpos = rpos + 2;
    rpos = string.find("\r\n\r\n", lpos);
    if (rpos != std::string::npos) // has header
    {
        headerLines = string.substr(lpos, rpos - lpos);
        lpos = rpos + 4;
        rpos = string.length();
        if (lpos < rpos)
            messageBody = string.substr(lpos, rpos - lpos);
    }

    iss.clear();
    iss.str(startLine);
    iss >> method >> path >> version;
    if (!iss.good() && !iss.eof())
        throw std::invalid_argument("Invalid start line format");

    request.setMethod(toMethod(method));
    request.setUri(Uri(path));
    if (toVersion(version) != request.version())
        throw std::logic_error("HTTP version not supported");

    iss.clear();
    iss.str(headerLines);
    while (std::getline(iss, line))
    {
        std::istringstream headerStream(line);
        std::getline(headerStream, key, ':');
        std::getline(headerStream, value);

        // remove whitespaces from the two strings
        key.erase(std::remove_if(key.begin(), key.end(),
            [](char c) { return std::isspace(c); }), key.end());
        
        value.erase(std::remove_if(value.begin(), value.end(),
            [](char c) { return std::isspace(c); }), value.end());
        
        request.setHeader(key, value);
    }

    request.setContent(messageBody);
    return request;
}

}