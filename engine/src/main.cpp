#include "include/net_config.hpp"
#include "include/proto.hpp"

#include "network.hpp"

#include <spdlog/spdlog.h>

#include <arpa/inet.h>
#include <array>
#include <cassert>
#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <netinet/in.h>
#include <span>
#include <sys/socket.h>
#include <sys/types.h>
#include <vector>

// TODO: Figure out configuration.
constexpr size_t BUFFER_SIZE = 64 * proto::scratch::MAX_MESSAGE_SIZE;

int main() {
    sockaddr_in mcast_addr{};
    mcast_addr.sin_family = AF_INET;
    mcast_addr.sin_port = htons(net_config::ENGINE_MULTICAST_PORT);
    if (inet_pton(AF_INET, net_config::ENGINE_MULTICAST_ADDRESS, &mcast_addr.sin_addr) <= 0) {
        perror("inet_pton()");
        exit(EXIT_FAILURE);
    }

    int mcast_fd = network::create_mcast_pub();
    if (mcast_fd == -1) {
        exit(EXIT_FAILURE);
    }

    spdlog::info("[main] awaiting gateway...");
    int gateway_fd = network::await_gateway_connection();
    if (gateway_fd == -1) {
        exit(EXIT_FAILURE);
    }
    spdlog::info("[main] ready");

    std::vector<std::byte> output;
    output.reserve(sizeof(proto::scratch::MAX_MESSAGE_SIZE));

    std::array<std::byte, BUFFER_SIZE> input{};
    size_t received = 0;

    while (true) {
        assert(!input.data() || received < (sizeof(proto::ouch::enter_order)));

        // Read pending data.
        ssize_t bytes = recv(gateway_fd, input.data() + received, BUFFER_SIZE - received, 0);
        if (bytes == -1) {
            spdlog::critical("[main] recv: {}", std::strerror(errno));
            exit(EXIT_FAILURE);
        }
        if (bytes == 0) {
            spdlog::info("[main] gateway disconnected - shutting down");
            exit(EXIT_SUCCESS);
        }
        received += bytes;

        // Parse messages from gateway.
        std::span<const std::byte> parseable{ input.data(), received };
        size_t parsed = 0;

        while (!parseable.empty()) {
            constexpr size_t ouch_offset = sizeof(proto::scratch::header);
            if (ouch_offset > parseable.size()) {
                break;
            }

            proto::ouch::message_type msg_type = proto::ouch::get_type(parseable[ouch_offset]);
            size_t msg_size = proto::ouch::get_size(msg_type);

            size_t frame_size = sizeof(proto::scratch::header) + msg_size;
            if (frame_size > parseable.size()) {
                break;
            }

            // Process the message.
            const auto *msg =
                reinterpret_cast<const proto::ouch::enter_order *>(parseable.data() + ouch_offset);

            proto::ouch::order_accepted res{
                .message_type = proto::ouch::ACCEPTED,
                .timestamp = 0,
                .order_token = {},
                .order_book_id = msg->order_book_id,
                .side = msg->side,
                .order_id = 0,
                .quantity = msg->quantity,
                .price = msg->price,
            };
            std::memcpy(&res.order_token, msg->order_token, proto::ouch::TOKEN_LENGTH);
            spdlog::info("[main] received order_accepted(token={:.{}s})", res.order_token,
                         proto::ouch::TOKEN_LENGTH);

            const std::byte *header_ptr = parseable.data();
            output.insert(output.end(), header_ptr, header_ptr + sizeof(proto::scratch::header));

            const auto *res_ptr = reinterpret_cast<const std::byte *>(&res);
            output.insert(output.end(), res_ptr, res_ptr + sizeof(res));

            if (sendto(mcast_fd, output.data(), output.size(), 0,
                       reinterpret_cast<const sockaddr *>(&mcast_addr), sizeof(mcast_addr)) == -1) {
                spdlog::critical("[main] sendto: {}", std::strerror(errno));
                exit(EXIT_FAILURE);
            }
            spdlog::info("[main] multicasted {} bytes", output.size());
            output.clear();

            // Advance the window.
            parsed += frame_size;
            parseable = parseable.subspan(frame_size);
        }

        // Handle remaining partial message.
        size_t remaining = received - parsed;
        if (remaining > 0) {
            std::memcpy(input.data(), input.data() + parsed, remaining);
        }
        received = remaining;
    }
}
