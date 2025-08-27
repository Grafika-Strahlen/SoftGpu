/**
 * @file
 *
 * Copyright (c) 2025. Grafika Strahlen LLC
 * All rights reserved.
 */
#pragma once

#include <VBox/msi.h>

#include <VBox/com/VirtualBox.h>

DECLCALLBACK(VBOXSTRICTRC) SoftGpuConfigRead(PPDMDEVINS deviceInstance, PPDMPCIDEV pPciDev, uint32_t uAddress, unsigned cb, uint32_t* pu32Value) noexcept;
DECLCALLBACK(VBOXSTRICTRC) SoftGpuConfigWrite(PPDMDEVINS deviceInstance, PPDMPCIDEV pPciDev, uint32_t uAddress, unsigned cb, uint32_t u32Value) noexcept;
