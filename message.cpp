#include "http.h"
#include <iostream>
#include <mutex>

std::mutex msgmtx;

void ok(const std::string &msg) {

    msgmtx.lock();
    std::cout << "[ \u001b[32mok\u001b[0m ] " << msg << std::endl;
    msgmtx.unlock();

}

void error(const std::string &msg) {

    msgmtx.lock();
    std::cout << "[ \u001b[91mfatal\u001b[0m ] ";
    perror(msg.c_str());
    exit(1);

}

void error_no_perror(const std::string &msg) {

    msgmtx.lock();
    std::cout << "[ \u001b[91mfatal\u001b[0m ] " << msg << std::endl;
    exit(1);

}

void log(const std::string &msg) {

    // TODO C++20 fmt
    msgmtx.lock();
    std::string ret = time_now_fmt("[%F %T] ");
    ret.append(msg);
    std::cout << ret << std::endl;
    msgmtx.unlock();

}

void log(const ServerException& e) {

    msgmtx.lock();
    log(std::to_string(static_cast<int>(e.code())).append(" Error: ").append(e.what())); // ew.
    msgmtx.unlock();

}