#include "common.hpp"

#include <arpa/inet.h>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <iterator>
#include <netinet/in.h>
#include <string_view>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

int setup_multicast() {
    int multicast_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (multicast_fd == -1) {
        perror("setup_multicast socket()");
        return -1;
    }

    in_addr interface{.s_addr = htonl(INADDR_LOOPBACK)};
    if (setsockopt(multicast_fd, IPPROTO_IP, IP_MULTICAST_IF, &interface, sizeof(interface)) < 0) {
        perror("setup_multicast setsockopt()");
        return -1;
    }

    return multicast_fd;
}

int await_connection() {
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd == -1) {
        perror("await_connection socket()");
        return -1;
    }

    // Bind to port.
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(config::ENGINE_PORT);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    int reuse = 1;
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        perror("await_connection setsockopt()");
        close(listen_fd);
        return -1;
    }

    if (bind(listen_fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0) {
        perror("await_connection bind()");
        close(listen_fd);
        return -1;
    }

    // Listen for gateway connection.
    if (listen(listen_fd, -1) < 0) {
        perror("await_connection listen()");
        close(listen_fd);
        return -1;
    }

    sockaddr_in gateway_addr{};
    socklen_t gateway_addrlen{};
    int gateway_fd =
        accept(listen_fd, reinterpret_cast<sockaddr *>(&gateway_addr), &gateway_addrlen);
    if (gateway_fd < 0) {
        perror("await_connection accept()");
        close(listen_fd);
        return -1;
    }

    close(listen_fd);
    return gateway_fd;
}

int main() {
    sockaddr_in multicast_addr{};
    multicast_addr.sin_family = AF_INET;
    multicast_addr.sin_port = htons(config::ENGINE_MULTICAST_PORT);
    if (inet_pton(AF_INET, config::ENGINE_MULTICAST_ADDR, &multicast_addr.sin_addr) <= 0) {
        perror("inet_pton()");
        return -1;
    }

    int multicast_fd = setup_multicast();
    if (multicast_fd == -1) {
        exit(EXIT_FAILURE);
    }
    std::cout << "[engine] Multicast established. Awaiting gateway...\n";

    int gateway_fd = await_connection();
    if (gateway_fd == -1) {
        exit(EXIT_FAILURE);
    }
    std::cout << "[engine] Connected to gateway. Listening...\n";

    while (true) {
        ouch::enter_order_message msg{};
        char *buf = reinterpret_cast<char *>(&msg);

        size_t read = 0;
        while (read != sizeof(msg)) {
            ssize_t bytes = recv(gateway_fd, buf + read, sizeof(msg) - read, 0);
            assert(bytes > 0 && "no partial messages");
            read += bytes;
        }
        std::cout << "[engine] Received order_token="
                  << std::string_view(std::data(msg.order_token), ouch::TOKEN_LENGTH) << ".\n";

        ouch::order_accepted_message res = {
            .message_type = 'A',
            .timestamp = 0,
            .order_token = "TO_BE_FILLED",
            .order_book_id = msg.order_book_id,
            .side = msg.side,
            .order_id = 0,
            .quantity = msg.quantity,
            .price = msg.price,
        };
        memcpy(std::data(res.order_token), std::data(msg.order_token), ouch::TOKEN_LENGTH);

        if (sendto(multicast_fd, &res, sizeof(res), 0,
                   reinterpret_cast<sockaddr *>(&multicast_addr), sizeof(multicast_addr)) == -1) {
            perror("sendto()");
            continue;
        }
        std::cout << "[engine] Multicasted order accepted order_token="
                  << std::string_view(std::data(msg.order_token), ouch::TOKEN_LENGTH) << ".\n";
    }
}
