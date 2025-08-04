#include "common.hpp"

#include <arpa/inet.h>
#include <array>
#include <cassert>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <iostream>
#include <iterator>
#include <netinet/in.h>
#include <string_view>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

// NOTE: This needs to be revisited for fairness down the line. epoll treats all file descriptors
// equally, and it is not 'fair' that the responses from the engine to clients (via multicast_fd)
// are of equal priority as incoming requests from other clients.
//  - Consider a separate thread for multicast data / client responses?
//  - Use epoll only for established client connections, process multicast data on each iteration?
//
// TODO:
//  - build client, send data back to client in gateway
//
//  - Spec out internal protocol
//  - Partial reads and client state in gateway
//  - Implement basic matching logic
//  - Implement MDS
//  - Reliability (gateway drops, ME goes down, etc.)
//
//  - **specifying 127.0.0.1?**
// addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // More portable than inet_pton for localhost
//
//
// fuzzing
// simulation testing

int connect_engine() {
    int engine_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (engine_fd == -1) {
        perror("connect_engine socket()");
        return -1;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(config::ENGINE_PORT);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    if (connect(engine_fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) == -1) {
        perror("connect_engine connect()");
        close(engine_fd);
        return -1;
    }

    return engine_fd;
}

// TODO: Test INADDR_ANY where possible, avoiding inet_pton() calls.
int register_multicast(int epoll_fd) {
    int multicast_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (multicast_fd == -1) {
        perror("register_multicast socket()");
        return -1;
    }

    // Set non-blocking.
    int flags = fcntl(multicast_fd, F_GETFL, 0);
    if (flags == -1) {
        perror("register_multicast fcntl(F_GETFL)");
        close(multicast_fd);
        return -1;
    }
    if (fcntl(multicast_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("register_multicast fcntl(F_SETFL)");
        close(multicast_fd);
        return -1;
    }

    // Bind to multicast port.
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(config::ENGINE_MULTICAST_PORT);
    if (inet_pton(AF_INET, config::ENGINE_MULTICAST_ADDR, &addr.sin_addr) <= 0) {
        perror("register_multicast inet_pton(sin_addr)");
        close(multicast_fd);
        return -1;
    }

    int reuse = 1;
    if (setsockopt(multicast_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == -1) {
        perror("register_multicast setsockopt(SO_REUSEADDR)");
        close(multicast_fd);
        return -1;
    }

    if (bind(multicast_fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) == -1) {
        perror("register_multicast bind()");
        close(multicast_fd);
        return -1;
    }

    // Register to multicast.
    struct ip_mreq mreq{};
    mreq.imr_interface.s_addr = htonl(INADDR_LOOPBACK);
    if (inet_pton(AF_INET, config::ENGINE_MULTICAST_ADDR, &mreq.imr_multiaddr) <= 0) {
        perror("register_multicast inet_pton(multiaddr)");
        close(multicast_fd);
        return -1;
    }
    if (setsockopt(multicast_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) == -1) {
        perror("register_multicast setsockopt()");
        close(multicast_fd);
        return -1;
    }

    // Register to epoll.
    epoll_event ev{.events = EPOLLIN | EPOLLET, .data = {.fd = multicast_fd}};
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, multicast_fd, &ev) == -1) {
        perror("register_multicast epoll_ctl()");
        close(multicast_fd);
        return -1;
    }

    return multicast_fd;
}

int create_listener(int epoll_fd) {
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd == -1) {
        perror("create_listener socket()");
        return -1;
    }

    // Set non-blocking.
    int flags = fcntl(listen_fd, F_GETFL, 0);
    if (flags == -1) {
        perror("create_listener fcntl(F_GETFL)");
        close(listen_fd);
        return -1;
    }
    if (fcntl(listen_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("create_listener fcntl(F_SETFL)");
        close(listen_fd);
        return -1;
    }

    // Bind to port.
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(config::GATEWAY_PORT);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    int reuse = 1;
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        perror("create_listener setsockopt()");
        close(listen_fd);
        return -1;
    }

    if (bind(listen_fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0) {
        perror("create_listener bind()");
        close(listen_fd);
        return -1;
    }

    // Register to epoll.
    epoll_event ev{.events = EPOLLIN | EPOLLET, .data = {.fd = listen_fd}};
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &ev) == -1) {
        perror("create_listener epoll_ctl()");
        close(listen_fd);
        return -1;
    }

    // Set listening.
    if (listen(listen_fd, -1) == -1) {
        perror("create_listener listen()");
        close(listen_fd);
        return -1;
    }

    return listen_fd;
}

