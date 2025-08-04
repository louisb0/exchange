#pragma once

#include <cstdint>

namespace proto {

namespace ouch {
constexpr uint8_t TOKEN_LENGTH = 14;

struct __attribute__((packed)) enter_order {
    char message_type;
    char order_token[TOKEN_LENGTH]; // NOLINT(*-c-arrays)
    uint32_t order_book_id;
    char side;
    uint64_t quantity;
    int32_t price;
};

struct __attribute__((packed)) order_accepted {
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

namespace scratch {
namespace {
template <typename T> struct message {
    uint32_t client_id;
    T ouch;
};
} // namespace

using enter_order = message<ouch::enter_order>;
using order_accepted = message<ouch::order_accepted>;
} // namespace scratch

} // namespace proto
