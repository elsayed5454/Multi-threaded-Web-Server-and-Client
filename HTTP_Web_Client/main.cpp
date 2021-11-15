#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>
#include <iostream>
#include <arpa/inet.h>
#include "HTTP_Request.h"

#define BUFFER_SIZE 512000 // max number of bytes we can get at once (0.5 Mb)
#define CLIENT_GET "client-get"
#define CLIENT_POST "client-post"

int main(int argc, char *argv[]) {

    if (argc < 1 || argc > 3) {
        std::cerr << "Usage: ./client (server_ip) (port_number)\nwhere the requests are in file requests.txt\n";
        std::cerr << "and each request in the form: client-get/client-post file-path\n";
        exit(1);
    }

    char *servIP = const_cast<char *>((argc == 2) ? argv[2] : "127.0.0.1");
    in_port_t port_number = (argc == 3) ? atoi(argv[2]) : 80;

    std::string request;
    std::ifstream requests_file("requests.txt");
    std::string curr_line;

    while (getline(requests_file, curr_line)) {
        // Create a reliable, stream socket using TCP
        int clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (clientSocket < 0) {
            std::cerr << "socket() failed\n";
            exit(1);
        }

        struct sockaddr_in server_address{};
        memset(&server_address, 0, sizeof(server_address));
        server_address.sin_family = AF_INET;
        int rtnVal = inet_pton(AF_INET, servIP, &server_address.sin_addr.s_addr);
        if (rtnVal == 0) {
            std::cerr << "inet_pton() failed, invalid address string\n";
            exit(1);
        } else if (rtnVal < 0) {
            std::cerr << "inet_pton() failed\n";
            exit(1);
        }
        server_address.sin_port = htons(port_number);

        // Establish the connection
        if (connect(clientSocket, (struct sockaddr *) &server_address, sizeof(server_address)) < 0) {
            std::cerr << "connect() failed\n";
            exit(1);
        }
        printf("Connection established\n");

        std::vector<std::string> tokens = HTTP_Request::split_by_char(curr_line, ' ');
        if (tokens.size() != 2) {
            std::cerr << "error while reading from requests file\n";
            exit(1);
        }
        if (tokens[0] == CLIENT_GET) {
            request = HTTP_Request::make_get_request(tokens[1]);
        } else if (tokens[0] == CLIENT_POST) {
            request = HTTP_Request::make_post_request(tokens[1]);
        } else {
            std::cerr << "unsupported request method\n";
            exit(1);
        }

        char buffer[BUFFER_SIZE] = "";
        ssize_t totalBytesSent = 0;
        ssize_t numBytesSent;
        unsigned long request_index = 0, buffer_index;
        while (totalBytesSent < request.size()) {
            buffer_index = 0;
            for (; request_index < request.size() && buffer_index < BUFFER_SIZE; request_index++, buffer_index++) {
                buffer[buffer_index] = request.at(request_index);
            }

            if ((numBytesSent = send(clientSocket, buffer, buffer_index, 0)) < 0) {
                std::cerr << "send() failed\n";
                exit(1);
            }
            totalBytesSent += numBytesSent;
        }

        std::cout << "<<<<<<<<<<<<<<<Sent: " << request;

        std::string rcvd;
        ssize_t numBytesRcvd, totalBytesRcvd = 0;
        if ((numBytesRcvd = recv(clientSocket, &buffer, sizeof(buffer), 0)) < 0) {
            std::cerr << "recv() failed\n";
            exit(1);
        }
        totalBytesRcvd += numBytesRcvd;
        rcvd.append(buffer, buffer + numBytesRcvd);

        unsigned long len = HTTP_Request::get_len_before_content_if_content_exists(rcvd);
        if (len != 0) {
            unsigned long content_len = HTTP_Request::get_content_length(rcvd);
            unsigned long curr_content_len = rcvd.size() - len;
            while (curr_content_len < content_len) {
                if ((numBytesRcvd = recv(clientSocket, &buffer, sizeof(buffer), 0)) < 0) {
                    std::cerr << "recv() failed\n";
                    exit(1);
                }
                curr_content_len += numBytesRcvd;
                rcvd.append(buffer, buffer + numBytesRcvd);
                totalBytesRcvd += numBytesRcvd;
            }
        }

        std::cout << ">>>>>>>>>>>>>>>Received: " << rcvd;
        close(clientSocket);
    }
    return 0;
}