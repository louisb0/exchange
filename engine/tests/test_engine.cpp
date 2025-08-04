#include "config.hpp"

#include <gtest/gtest.h>

#include <string>

TEST(EngineTest, Test) { EXPECT_EQ(config::ENGINE_PORT, 3001); }
