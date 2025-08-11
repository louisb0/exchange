#include "network.hpp"

#include "include/net_config.hpp"

#include <spdlog/spdlog.h>

#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

int network::create_mcast_pub() {
    int mcast_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (mcast_fd == -1) {
        spdlog::critical("[network::create_mcast_pub] socket: {}", std::strerror(errno));
        return -1;
    }

    in_addr interface{ .s_addr = htonl(INADDR_LOOPBACK) };
    if (setsockopt(mcast_fd, IPPROTO_IP, IP_MULTICAST_IF, &interface, sizeof(interface)) < 0) {
        spdlog::critical("[network::create_mcast_pub] setsockopt IP_MULTICAST_IF: {}",
                         std::strerror(errno));
        return -1;
    }

    int buffer_size = 8 * 1024 * 1024; // NOLINT(*-magic-numbers)
    if (setsockopt(mcast_fd, SOL_SOCKET, SO_RCVBUF, &buffer_size, sizeof(buffer_size)) == -1) {
        spdlog::critical("[network::sub_mcast] setsockopt(SO_RCVBUF): {}", std::strerror(errno));
        return -1;
    }

    return mcast_fd;
}

int network::await_gateway_connection() {
    int listener_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listener_fd == -1) {
        spdlog::critical("[network::await_gateway_connection] socket: {}", std::strerror(errno));
        return -1;
    }

    // Bind to port.
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(net_config::ENGINE_PORT);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    int reuse = 1;
    if (setsockopt(listener_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        spdlog::critical("[network::await_gateway_connection] setsockopt(SO_REUSEADDR): {}",
                         std::strerror(errno));
        close(listener_fd);
        return -1;
    }

    if (bind(listener_fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0) {
        spdlog::critical("[network::await_gateway_connection] bind: {}", std::strerror(errno));
        close(listener_fd);
        return -1;
    }

    // Listen for gateway connection.
    if (listen(listener_fd, -1) < 0) {
        spdlog::critical("[network::await_gateway_connection] listen: {}", std::strerror(errno));
        close(listener_fd);
        return -1;
    }

    sockaddr_in gateway_addr{};
    socklen_t gateway_addrlen = sizeof(gateway_addr);
    int gateway_fd =
        accept(listener_fd, reinterpret_cast<sockaddr *>(&gateway_addr), &gateway_addrlen);
    if (gateway_fd < 0) {
        spdlog::critical("[network::await_gateway_connection] accept: {}", std::strerror(errno));
        close(listener_fd);
        return -1;
    }

    close(listener_fd);
    return gateway_fd;
}
