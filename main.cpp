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
#include <sys/uio.h>
#include <sys/time.h>

static int sockfd;

void handle_connection(int);
void signal_handler(int);

int main(){

    signal(SIGINT, signal_handler);

    log("Welcome to Nathan's Webserver!");
    log("Initialising...");

    // Define vars
    int newsockfd, portno = LISTEN_PORT;
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
    ok(std::string("Started listening on port ").append(std::to_string(LISTEN_PORT)));

    signal(SIGCHLD, SIG_IGN);
    clilen = sizeof(cli_addr);
    while (true) {

        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd < 0)
            error("Error on accept()");

        handle_connection(newsockfd);

    }

    return 0;

}

void handle_connection(int sockfd) {

    // TODO socket timeouts

    const int block_size = 1024;
    constexpr int buffer_size = block_size * 8; // 8192 bytes
    std::vector<char> buffer;
    int bytes_read = 0;

    try {
        do {
            // read() from socket into block
            std::array<char, block_size> block;
            bytes_read = read(sockfd, block.data(), block_size);
            // Check if the data won't exceed our buffer_size limit upon insertion
            if (buffer.size() + bytes_read > buffer_size - 1)
                throw ServerException(http_code::HTTP_CLI_ERR, "Request length too long");
            // Insert the block into the buffer
            buffer.reserve(bytes_read); // For performance
            buffer.insert(buffer.end(), block.begin(), block.begin() + bytes_read);
        } while (! contains_double_newline(buffer.cbegin(), buffer.cend())); // In case there is more to come eg interactive session
    } catch (const ServerException &e) {
        log(e);
        return;
        // TODO send error HTTP response, then return.
    }

    if (bytes_read < 0)
        error("Error reading from the socket");
    
    buffer.push_back('\0');

    HTTP_Request request;
    try {
        request.parse(buffer.data());
    } catch (const ServerException &e) {
        //HTTP_Response response;
        // send error
        // maybe add a method to HTTP_Response called generateerror() that excepts const ServerException&?
        // then send response.dump()
        log(e);
        return;
    }

    // debug
    log(request.dump());
    // request.resource()
    // check if file exists, send if not 404
}

void signal_handler(int signum) {
    log(std::string("\nInterrupt signal (").append(std::to_string(signum)).append(") received.\nExiting."));
    if (sockfd > 0)
        close(sockfd);
    exit(0);
}