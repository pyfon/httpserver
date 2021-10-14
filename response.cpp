#include "http.h"
#include <algorithm>
#include <fstream>
#include <cstdio>
#include <sys/types.h>
#include <sys/socket.h>

HTTP_Response::HTTP_Response(const HTTP_Request& request) {

    _body_size = 0;
    protocol_version = request.protocol();
    update_target_path(request.target()); // May throw ServerException
    update_code_reason_phrase(http_code::HTTP_OK);
    if (request.method() == http_method::HEAD)
        send_body = false;
    else
        send_body = true;

}

HTTP_Response::HTTP_Response(const ServerException& e) {

    _body_size = 0;
    protocol_version = "HTTP/1.1";
    send_body = true;
    update_code_reason_phrase(e.code());
    // TODO C++20 fmt
    msg_body = std::to_string(static_cast<int>(e.code()));
    msg_body.append(" Error: ");
    msg_body.append(e.what());
    msg_body.append("\r\n");

}

void HTTP_Response::send_to(const int &sockfd) {

    // Send the header
    update_headers();
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
        if (send(sockfd, "\r\n", 2, 0) < 0)
            error("send() error");

    } else {
        if (send(sockfd, msg_body.c_str(), msg_body.size(), 0) < 0)
            error("send() error");
    }

}

std::string HTTP_Response::generate_header() {

    // TODO C++20 fmt

    std::string ret;
    const int status_line_len = 128;
    char status_line[status_line_len];
    snprintf(status_line, status_line_len, "%s %d %s\r\n", protocol_version.c_str(), static_cast<int>(_response_code), reason_phrase.c_str());
    ret.append(status_line);

    // HTTP headers
    for (const auto &i : headers.headers()) {
        ret.append(i);
        ret.append("\r\n");
    }

    ret.append("\r\n");

    return ret;

}

void HTTP_Response::update_target_path(const std::string &target) {

    std::filesystem::path fspath;

    if (target == "/") {
        fspath = server_path / std::filesystem::path(index_document);
    } else {
        fspath = (server_path / std::filesystem::path(target.substr(1, std::string::npos))).lexically_normal();
        auto [rootEnd, nothing] = std::mismatch(server_path.begin(), server_path.end(), fspath.begin());
        if (rootEnd != server_path.end())
            throw ServerException(http_code::HTTP_UNAUTHORIZED, "Nice try");
    }

    if (! std::filesystem::exists(fspath))
        throw ServerException(http_code::HTTP_NOT_FOUND, "Not found");
    target_path = fspath;

}

void HTTP_Response::update_headers() {

    // Content-Length
    if (send_body) {
        auto len = body_size();
        if (len)
            headers["Content-Length"] = std::to_string(len);
    }
    // Date
    headers["Date"] = time_now_fmt("%a, %d %b %Y %H:%M:%S %Z");

    // Content-Type
    if (target_path.string().length()) {
        std::string mime_type = "application/octet-stream";
        std::string ext = target_path.extension();
        std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c){ return std::tolower(c); });
        // This could be improved MASSIVELY
        // Like this is awful.
        const std::string plaintext_files[] = {".css", ".js", ".php", ".txt", ".c", ".cpp", ".ini"};
        if (ext == ".html" || ext == ".htm")
            mime_type = "text/html";
        else if (ext == ".jpg" || ext == ".jpeg")
            mime_type = "image/jpeg";
        else if (ext == "png")
            mime_type = "image/png";
        else if (ext == "bmp")
            mime_type = "image/bmp";
        else {
            for (const auto &i : plaintext_files) {
                if (ext == i) {
                    mime_type = "text/plain";
                    break;
                }
            }
        }
        headers["Content-Type"] = mime_type;
    }

    // Connection
    headers["Connection"] = "close";

}

int HTTP_Response::body_size() {

    if (! _body_size) {
        if (target_path.string().length()) {
            std::ifstream in(target_path.string(), std::ifstream::ate | std::ifstream::binary);
            if (in.bad())
                throw ServerException(http_code::HTTP_SERVER_ERR, "Couldn't open file");
            _body_size = in.tellg();
        } else {
            _body_size = msg_body.size();
        }
    }

    return _body_size;

}

http_code HTTP_Response::response_code() const noexcept {
    return _response_code;
}

void HTTP_Response::update_code_reason_phrase(const http_code& code){

    _response_code = code;

    switch (_response_code) {
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