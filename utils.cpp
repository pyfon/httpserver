#include "http.h"
#include <algorithm>
#include <cctype>
#include <chrono>  // chrono::system_clock
#include <ctime> // localtime
#include <sstream> // stringstream
#include <iomanip> // put_time

// Splitline

std::vector<std::string> splitline(const std::string& line, const std::string& delim) {

    size_t pos_start = 0, pos_end, delim_len = delim.length();
    std::string token;
    std::vector<std::string> ret;

    while ((pos_end = line.find(delim, pos_start)) != std::string::npos) {
        token = line.substr(pos_start, pos_end - pos_start);
        pos_start = pos_end + delim_len;
        ret.push_back(token);
    }

    ret.push_back(line.substr(pos_start));
    return ret;

}

// Strip CR

std::string trim(const std::string& str) {
    const auto strbegin = str.find_first_not_of(" ");
    if (strbegin == std::string::npos)
        return "";
    const auto strend = str.find_last_not_of(" ");
    const auto strrange = strend - strbegin + 1;
    return str.substr(strbegin, strrange);
}

bool strip_cr(std::string &str) {

    if (str.back() == '\r') {
        str.pop_back();
        return true;
    }

    return false;
}

// HeaderMap

std::string &HeaderMap::operator[](std::string key) {

    return map[key];

}

std::string HeaderMap::header(std::string key) {

    // TODO replace with string formatting in C++20
    return key.append(std::string(": ").append(map[key]));

}
std::vector<std::string> HeaderMap::headers() {

    std::vector<std::string> ret(map.size());
    auto i = 0;
    for (const auto &header : map) {
        std::string header_line = header.first;
        header_line.append(": ");
        header_line.append(header.second);
        ret[i++] = header_line;
    }

    return ret;

}

void HeaderMap::add(std::string line) {

    auto split = split_header(line);
    map[split.first] = split.second;
    return;

}

std::pair<std::string, std::string> HeaderMap::split_header(std::string& line) {

    const std::string delim = ":";
    auto pos = line.find(delim);
    if (pos == std::string::npos)
        throw ServerException(http_code::HTTP_CLI_ERR, std::string("Header ").append(line).append(" not valid")); // TODO replace with C++20 fmt
    return std::make_pair(trim(line.substr(0, pos)), trim(line.substr(pos, std::string::npos)));

}

std::string time_now_fmt(std::string fmt) {

    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), fmt.c_str());
    return ss.str();

}

std::string method_to_str(const http_method& method) noexcept {

    switch (method) {
    case http_method::GET:
        return "GET";
        break;
    case http_method::HEAD:
        return "HEAD";
        break;
    default:
        return std::string();
        break;
    }

}