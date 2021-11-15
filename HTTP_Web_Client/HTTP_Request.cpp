#include "HTTP_Request.h"


std::string HTTP_Request::make_get_request(const std::string &filename) {
    std::string request;
    request.append("GET ");
    if (filename[0] != '/') request.append("/");
    request.append(filename).append(" ").append("HTTP/1.1").append(CARRIAGE_RETURN_NEW_LINE);
    request.append(CARRIAGE_RETURN_NEW_LINE);
    return request;
}

std::string HTTP_Request::make_post_request(const std::string &filename) {
    std::string request;
    request.append("POST ").append("/").append(filename).append(" ")
            .append("HTTP/1.1").append(CARRIAGE_RETURN_NEW_LINE);

    std::string file_ext = filename.substr(filename.find_last_of('.') + 1);

    std::string content_type;
    std::unordered_set<std::string> jpeg_extensions = {"jpg", "jpeg", "jfif", "pjpeg", "pjp"};
    std::unordered_set<std::string> other_image_extensions = {"apng", "avif", "gif", "png", "svg", "webp"};
    if (file_ext == "txt") {
        content_type = "text/plain";
    } else if (file_ext == "html") {
        content_type = "text/html";
    } else if (jpeg_extensions.count(file_ext)) {
        content_type = "image/jpeg";
    } else if (other_image_extensions.count(file_ext)) {
        content_type = "image/" + file_ext;
    } else {
        std::cerr << "filename error\n";
        exit(1);
    }

    std::ifstream file(filename);
    std::string request_body;
    if (file) {
        file.seekg(0, std::ios::end);
        std::streampos length = file.tellg();
        file.seekg(0, std::ios::beg);
        std::vector<char> buffer(length);
        file.read(&buffer[0], length);
        std::stringstream localStream;
        localStream.rdbuf()->pubsetbuf(&buffer[0], length);
        file.close();

        request_body = localStream.str();
    } else {
        std::cerr << "Can't open file\n";
        exit(1);
    }

    request.append("Content-Type: ").append(content_type).append(CARRIAGE_RETURN_NEW_LINE);
    request.append("Content-Length: ").append(std::to_string(request_body.size())).append(CARRIAGE_RETURN_NEW_LINE);
    request.append(CARRIAGE_RETURN_NEW_LINE);

    request.append(request_body);
    return request;
}

std::vector<std::string> split_by_string(std::string text, std::string del) {
    std::vector<std::string> lines;
    size_t pos;
    std::string token;
    while ((pos = text.find(del)) != std::string::npos) {
        token = text.substr(0, pos);
        lines.push_back(token);
        text.erase(0, pos + del.length());
    }
    lines.push_back(text);
    return lines;
}

std::vector<std::string> HTTP_Request::split_by_char(const std::string &s, char delim) {
    std::vector<std::string> result;
    std::istringstream iss(s);
    std::string item;
    while (std::getline(iss, item, delim)) {
        result.push_back(item);
    }
    return result;
}

unsigned long HTTP_Request::get_content_length(const std::string &response) {
    std::stringstream ss(response);
    std::string curr_line;

    while (getline(ss, curr_line)) {
        std::vector<std::string> tokens = split_by_char(curr_line, ':');
        if (tokens[0] == CONTENT_LENGTH) {
            return std::stoi(tokens[1]);
        }
    }

    return 0;
}

unsigned long HTTP_Request::get_len_before_content_if_content_exists(const std::string& response) {
    std::string double_carriage_return;
    double_carriage_return.append(CARRIAGE_RETURN_NEW_LINE).append(CARRIAGE_RETURN_NEW_LINE);
    std::vector<std::string> tokens = split_by_string(response, double_carriage_return);
    if (tokens.size() == 2) return tokens[0].size() + double_carriage_return.size();
    return 0;
}
