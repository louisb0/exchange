#include "network.hpp"

#include "include/net_config.hpp"

#include <spdlog/spdlog.h>

#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

int network::await_connect_engine() {
    int engine_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (engine_fd == -1) {
        spdlog::critical("[network::await_connect_engine] socket: {}", std::strerror(errno));
        return -1;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(net_config::ENGINE_PORT);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    while (connect(engine_fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) == -1) {
        if (errno != ECONNREFUSED) {
            spdlog::critical("[network::await_connect_engine] connect: {}", std::strerror(errno));
            close(engine_fd);
            return -1;
        }

        sleep(1);
    }

    return engine_fd;
}

int network::sub_mcast() {
    int mcast_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (mcast_fd == -1) {
        spdlog::critical("[network::sub_mcast] socket: {}", std::strerror(errno));
        return -1;
    }

    // Set non-blocking.
    int flags = fcntl(mcast_fd, F_GETFL, 0);
    if (flags == -1) {
        spdlog::critical("[network::sub_mcast] fcntl(F_GETFL): {}", std::strerror(errno));
        close(mcast_fd);
        return -1;
    }

    if (fcntl(mcast_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        spdlog::critical("[network::sub_mcast] fcntl(F_SETFL): {}", std::strerror(errno));
        close(mcast_fd);
        return -1;
    }

    // Bind to multicast address.
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(net_config::ENGINE_MULTICAST_PORT);
    if (inet_pton(AF_INET, net_config::ENGINE_MULTICAST_ADDRESS, &addr.sin_addr) <= 0) {
        spdlog::critical("[network::sub_mcast] inet_pton(addr): {}", std::strerror(errno));
        close(mcast_fd);
        return -1;
    }

    int reuse = 1;
    if (setsockopt(mcast_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == -1) {
        spdlog::critical("[network::sub_mcast] setsockopt(SO_REUSEADDR): {}", std::strerror(errno));
        close(mcast_fd);
        return -1;
    }
    if (setsockopt(mcast_fd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse)) == -1) {
        spdlog::critical("[network::sub_mcast] setsockopt(SO_REUSEPORT): {}", std::strerror(errno));
        close(mcast_fd);
        return -1;
    }

    if (bind(mcast_fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) == -1) {
        spdlog::critical("[network::sub_mcast] bind: {}", std::strerror(errno));
        close(mcast_fd);
        return -1;
    }

    // Increase buffer size.
    // TODO: Config sync.
    int buffer_size = 8 * 1024 * 1024; // NOLINT(*-magic-numbers)
    if (setsockopt(mcast_fd, SOL_SOCKET, SO_RCVBUF, &buffer_size, sizeof(buffer_size)) == -1) {
        spdlog::critical("[network::sub_mcast] setsockopt(SO_RCVBUF): {}", std::strerror(errno));
        return -1;
    }

    // Subscribe to multicast.
    ip_mreq mreq{};
    mreq.imr_interface.s_addr = htonl(INADDR_LOOPBACK);
    if (inet_pton(AF_INET, net_config::ENGINE_MULTICAST_ADDRESS, &mreq.imr_multiaddr) <= 0) {
        spdlog::critical("[network::sub_mcast] inet_pton(mreq): {}", std::strerror(errno));
        close(mcast_fd);
        return -1;
    }

    if (setsockopt(mcast_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) == -1) {
        spdlog::critical("[network::sub_mcast] setsockopt(IP_ADD_MEMBERSHIP): {}",
                         std::strerror(errno));
        close(mcast_fd);
        return -1;
    }

    return mcast_fd;
}

int network::listen() {
    int listener_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listener_fd == -1) {
        spdlog::critical("[network::listen] socket: {}", std::strerror(errno));
        return -1;
    }

    // Set non-blocking.
    int flags = fcntl(listener_fd, F_GETFL, 0);
    if (flags == -1) {
        spdlog::critical("[network::listen] fcntl(F_GETFL): {}", std::strerror(errno));
        close(listener_fd);
        return -1;
    }

    if (fcntl(listener_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        spdlog::critical("[network::listen] fcntl(F_SETFL): {}", std::strerror(errno));
        close(listener_fd);
        return -1;
    }

    // Bind to port.
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(net_config::GATEWAY_PORT);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    int reuse = 1;
    if (setsockopt(listener_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        spdlog::critical("[network::listen] setsockopt(): {}", std::strerror(errno));
        close(listener_fd);
        return -1;
    }

    if (bind(listener_fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0) {
        spdlog::critical("[network::listen] bind(): {}", std::strerror(errno));
        close(listener_fd);
        return -1;
    }

    // Set listening.
    if (::listen(listener_fd, -1) == -1) {
        spdlog::critical("[network::listen] listen(): {}", std::strerror(errno));
        close(listener_fd);
        return -1;
    }

    return listener_fd;
}
