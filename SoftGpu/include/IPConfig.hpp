/**
 * @file
 *
 * Copyright (c) 2025. Grafika Strahlen LLC
 * All rights reserved.
 */
#pragma once

#include <NumTypes.hpp>
#include <TUMaths.hpp>

namespace IPConfig {

static inline constexpr u32 MEMORY_BUS_BIT_WIDTH = 128;
static inline constexpr u32 MEMORY_BUS_BIT_WIDTH_EXPONENT = log2i(MEMORY_BUS_BIT_WIDTH);
static inline constexpr u32 PHYSICAL_ADDRESS_BITS = 48;

static inline constexpr bool ASSERT_ON_STD_LOGIC_CONFLICT = true;

}
