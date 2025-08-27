/**
 * @file
 *
 * Copyright (c) 2025. Grafika Strahlen LLC
 * All rights reserved.
 */
#pragma once

#if 0
#include <vulkan/vulkan.h>

#define VD_FORCE_MAX_VULKAN_VERSION VK_API_VERSION_1_0
#define VD_FORCE_DISABLE_VK_KHR_get_physical_device_properties2 (1)
#endif

// #define VD_FORCE_MAX_VULKAN_VERSION VK_API_VERSION_1_3

#ifndef VD_FORCE_DISABLE_VK_KHR_get_physical_device_properties2
  #define VD_FORCE_DISABLE_VK_KHR_get_physical_device_properties2 (0)
#endif
