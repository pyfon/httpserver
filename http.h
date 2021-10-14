#include <string>
#include <unordered_map>
#include <vector>
#include <stdexcept>
#include <array>
#include <iterator>
#include <filesystem> // C++17
#include <utility>

// Typedefs

// Constants

const std::array<std::string, 5> supported_http_versions {"0.9", "1.0", "1.1", "2", "3"};
extern std::filesystem::path server_path;
extern std::string index_document;

// Enums

enum class http_method {
    GET,
    HEAD
};

enum class http_code {
    HTTP_OK = 200,
    HTTP_RD_PERM = 301,
    HTTP_RD_TEMP = 302,
    HTTP_CLI_ERR = 400,
    HTTP_UNAUTHORIZED = 401,
    HTTP_NOT_FOUND = 404,
    HTTP_REQ_TIMEOUT = 408,
    HTTP_SERVER_ERR = 500,
    HTTP_NOT_IMPLEMENTED = 501
};

// Exceptions

class ServerException : public std::exception {

    http_code _code;
    std::string message;

public:

    ServerException(const http_code& codearg, const std::string& msg) : _code(codearg), message(msg) {};
    const char *what() const noexcept;
    http_code code() const noexcept;

};

// Classes

// Basically a wrapper around unordered_map to store headers
class HeaderMap {

    std::unordered_map<std::string, std::string> map;
    std::pair<std::string, std::string> split_header(std::string&); // Into key, value

public:

    std::string &operator[](std::string);
    std::string header(std::string); // Formatted header for key
    std::vector<std::string> headers(); // List of all headers
    void add(std::string); // Parse header string and add to map

};

class HTTP_Request {

private:

    http_method _method;
    std::string _target;
    std::string protocol_version;
    HeaderMap headers;
    std::string msg_body;

public:

    void parse(const std::string&);
    http_method method() const noexcept;
    std::string protocol() const noexcept;
    std::string target() const noexcept;
    std::string dump(); // For debugging

};

class HTTP_Response {

private:

    std::filesystem::path target_path; // Path on filesystem
    http_code _response_code;
    std::string reason_phrase;
    std::string protocol_version;
    HeaderMap headers;
    std::string msg_body; // Only used if target_path is empty
    bool send_body;

    std::string generate_header(); // Whole HTTP header
    void update_headers(); // Content-Length requires target_path or msg_body
    // Locate relative request target on disk, save to target_path. Throws ServerException:
    void update_target_path(const std::string&);
    void update_code_reason_phrase(const http_code&);

public:

    HTTP_Response(const HTTP_Request&);
    HTTP_Response(const ServerException&);
    void send_to(const int&);
    http_code response_code() const noexcept;
    int body_size() const; // bytes

};

// Functions

void ok(const std::string&);
void error(const std::string&);
void error_no_perror(const std::string &msg);
void log(const std::string&);
void log(const ServerException&);
// Tokenise strings by the delimiter (arg2) in the input string (arg1), return results as vector.
std::vector<std::string> splitline(const std::string&, const std::string&);
// Returns true if CR from end of string was stripped.
bool strip_cr(std::string&);
std::string time_now_fmt(std::string);
std::string method_to_str(const http_method&) noexcept;
std::string trim(const std::string&);

// Templates need to be in .h for compiler/linker reasons
template <class InputIterator> bool contains_double_newline(InputIterator first, InputIterator last) {
    bool crlf_found = false; // If found in the last iteration
    while (first != last) {
        if (*first == '\r' && *(first + 1) == '\n'){
            if (crlf_found)
                return true;
            crlf_found = true;
            first += 2;
        } else {
            crlf_found = false;
            first++;
        }
    }
    return false;
}
