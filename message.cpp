#include "http.h"
#include <iostream>

void ok(const std::string &msg) {
    std::cout << "[ \u001b[32mok\u001b[0m ] " << msg << std::endl;
}

void error(const std::string &msg) {
    perror(msg.c_str());
    exit(1);
}

void log(const std::string &msg) {

    std::cout << msg << std::endl;

}

void log(const ServerException& e) {

    log(std::to_string(static_cast<int>(e.code())).append(" Error: ").append(e.what())); // ew.

}