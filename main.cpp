#include "http.h"
#include <iostream>
#include <cstdio>
#include <cstring>
#include <csignal>
#include <cstdlib>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/uio.h>
#include <unistd.h>

#define BUFSIZE 256

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
    // To avoid re-binding issues
    bool reuseaddr_on = true;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr_on, sizeof(int));

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

    std::array<char, BUFSIZE> buffer;
    buffer.fill(0);
    auto n = read(sockfd, buffer.data(), BUFSIZE - 1);
    if (n < 0)
        error("Error reading from the socket");
    
    HTTP_Request request;
    try {
        request.parse(buffer.data());
    } catch (const ServerException &e) {
        //HTTP_Response response;
        // send error
        // maybe add a method to HTTP_Response called generateerror() that excepts const ServerException&?
        // then send response.dump()
        log(std::to_string(static_cast<int>(e.code())).append(" Error: ").append(e.what())); // ew.
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