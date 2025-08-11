#pragma once

#include "include/proto.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <unordered_map>

struct client {
    uint64_t id{};
    int fd{};

    std::array<std::byte, proto::ouch::MAX_MESSAGE_SIZE> partial{};
    size_t partial_len{};
};

class client_manager {
    uint64_t rolling_id_{};

    int epoll_fd_;
    int listener_fd_;

    std::unordered_map<uint64_t, client> clients_;

public:
    client_manager(int epoll_fd, int listener_fd)
        : epoll_fd_(epoll_fd), listener_fd_(listener_fd) {}

    void accept_clients();

    client &get_client(uint64_t client_id);
    void remove_client(uint64_t client_id);
};
