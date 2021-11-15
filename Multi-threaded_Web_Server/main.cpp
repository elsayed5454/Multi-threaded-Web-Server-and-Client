#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <queue>
#include <chrono>
#include "HTTP.h"

#define MAX_CONNECTIONS 10      // how many pending connections queue will hold
#define BUFFER_SIZE 512000      // 0.5 Mb buffer
#define POOL_SIZE 15
#define TIMEOUT_DELAY_SEC 5

pthread_t thread_pool[POOL_SIZE];
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

std::queue<int *> clients;

void *handle_client_request(int *pclient_socket) {
    int clientSocket = *pclient_socket;
    std::free(pclient_socket);

    // Timeout for send and receive
    struct timeval tv{};
    tv.tv_sec = TIMEOUT_DELAY_SEC;
    tv.tv_usec = 0;
    setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
    setsockopt(clientSocket, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof tv);

    char buffer[BUFFER_SIZE] = "";
    std::string rcvd;

    ssize_t numBytesRcvd;
    // Receive the request, the buffer size is enough in case of get request
    if ((numBytesRcvd = recv(clientSocket, &buffer, sizeof(buffer), 0)) < 0) {
        std::cerr << "recv() failed\n";
        exit(1);
    }
    if (numBytesRcvd == 0) return nullptr;    // Client closed connection
    rcvd.append(buffer, buffer + numBytesRcvd);

    HTTP http;
    if (!http.parse(rcvd) ||    // If bad request
        (http.get_request_method() == http.get_method && numBytesRcvd == BUFFER_SIZE)) { // Too long get request
        http.prepare_bad_response();
    }
    // There is still content for post method
    else if (http.get_request_method() == http.post_method) {
        unsigned long content_length = std::stoi(http.get_headers_values().at("Content-Length"));
        while (http.entity_body.size() < content_length) {  // Receive the rest of content if exists
            if ((numBytesRcvd = recv(clientSocket, &buffer, BUFFER_SIZE, 0)) < 0) {
                std::cerr << "recv() failed\n";
                exit(1);
            }
            http.entity_body.append(buffer, buffer + numBytesRcvd);
            rcvd.append(buffer, buffer + numBytesRcvd);
        }
        http.prepare_response();
    } else {
        http.prepare_response();
    }

    ssize_t totalBytesSent = 0;
    ssize_t numBytesSent;
    unsigned long response_len = http.get_response().size();
    unsigned long response_index = 0, buffer_index;
    while (totalBytesSent < response_len) {
        buffer_index = 0;
        for (; response_index < response_len && buffer_index < BUFFER_SIZE; response_index++, buffer_index++) {
            buffer[buffer_index] = http.get_response().at(response_index);
        }

        if ((numBytesSent = send(clientSocket, buffer, buffer_index, 0)) < 0) {
            std::cerr << "send() failed\n";
            exit(1);
        }
        totalBytesSent += numBytesSent;
    }

    close(clientSocket);
    printf("Connection closed\n");

    return nullptr;
}

void *thread_func(void *arg) {
    while (true) {
        pthread_mutex_lock(&lock);
        pthread_cond_wait(&cond, &lock);
        if (!clients.empty()) {
            int *pclient_socket = clients.front();
            clients.pop();
            if (pclient_socket != NULL) {
                handle_client_request(pclient_socket);
            }
        }
        pthread_mutex_unlock(&lock);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 1 || argc > 2) {
        std::cerr << "Usage: ./server (port_number)\n";
        exit(1);
    }
    in_port_t port_number = (argc == 2) ? atoi(argv[1]) : 80;

    for (int i = 0; i < POOL_SIZE; ++i) {
        pthread_create(&thread_pool[i], nullptr, thread_func, nullptr);
    }

    int servSocket;
    struct sockaddr_in clientAddr{};

    // Create socket for incoming connections
    if ((servSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        std::cerr << "socket() failed\n";
        exit(1);
    }

    struct sockaddr_in serverAddr{};
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(port_number);

    if (bind(servSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "bind() failed\n";
        exit(1);
    }

    if (listen(servSocket, MAX_CONNECTIONS) == 0) {
        printf("Listening...\n");
    } else {
        std::cerr << "listen() failed\n";
        exit(1);
    }

    socklen_t clientAddr_size = sizeof(clientAddr);

    // Client socket is connected to a client now !!
    while (true) {
        int clientSocket = accept(servSocket, (struct sockaddr *) &clientAddr, &clientAddr_size);
        if (clientSocket < 0) {
            std::cerr << "accept() failed\n";
            exit(1);
        }

        char clientName[INET_ADDRSTRLEN];
        if (inet_ntop(AF_INET, &clientAddr.sin_addr.s_addr, clientName, sizeof(clientName)) != nullptr)
            std::cout << "Handling client " << clientName << ntohs(clientAddr.sin_port) << '\n';
        else
            std::cerr << "Unable to get client address";

        int *pclient_socket = static_cast<int *>(malloc(sizeof(int)));
        *pclient_socket = clientSocket;
        pthread_mutex_lock(&lock);
        clients.push(pclient_socket);
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&lock);
    }
    // Not reachable
}
