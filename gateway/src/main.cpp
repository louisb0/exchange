#include "include/proto.hpp"

#include "client.hpp"
#include "config.hpp"
#include "network.hpp"

#include <spdlog/spdlog.h>

#include <array>
#include <cassert>
#include <cerrno>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <span>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>

int main() {
    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        exit(EXIT_FAILURE);
    }

    int mcast_fd = network::sub_mcast();
    if (mcast_fd == -1) {
        exit(EXIT_FAILURE);
    }

    spdlog::info("[main] awaiting engine...");
    int engine_fd = network::await_connect_engine();
    if (engine_fd == -1) {
        exit(EXIT_FAILURE);
    }

    int listener_fd = network::listen();
    if (listener_fd == -1) {
        exit(EXIT_FAILURE);
    }
    spdlog::info("[main] ready");

    client_manager cmgr(epoll_fd, listener_fd);

    std::array<epoll_event, config::MAX_EVENTS> events{};
    while (true) {
        int nfds = epoll_wait(epoll_fd, events.data(), config::MAX_EVENTS, 1);
        if (nfds == -1) {
            assert(errno == EINTR);
            continue;
        }

        // Accept client data.
        for (const auto &ev : std::span(events.data(), nfds)) {
            client &client = cmgr.get_client(ev.data.u64);

            std::array<std::byte, config::IN_CLIENT_BUFFER_SIZE> input{};
            std::array<std::byte, config::OUT_ENGINE_BUFFER_SIZE> output{};

            // Receive messages from client.
            size_t received = client.partial_len;
            if (received > 0) {
                std::memcpy(input.data(), client.partial.data(), received);
            }

            while (received != config::IN_CLIENT_BUFFER_SIZE) {
                ssize_t bytes = recv(client.fd, input.data() + received,
                                     config::IN_CLIENT_BUFFER_SIZE - received, 0);
                if (bytes == -1) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        break;
                    }

                    spdlog::critical("[main] client(id={}) recv: {}", ev.data.u64,
                                     std::strerror(errno));
                    exit(EXIT_FAILURE);
                }

                if (bytes == 0) {
                    cmgr.remove_client(client.id);
                    break;
                }

                received += bytes;
            }

            // Parse messages from client.
            std::span<const std::byte> parseable{ input.data(), received };
            size_t parsed = 0;

            while (!parseable.empty()) {
                proto::ouch::message_type msg_type = proto::ouch::get_type(parseable.front());
                size_t msg_size = proto::ouch::get_size(msg_type);

                if (msg_size > parseable.size()) {
                    client.partial_len = parseable.size();
                    std::memcpy(client.partial.data(), parseable.data(), parseable.size());
                    break;
                }

                // TODO: Find a more suitable place / design for handling partials. This appears
                // random and disconnected.
                client.partial_len = 0;

                size_t frame_size = sizeof(proto::scratch::header) + msg_size;
                assert(parsed + frame_size <= output.size());

                proto::scratch::header header{ .client_id = client.id };
                std::memcpy(output.data() + parsed, &header, sizeof(header));
                std::memcpy(output.data() + parsed + sizeof(header), parseable.data(), msg_size);

                parsed += frame_size;
                parseable = parseable.subspan(msg_size);
            }

            // Forward parsed messages to engine.
            if (parsed == 0) {
                continue;
            }

            if (send(engine_fd, output.data(), parsed, 0) == -1) {
                spdlog::critical("[main] client(id={}) to engine send: {}", client.id,
                                 std::strerror(errno));
                exit(EXIT_FAILURE);
            }

            // spdlog::info("[main] client(id={}) forwarded {} bytes", client.id, parsed);
        }

        // Reply to clients.
        std::array<std::byte, config::IN_ENGINE_BUFFER_SIZE> replies{};
        ssize_t bytes = recv(mcast_fd, replies.data(), replies.size(), 0);
        if (bytes == -1) {
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                spdlog::critical("[main] mcast recv: ", std::strerror(errno));
                exit(EXIT_FAILURE);
            }
        }

        if (bytes > 0) {
            spdlog::info("{} bytes", bytes);
            std::span parseable{ replies.data(), static_cast<size_t>(bytes) };
            while (!parseable.empty()) {
                constexpr size_t ouch_offset = sizeof(proto::scratch::header);

                proto::ouch::message_type msg_type = proto::ouch::get_type(parseable[ouch_offset]);
                size_t msg_size = proto::ouch::get_size(msg_type);

                auto *header = reinterpret_cast<proto::scratch::header *>(parseable.data());

                client &client = cmgr.get_client(header->client_id);
                if (send(client.fd, parseable.data() + ouch_offset, msg_size, 0) == -1) {
                    spdlog::critical("[main] engine to client(id={}) send: {}", client.id,
                                     std::strerror(errno));
                    exit(EXIT_FAILURE);
                }

                spdlog::info("[main] replied to client(id={})", client.id);
                parseable = parseable.subspan(ouch_offset + msg_size);
            }
        }

        // TODO: Arguable whether or not this should be part of client_manager - should it just be a
        // wrapper around some ADT?
        cmgr.accept_clients();
    }
}
