#ifndef HTTP_WEB_CLIENT_HTTP_REQUEST_H
#define HTTP_WEB_CLIENT_HTTP_REQUEST_H

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>

#define CARRIAGE_RETURN_NEW_LINE "\r\n"
#define CONTENT_LENGTH "Content-Length"

class HTTP_Request {
public:
    static std::string make_get_request(const std::string &filename);

    static std::string make_post_request(const std::string &filename);

    static unsigned long get_content_length(const std::string& response);

    static unsigned long get_len_before_content_if_content_exists(const std::string& response);

    static std::vector<std::string> split_by_char(const std::string &s, char delim);
};


#endif //HTTP_WEB_CLIENT_HTTP_REQUEST_H
