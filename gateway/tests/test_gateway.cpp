#include "config.hpp"

#include <gtest/gtest.h>

#include <string>

TEST(GatewayTest, Test) { EXPECT_EQ(config::GATEWAY_PORT, 3000); }
