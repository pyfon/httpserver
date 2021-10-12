#include "http.h"
#include <fstream>
#include <sys/types.h>
#include <sys/socket.h>

HTTP_Response::HTTP_Response(const HTTP_Request& request) {

    protocol_version = request.protocol();
    update_target_path(request.target()); // May throw ServerException
    update_code_reason_phrase(http_code::HTTP_OK);
    update_headers();
    if (request.method() == http_method::HEAD)
        send_body = false;
    else
        send_body = true;

}

HTTP_Response::HTTP_Response(const ServerException& e) {

    protocol_version = "HTTP/1.1";
    send_body = true;
    update_code_reason_phrase(e.code());
    update_headers();

}

void HTTP_Response::send_to(const int &sockfd) {

    // Send the header
    std::string header = generate_header();
    send(sockfd, header.c_str(), header.size(), 0);

    if (! send_body)
        return;

    if (target_path.string().length()) {

        const int block_size = 4096; // bytes
        std::array<char, block_size> block;
        std::ifstream target_file(target_path.string());
        if (! target_file.is_open())
            throw ServerException(http_code::HTTP_SERVER_ERR, "Could not open file");
        
        while (! target_file.eof()) {
            target_file.read(block.data(), block_size);
            if (send(sockfd, block.data(), target_file.gcount(), 0) < 0)
                error("send() error");
        }

    } else {
        if (send(sockfd, msg_body.c_str(), msg_body.size(), 0) < 0)
            error("send() error");
    }

}

std::string HTTP_Response::generate_header() {

    // I'll consider optimising this

    std::string ret;

    // Status line
    ret.append(protocol_version);
    ret.append(" ");
    ret.append(std::to_string(static_cast<int>(response_code)));
    ret.append(" ");
    ret.append(reason_phrase);
    ret.append("\r\n");

    // HTTP headers
    for (const auto &i : headers.headers()) {
        ret.append(i);
        ret.append("\r\n");
    }

    ret.append("\r\n");

    return ret;

}

void HTTP_Response::update_target_path(const std::string &target) {

    auto fspath = (server_path / std::filesystem::path(target)).lexically_normal();
    auto [rootEnd, nothing] = std::mismatch(server_path.begin(), server_path.end(), fspath.begin());
    if (rootEnd != server_path.end())
        throw ServerException(http_code::HTTP_UNAUTHORIZED, "Nice try");
    if (! std::filesystem::exists(fspath))
        throw ServerException(http_code::HTTP_NOT_FOUND, "Not found");
    target_path = fspath;

}

void HTTP_Response::update_headers() {

    // Content-Length
    if (send_body)
        headers["Content-Length"] = std::to_string(body_size());
    // Date
    headers["Date"] = time_now_fmt("%a, %d %b %Y %H:%M:%S %Z");

    // Content-Type
    if (target_path.string().length()) {
        std::string mime_type = "application/octet-stream";
        std::string ext = target_path.extension();
        if (ext == "html")
            mime_type = "text/html";
        else if (ext == "txt")
            mime_type = "text/plain";
        headers["Content-Type"] = mime_type;
    }

}

int HTTP_Response::body_size() const {

    if (target_path.string().length()) {
        std::ifstream in(target_path.string(), std::ifstream::ate | std::ifstream::binary);
        return in.tellg();
    } else {
        return msg_body.size();
    }

}

void HTTP_Response::update_code_reason_phrase(const http_code& code){

    response_code = code;

    switch (response_code) {
    case http_code::HTTP_OK:
        reason_phrase = "OK";
        break;
    case http_code::HTTP_RD_PERM:
        reason_phrase = "Moved Permanently";
        break;
    case http_code::HTTP_RD_TEMP:
        reason_phrase = "Found";
        break;
    case http_code::HTTP_CLI_ERR:
        reason_phrase = "Malformed Request";
        break;
    case http_code::HTTP_UNAUTHORIZED:
        reason_phrase = "Unauthorized";
        break;
    case http_code::HTTP_NOT_FOUND:
        reason_phrase = "Not Found";
        break;
    case http_code::HTTP_REQ_TIMEOUT:
        reason_phrase = "Request Timeout";
        break;
    case http_code::HTTP_SERVER_ERR:
        reason_phrase = "Internal Server Error";
        break;
    case http_code::HTTP_NOT_IMPLEMENTED:
        reason_phrase = "Not Implemented";
        break;
    default:
        break;
    }

}