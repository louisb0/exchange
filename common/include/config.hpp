#pragma once

#include "include/proto.hpp"

#include <cstddef>
#include <cstdint>

namespace config {
namespace gateway {
    static inline constexpr uint16_t PORT = 3000;

    static inline constexpr uint32_t MAX_ACCEPTS_PER_LOOP = 4;
    static inline constexpr uint32_t MAX_CLIENTS_PER_LOOP = 16;
    static inline constexpr uint32_t MAX_MESSAGES_PER_CLIENT = 8;
    static inline constexpr uint32_t MAX_ENGINE_DATAGRAMS_PER_LOOP = 64;

    inline constexpr uint32_t MAX_MESSAGES_PER_LOOP =
        MAX_CLIENTS_PER_LOOP * MAX_MESSAGES_PER_CLIENT;

    static inline constexpr uint32_t CLIENT_RECV_MAX_BYTES =
        MAX_MESSAGES_PER_CLIENT * proto::ouch::MAX_MESSAGE_SIZE;
    static inline constexpr uint32_t LOOP_RECV_MAX_BYTES =
        MAX_MESSAGES_PER_LOOP * proto::ouch::MAX_MESSAGE_SIZE;
    static inline constexpr uint32_t ENGINE_SEND_MAX_BYTES =
        MAX_MESSAGES_PER_LOOP * proto::scratch::MAX_MESSAGE_SIZE;
} // namespace gateway

namespace engine {
    static inline constexpr uint16_t PORT = 3001;

    static inline constexpr uint32_t GATEWAY_RECV_MAX_BYTES = gateway::ENGINE_SEND_MAX_BYTES;
} // namespace engine

namespace multicast {
    static inline constexpr uint16_t PORT = 3002;
    static inline constexpr const char *ADDRESS = "239.1.1.1";
} // namespace multicast

static_assert(gateway::PORT != engine::PORT && engine::PORT != multicast::PORT);
static_assert(gateway::ENGINE_SEND_MAX_BYTES == engine::GATEWAY_RECV_MAX_BYTES);

} // namespace config
