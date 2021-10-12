#include "http.h"
#include <algorithm>
#include <sstream>

void HTTP_Request::parse(const std::string& data) {

    // Make data a stream
    std::istringstream iss(data);

    // Parse the request line
    {

        std::string reqline;
        if (! std::getline(iss, reqline))
            throw ServerException(http_code::HTTP_CLI_ERR, "Empty request data");

        if (! strip_cr(reqline))
            throw ServerException(http_code::HTTP_CLI_ERR, "CR character missing from request line");

        // Get tokens
        auto tokens = splitline(reqline, " ");
        if (tokens.size() != 3)
            throw std::invalid_argument("Request line invalid: more than 3 tokens.");

        // Parse request method
        auto reqmethod = tokens[0];
        if (reqmethod == "GET")
            _method = http_method::GET;
        else if (reqmethod == "HEAD")
            _method = http_method::HEAD;
        else
            throw ServerException(http_code::HTTP_NOT_IMPLEMENTED, "HTTP Method not supported"); // TODO method {} not supported when C++20 formatting

        // Save request target

        _target = tokens[1];

        // Parse protocol version

        if (tokens[2].substr(0, 5) != "HTTP/")
            throw ServerException(http_code::HTTP_CLI_ERR, "Invalid protocol version");
        auto version_number = tokens[2].substr(5);
        if (std::find(supported_http_versions.begin(), supported_http_versions.end(), version_number) == supported_http_versions.end())
            throw ServerException(http_code::HTTP_NOT_IMPLEMENTED, "HTTP protocol version not supported");
        protocol_version = version_number;

    }

    // Parse request headers
    {
        std::string headerline;
        while (std::getline(iss, headerline)) {
            if (! strip_cr(headerline))
                throw ServerException(http_code::HTTP_CLI_ERR, std::string("CR character missing from line ").append(headerline));
            if (headerline.length() == 0)
                break;
            headers.add(headerline);
        }
    }

    // Save message body
    {
        std::string bodyline;
        while (std::getline(iss, bodyline))
            msg_body.append(bodyline);
    }

    return;

}

http_method HTTP_Request::method() const noexcept {
    return _method;
}


std::string HTTP_Request::target() const noexcept {
    return _target;
}

// for debugging
std::string HTTP_Request::dump() {

    std::string ret;
    // I don't like this
    if (_method == http_method::GET)
        ret.append("GET ");
    else if (_method == http_method::HEAD)
        ret.append("HEAD ");
    ret.append(_target);
    ret.append(" HTTP/");
    ret.append(protocol_version);
    ret.append("\r\n");
    for (const auto &i : headers.headers()) {
        ret.append(i);
        ret.append("\r\n");
    }
    ret.append(msg_body);
    return ret;

}

std::string HTTP_Request::protocol() const noexcept {
    return protocol_version;
}