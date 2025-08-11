#include "include/net_config.hpp"

#include <gtest/gtest.h>

#include <string>

TEST(EngineTest, Test) { EXPECT_EQ(net_config::ENGINE_PORT, 3001); }
