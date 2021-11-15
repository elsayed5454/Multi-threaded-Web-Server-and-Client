#include "HTTP.h"

bool HTTP::parse(const std::string &buffer) {
    std::stringstream ss(buffer);

    std::string request_line;
    getline(ss, request_line);
    if (!parse_request_line(request_line)) return false;

    std::string header_line;
    while (getline(ss, header_line)) {
        trim(header_line);
        if (header_line == CARRIAGE_RETURN) break;

        if (!parse_header_line(header_line)) return false;
    }

    entity_body = std::string(std::istreambuf_iterator<char>(ss), {});
    if (request_method == get_method && !entity_body.empty()) return false;

    return true;
}

bool HTTP::parse_request_line(const std::string &request_line) {
    std::cout << request_line;
    std::vector<std::string> tokens = split(request_line, ' ');

    if (tokens.size() != 3) return false;
    if (tokens[0] == get_method) {
        request_method = get_method;
        request_url = std::string(tokens[1]);
    } else if (tokens[0] == post_method) {
        request_method = post_method;
        request_url = std::string(tokens[1]);
    } else {
        return false;
    }

    unsigned long req_ver_size = tokens[2].size();
    if (tokens[2][req_ver_size - 1] == '\r') tokens[2].pop_back();
    if (tokens[2] != http_version) return false;

    return true;
}

bool HTTP::parse_header_line(const std::string &header_line) {
    unsigned long colon_pos = header_line.find(':');
    std::string header_key = header_line.substr(0, colon_pos);

    // Repeated header_key key
    if (headers_values.count(header_key) == 1) return false;

    std::string value = header_line.substr(colon_pos + 1);
    trim(value);
    headers_values[header_key] = value;

    return true;
}

std::vector<std::string> HTTP::split(const std::string &s, char delim) {
    std::vector<std::string> result;
    std::istringstream iss(s);
    std::string item;
    while (std::getline(iss, item, delim)) {
        result.push_back(item);
    }
    return result;
}

void HTTP::trim(std::string &s) {
    // trim from start while ignoring carriage return
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !std::isspace(ch) || ch == '\r';
    }));

    // trim from end while ignoring carriage return
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !std::isspace(ch) || ch == '\r';
    }).base(), s.end());
}

void HTTP::prepare_response() {
    if (request_method == get_method) {
        prepare_get_response();
    } else if (request_method == post_method) {
        prepare_post_response();
    } else {
        prepare_bad_response();
    }
}

const std::string &HTTP::get_response() const {
    return response;
}

void HTTP::prepare_get_response() {
    response = "";
    if (request_url == "/") request_url += "index.html";
    if (request_url[0] == '/') request_url.erase(0, 1);

    std::string file_ext = request_url.substr(request_url.find_last_of('.') + 1);

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
        prepare_bad_response();
        return;
    }

    std::ifstream file(request_url);
    if (file) {
        file.seekg(0, std::ios::end);
        std::streampos length = file.tellg();
        file.seekg(0, std::ios::beg);
        std::vector<char> buffer(length);
        file.read(&buffer[0], length);
        std::stringstream localStream;
        localStream.rdbuf()->pubsetbuf(&buffer[0], length);
        file.close();

        entity_body = localStream.str();
    } else {
        prepare_bad_response();
        return;
    }

    response.append(http_version).append(" ")
            .append("200").append(" ").append(status_codes.at(200)).append(CARRIAGE_RETURN_NEW_LINE);
    response.append("Content-Type: ").append(content_type).append(CARRIAGE_RETURN_NEW_LINE);
    response.append("Content-Length: ").append(std::to_string(entity_body.size())).append(CARRIAGE_RETURN_NEW_LINE);
    response.append(CARRIAGE_RETURN_NEW_LINE);
    response.append(entity_body);
}

void HTTP::prepare_post_response() {
    response = "";

    if (request_url[0] == '/') request_url.erase(0, 1);
    std::ofstream out(request_url);
    if (!out.is_open()) {
        prepare_bad_response();
        return;
    }
    out << entity_body;
    out.close();

    response.append(http_version).append(" ")
            .append("200").append(" ").append(status_codes.at(200)).append(CARRIAGE_RETURN_NEW_LINE);
    response.append(CARRIAGE_RETURN_NEW_LINE);
}

void HTTP::prepare_bad_response() {
    response = "";
    response.append(http_version).append(" ")
            .append("404").append(" ").append(status_codes.at(404)).append(CARRIAGE_RETURN_NEW_LINE);
    response.append(CARRIAGE_RETURN_NEW_LINE);
}


const std::string &HTTP::get_request_method() const {
    return request_method;
}

const std::unordered_map<std::string, std::string> &HTTP::get_headers_values() const {
    return headers_values;
}
