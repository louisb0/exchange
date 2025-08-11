#include "include/proto.hpp"

#include "network.hpp"

#include "spdlog/spdlog.h"
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <span>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

static const proto::ouch::enter_order msg{
    .order_token = "LOAD_TEST",
};

// NOLINTBEGIN(*-magic-numbers)
int main() {
    size_t n_clients{};
    size_t n_messages{};
    size_t n_packet_hint{};
    bool paritals{};

    std::cout << "n_clients: ";
    std::cin >> n_clients;

    std::cout << "n_messages: ";
    std::cin >> n_messages;

    std::cout << "n_packet_hint: ";
    std::cin >> n_packet_hint;

    std::cout << "with_partials (0 or 1): ";
    std::cin >> paritals;

    if (n_messages % n_packet_hint != 0) {
        spdlog::critical("n_messages must be divisible by n_packet_hint");
        exit(EXIT_FAILURE);
    }

    std::vector<int> fds;
    if (network::connect_clients(n_clients, fds) == -1) {
        exit(EXIT_FAILURE);
    }

    std::vector<std::byte> buffer(n_messages * sizeof(msg));
    for (size_t i = 0; i < n_messages; ++i) {
        std::memcpy(buffer.data() + (i * sizeof(msg)), &msg, sizeof(msg));
    }

    // Send phase.
    size_t sends = n_messages / n_packet_hint;
    size_t bytes_per_send = buffer.size() / sends;
    if (paritals) {
        bytes_per_send -= 1;
    }

    std::span to_send{ buffer };

    for (size_t i = 0; i < sends; i++) {
        for (int fd : fds) {
            if (send(fd, to_send.first(bytes_per_send).data(), bytes_per_send, 0) == -1) {
                spdlog::critical("[main] send-0: {}", std::strerror(errno));
                exit(EXIT_FAILURE);
            }
        }

        to_send = to_send.subspan(bytes_per_send);
        usleep(50000);
    }

    if (paritals) {
        assert(to_send.size() == sends);
        for (int fd : fds) {
            if (send(fd, to_send.data(), to_send.size(), 0) == -1) {
                spdlog::critical("[main] send-1: {}", std::strerror(errno));
                exit(EXIT_FAILURE);
            }
        }
    }

    // Receive phase.
    size_t expected = n_clients * n_messages * sizeof(proto::ouch::order_accepted);
    size_t received = 0;

    auto last_log_time = std::chrono::steady_clock::now();
    const auto log_interval = std::chrono::seconds(2);

    while (expected != received) {
        for (int fd : fds) {
            ssize_t bytes = recv(fd, buffer.data(), buffer.size(), MSG_DONTWAIT);
            if (bytes == -1) {
                if (errno == EWOULDBLOCK || errno == EAGAIN) {
                    continue;
                }

                spdlog::critical("[main] recv: {}", std::strerror(errno));
                exit(EXIT_FAILURE);
            }

            received += bytes;
        }

        auto now = std::chrono::steady_clock::now();
        if (now - last_log_time >= log_interval) {
            spdlog::info("[main] recv={} exp={}", received, expected);
            last_log_time = now;
        }
    }
}
// NOLINTEND(*-magic-numbers)
