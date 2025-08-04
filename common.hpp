#pragma once

#include <cstdint>
#include <netinet/in.h>

namespace config {
static constexpr uint16_t GATEWAY_PORT = 3000;
static constexpr uint16_t ENGINE_PORT = 3001;
static constexpr uint16_t ENGINE_MULTICAST_PORT = 3002;
static constexpr const char *ENGINE_MULTICAST_ADDR = "239.1.1.1";
} // namespace config

namespace ouch {
constexpr uint8_t TOKEN_LENGTH = 14;

struct __attribute__((packed)) enter_order_message {
    char message_type;
    char order_token[TOKEN_LENGTH]; // NOLINT(*-c-arrays)
    uint32_t order_book_id;
    char side;
    uint64_t quantity;
    int32_t price;
};

struct __attribute__((packed)) order_accepted_message {
    char message_type;
    uint64_t timestamp;
    char order_token[TOKEN_LENGTH]; // NOLINT(*-c-arrays)
    uint32_t order_book_id;
    char side;
    uint64_t order_id;
    uint64_t quantity;
    int32_t price;
};
} // namespace ouch
