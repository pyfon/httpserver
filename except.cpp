#include "http.h"

const char *ServerException::what() const noexcept {

    return this->message.c_str();

}

http_code ServerException::code() const noexcept {

    return _code;

}