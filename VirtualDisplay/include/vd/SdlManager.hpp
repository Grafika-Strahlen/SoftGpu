/**
 * @file
 *
 * Copyright (c) 2025. Grafika Strahlen LLC
 * All rights reserved.
 */
#pragma once

namespace tau::vd {

bool InitSdl() noexcept;

void CleanupSdl() noexcept;

void PollEvents() noexcept;

bool ShouldClose() noexcept;

}