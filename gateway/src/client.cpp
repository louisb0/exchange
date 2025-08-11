#include "client.hpp"

#include <spdlog/spdlog.h>

#include <cerrno>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <utility>

void client_manager::accept_clients() {
    sockaddr_in addr{};
    socklen_t addrlen = sizeof(addr);

    while (true) {
        int fd = accept(listener_fd_, reinterpret_cast<sockaddr *>(&addr), &addrlen);
        if (fd == -1) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                break;
            }

            spdlog::error("[client_manager::accept_clients] accept: {}", std::strerror(errno));
            continue;
        }

        // Set non-blocking
        int flags = fcntl(fd, F_GETFL, 0);
        if (flags == -1) {
            spdlog::error("[client_manager::accept_clients] fcntl(F_GETFL): {}",
                          std::strerror(errno));
            close(fd);
            continue;
        }

        if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
            spdlog::error("[client_manager::accept_clients] fcntl(F_SETFL): {}",
                          std::strerror(errno));
            close(fd);
            continue;
        }

        uint64_t client_id = rolling_id_++;

        // Register client.
        epoll_event ev{ .events = EPOLLIN, .data = { .u64 = client_id } };
        if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &ev) == -1) {
            spdlog::error("[client_manager::accept_clients] epoll_ctl(): {}", std::strerror(errno));
            close(fd);
            continue;
        }

        clients_.emplace(client_id, client{ .id = client_id, .fd = fd });
        spdlog::info("[client_manager::accept_clients] client(id={}) connected", client_id);
    }
}

client &client_manager::get_client(uint64_t client_id) { return clients_[client_id]; }

void client_manager::remove_client(uint64_t client_id) {
    auto it = clients_.find(client_id);
    close(it->second.fd);
    clients_.erase(it);

    spdlog::info("[client_manager::remove_client] client(id={}) disconnected", client_id);
}
