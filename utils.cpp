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

bool strip_cr(std::string &str) {

    if (str.back() == '\r') {
        str.pop_back();
        return true;
    }

    return false;
}

// HeaderMap

std::string HeaderMap::operator[](std::string key) {

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

    auto elems = splitline(line, ":");
    if (elems.size() != 2)
        throw ServerException(http_code::HTTP_CLI_ERR, std::string("Header ").append(line).append(" not valid")); // TODO replace with C++20 fmt
    // Strip whitespace
    for (auto &i : elems)
        i.erase(std::remove_if(i.begin(), i.end(), isspace), i.end());

    map[elems[0]] = elems[1];
    return;

}

std::string time_now_fmt(std::string fmt) {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);

    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), fmt.c_str());
    return ss.str();

}