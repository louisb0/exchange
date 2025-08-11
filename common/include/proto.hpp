#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>

namespace proto {

namespace ouch {
constexpr uint8_t TOKEN_LENGTH = 14;

enum message_type : char {
    ENTER = 'O',
    ACCEPTED = 'A',
};

struct __attribute__((packed)) enter_order {
    char message_type{ message_type::ENTER };
    char order_token[TOKEN_LENGTH]{}; // NOLINT(*-c-arrays)
    uint32_t order_book_id{};
    char side{};
    uint64_t quantity{};
    int32_t price{};
};

struct __attribute__((packed)) order_accepted {
    char message_type{ message_type::ACCEPTED };
    uint64_t timestamp{};
    char order_token[TOKEN_LENGTH]{}; // NOLINT(*-c-arrays)
    uint32_t order_book_id{};
    char side{};
    uint64_t order_id{};
    uint64_t quantity{};
    int32_t price{};
};

constexpr size_t MAX_MESSAGE_SIZE = std::max(sizeof(enter_order), sizeof(order_accepted));

inline message_type get_type(std::byte first) {
    char c = static_cast<char>(first);
    assert(c == ENTER || c == ACCEPTED);
    return static_cast<message_type>(c);
}

inline size_t get_size(message_type type) {
    switch (type) {
    case message_type::ENTER:
        return sizeof(enter_order);
    case message_type::ACCEPTED:
        return sizeof(order_accepted);
    default:
        assert(false); // TODO: Custom unreachable.
        return 0;
    }
}
} // namespace ouch

namespace scratch {
struct __attribute__((packed)) header {
    uint64_t client_id;
};

template <typename T> struct __attribute__((packed)) message {
    struct header header;
    T ouch;
};

using enter_order = message<ouch::enter_order>;
using order_accepted = message<ouch::order_accepted>;

constexpr size_t MAX_MESSAGE_SIZE = std::max(sizeof(enter_order), sizeof(order_accepted));
} // namespace scratch

} // namespace proto
