#pragma once

#include <string>

#include "Enum.hpp"

class Request;
class Response;

namespace http::utils
{

// Enum Helpers
std::string toString(Method method);
std::string toString(Version version);
std::string toString(StatusCode code);
Method toMethod(const std::string& string);
Version toVersion(const std::string& string);

// MessageInterface Helpers
std::string toString(const Request& request);
std::string toString(const Response& response, bool sendBody = true);
Request toRequest(const std::string& string);
Response toResponse(const std::string& string);

}