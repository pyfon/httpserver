#include <string>
#include <unordered_map>
#include <vector>
#include <stdexcept>
#include <array>
#include <iterator>

#define LISTEN_PORT 80

// Typedefs

// Constants

const std::array<std::string, 5> supported_http_versions {"0.9", "1.0", "1.1", "2", "3"};

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
    HTTP_NOT_FOUND = 404,
    HTTP_SERVER_ERR = 500,
    HTTP_NOT_IMPLEMENTED = 501
};

// Classes

// Basically a wrapper around unordered_map to store headers
class HeaderMap {

    std::unordered_map<std::string, std::string> map;

public:

    std::string operator[](std::string);
    std::string header(std::string); // Formatted header for key
    std::vector<std::string> headers(); // List of all headers
    void add(std::string); // Parse header string and add to map

};

class HTTP_Request {

private:

    http_method method;
    std::string _target;
    std::string protocol_version;
    HeaderMap headers;
    std::string msg_body;

public:

    void parse(const std::string&);
    std::string target();
    std::string dump(); // For debugging

};

class HTTP_Response {

private:

    float protocol_version;
    HeaderMap headers;
    std::string msg_body;
    void generate_date_headers();

public:

    HTTP_Response();
    std::string dump(); // add date and expires header when dumping

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

// Functions

void ok(const std::string&);
void error(const std::string&);
void log(const std::string&);
// Tokenise strings by the delimiter (arg2) in the input string (arg1), return results as vector.
std::vector<std::string> splitline(const std::string&, const std::string&);
// Returns true if CR from end of string was stripped.
bool strip_cr(std::string&);
