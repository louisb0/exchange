#include "include/net_config.hpp"

#include <gtest/gtest.h>

#include <string>

TEST(GatewayTest, Test) { EXPECT_EQ(net_config::GATEWAY_PORT, 3000); }
