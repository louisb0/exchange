#pragma once

#include "include/proto.hpp"

#include <cstdint>

namespace config {

static constexpr uint16_t MAX_ACCEPTS = 4;
static constexpr uint16_t MAX_MESSAGES_PER_CLIENT = 8;
static constexpr uint16_t MAX_EVENTS = 16;
static constexpr uint16_t MAX_REPLIES = 64;

static constexpr size_t IN_CLIENT_BUFFER_SIZE =
    MAX_MESSAGES_PER_CLIENT * proto::ouch::MAX_MESSAGE_SIZE;

static constexpr size_t OUT_ENGINE_BUFFER_SIZE =
    MAX_MESSAGES_PER_CLIENT * proto::scratch::MAX_MESSAGE_SIZE;

static constexpr size_t IN_ENGINE_BUFFER_SIZE = MAX_REPLIES * proto::scratch::MAX_MESSAGE_SIZE;

} // namespace config