int main() {
    int engine_fd = connect_engine();
    if (engine_fd == -1) {
        exit(EXIT_FAILURE);
    }
    std::cout << "[gateway] Connected to engine.\n";

    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("epoll_create1()");
        exit(EXIT_FAILURE);
    }

    int multicast_fd = register_multicast(epoll_fd);
    if (multicast_fd < 0) {
        exit(EXIT_FAILURE);
    }
    std::cout << "[gateway] Registered to multicast.\n";

    int listen_fd = create_listener(epoll_fd);
    if (listen_fd < 0) {
        exit(EXIT_FAILURE);
    }
    std::cout << "[gateway] Listening...\n";

    constexpr uint16_t MAX_EVENTS = 64;
    std::array<epoll_event, MAX_EVENTS> events{};
    while (true) {
        int nfds = epoll_wait(epoll_fd, events.data(), MAX_EVENTS, -1);
        if (nfds == -1) {
            if (errno == EINTR) {
                continue;
            }

            perror("epoll_wait()");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < nfds; i++) {
            int fd = events[i].data.fd;

            if (fd == listen_fd) {
                sockaddr_in client_addr{};
                socklen_t client_addrlen{};

                while (true) {
                    // Accept connection.
                    int client_fd = accept(listen_fd, reinterpret_cast<sockaddr *>(&client_addr),
                                           &client_addrlen);
                    if (client_fd == -1) {
                        if (errno == EWOULDBLOCK || errno == EAGAIN) {
                            break;
                        }

                        perror("accept()");
                        continue;
                    }

                    // Set non-blocking.
                    int flags = fcntl(client_fd, F_GETFL, 0);
                    if (flags == -1) {
                        perror("fcntl(F_GETFL)");
                        close(client_fd);
                        continue;
                    }
                    if (fcntl(client_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
                        perror("fcntl(F_SETFL)");
                        close(client_fd);
                        continue;
                    }

                    // Register to epoll.
                    epoll_event ev{.events = EPOLLIN | EPOLLET, .data = {.fd = client_fd}};
                    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev) == -1) {
                        perror("epoll_ctl()");
                        close(client_fd);
                        continue;
                    }

                    std::cout << "[gateway] Accepted client.\n";
                }
            } else if (fd == multicast_fd) {
                ouch::order_accepted_message msg{};
                char *buf = reinterpret_cast<char *>(&msg);
                ssize_t bytes = recv(multicast_fd, buf, sizeof(msg), 0);
                assert(bytes > 0 && "only exact messages multicasted");

                std::cout << "[gateway] ME accepted order_token="
                          << std::string_view(std::data(msg.order_token), ouch::TOKEN_LENGTH)
                          << ".\n";
            } else {
                ouch::enter_order_message msg{};
                char *buf = reinterpret_cast<char *>(&msg);

                size_t read = 0;
                while (read != sizeof(msg)) {
                    ssize_t bytes = recv(fd, buf + read, sizeof(msg) - read, 0);
                    assert(bytes > 0 && "no partial messages");
                    read += bytes;
                }
                std::cout << "[gateway] Received order_token="
                          << std::string_view(std::data(msg.order_token), ouch::TOKEN_LENGTH)
                          << ".\n";

                if (send(engine_fd, &msg, sizeof(msg), 0) == -1) {
                    perror("send()");
                    continue;
                }
                std::cout << "[gateway] Sent order_token="
                          << std::string_view(std::data(msg.order_token), ouch::TOKEN_LENGTH)
                          << ".\n";
            }
        }
    }
}
