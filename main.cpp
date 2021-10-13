#include "http.h"
#include <iostream>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <csignal>
#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/uio.h>
#include <sys/time.h>

static int sockfd;

void handle_connection(int, const struct sockaddr_in*);
void signal_handler(int);
std::string create_logline(const struct sockaddr_in*, const HTTP_Request&, const HTTP_Response&);

int main(){

    const int listen_port = 80;

    signal(SIGINT, signal_handler);

    log("Welcome to Nathan's Webserver!");
    log("Initialising...");

    // Define vars
    int newsockfd, portno = listen_port;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;

    // Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("Error creating socket");
    ok("Socket created");

    // Set socket options
    bool toggle = true;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &toggle, sizeof(int)); // To avoid re-binding issues

    memset((char *) &serv_addr, 0, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(portno);
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    // Bind server address to socket
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        error("Error binding socket");

    // Listen
    listen(sockfd, 5);
    // God I can't wait for C++20 std::format support on GCC/Clang
    ok(std::string("Started listening on port ").append(std::to_string(listen_port)));

    signal(SIGCHLD, SIG_IGN);
    clilen = sizeof(cli_addr);
    while (true) {

        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd < 0)
            error("Error on accept()");

        handle_connection(newsockfd, &cli_addr);

    }

    return 0;

}

void handle_connection(int sockfd, const struct sockaddr_in* cli_addr) {

    const int block_size = 1024;
    constexpr int buffer_size = block_size * 8; // 8192 bytes
    const int timeout_sec = 30;

    // Set socket timeout
    static struct timeval tv;
    tv.tv_sec = timeout_sec;
    tv.tv_usec = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
        error("setsockopt() failed");

    std::vector<char> buffer;
    int bytes_read = 0;

    // Read a HTTP request from sockfd into buffer
    try {
        std::array<char, block_size> block;
        do {
            // read() from socket into block
            bytes_read = read(sockfd, block.data(), block_size);
            if (bytes_read <= 0) {
                if (bytes_read == 0)
                    throw ServerException(http_code::HTTP_REQ_TIMEOUT, "Request timeout");
                else
                    error("Error reading from the socket");
            }
            // Check if the data won't exceed our buffer_size limit upon insertion
            if (buffer.size() + bytes_read > buffer_size - 1)
                throw ServerException(http_code::HTTP_CLI_ERR, "Request length too long");
            // Insert the block into the buffer
            buffer.reserve(bytes_read); // For performance
            buffer.insert(buffer.end(), block.begin(), block.begin() + bytes_read);
        } while (! contains_double_newline(buffer.cbegin(), buffer.cend())); // In case there is more to come eg interactive session
    } catch (const ServerException &e) {
        try {
            HTTP_Response(e).send_to(sockfd);
        } catch (const ServerException& e) {
            log("Exception happened while sending exception response!");
            log(e);
            return;
        }
        log(e);
        return;
    }
    
    buffer.push_back('\0');

    HTTP_Request request;
    try {
        request.parse(buffer.data());
    } catch (const ServerException &e) {
        HTTP_Response error_response(e);
        error_response.send_to(sockfd);
        log(create_logline(cli_addr, request, error_response));
        return;
    }

    try {
        HTTP_Response response(request);
        response.send_to(sockfd);
        log(create_logline(cli_addr, request, response));
    }
    catch (const ServerException &e) {
        try {
            HTTP_Response error_response(e);
            error_response.send_to(sockfd);
            log(create_logline(cli_addr, request, error_response));
        } catch (const ServerException &e) {
            log("Exception happened while sending exception response!");
            log(e);
            return;
        }
        return;
    }

    close(sockfd);

}

void signal_handler(int signum) {
    log(std::string("\nInterrupt signal (").append(std::to_string(signum)).append(") received.\nExiting."));
    if (sockfd > 0)
        close(sockfd);
    exit(0);
}

std::string create_logline(const struct sockaddr_in *cli_addr, const HTTP_Request& req, const HTTP_Response& response) {

    // TODO C++20 string fmt
    char ipaddress[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(cli_addr->sin_addr), ipaddress, INET_ADDRSTRLEN);
    const int msg_len = 2048;
    static char out[msg_len];
    snprintf(out, msg_len, "%s - - %s \"%s %s %s\" %d %d\n",
        ipaddress,
        time_now_fmt("[%d/%b/%Y:%H:%M:%S %z]").c_str(),
        method_to_str(req.method()).c_str(),
        req.target().c_str(),
        req.protocol().c_str(),
        static_cast<int>(response.response_code()),
        response.body_size()
    );

    return out;

}