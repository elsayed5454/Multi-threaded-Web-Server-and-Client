#ifndef MULTI_THREADED_WEB_SERVER_HTTP_H
#define MULTI_THREADED_WEB_SERVER_HTTP_H

#include <sstream>
#include <vector>
#include <iostream>
#include <algorithm>
#include <map>
#include <unordered_set>
#include <fstream>

#define CARRIAGE_RETURN_NEW_LINE "\r\n"
#define CARRIAGE_RETURN "\r"

class HTTP {
private:

    const std::string http_version = "HTTP/1.1";

    const std::unordered_map<int, std::string> status_codes = {{200, "OK"},
                                                               {404, "Not Found"}};

    std::string request_method;
public:
    const std::string &get_request_method() const;

private:
    std::string request_url;
    std::unordered_map<std::string, std::string> headers_values;
public:
    const std::unordered_map<std::string, std::string> &get_headers_values() const;

private:
    std::string response;

private:
    bool parse_request_line(const std::string &request_line);

    bool parse_header_line(const std::string &string);

    static std::vector<std::string> split(const std::string &s, char delim);

    void trim(std::string &s);

    void prepare_get_response();

    void prepare_post_response();

public:
    const std::string get_method = "GET";
    const std::string post_method = "POST";
    std::string entity_body;

    bool parse(const std::string &buffer);

    void prepare_response();

    const std::string &get_response() const;

    void prepare_bad_response();
};


#endif //MULTI_THREADED_WEB_SERVER_HTTP_H
