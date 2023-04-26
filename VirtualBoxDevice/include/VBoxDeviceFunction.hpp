#pragma once

#include <VBox/vmm/pdmdev.h>
#include <iprt/assert.h>

#include "Processor.hpp"

/**
 * Playground device per function (sub-device) data.
 */
struct SoftGpuDeviceFunction final
{
    /** The function number. */
    uint8_t         iFun;
    /** Device function name. */
    char            szName[31];
    /** MMIO region \#0 name. */
    char            szMmio0[32];
    /** MMIO region \#1 name. */
    char            szMmio1[32];
    /** The MMIO region \#0 handle. */
    IOMMMIOHANDLE   hMmio0;
    /** The MMIO region \#1 handle. */
    IOMMMIOHANDLE   hMmio1;
    /** Backing storage. */
    uint8_t         abBacking[4096];
    PDMIBASE IBase;
    PDMIDISPLAYPORT IDisplayPort;
    R3PTRTYPE(PPDMIBASE) pDrvBase;
    Processor processor;
};

/**
 * Playground device instance data.
 */
struct SoftGpuDevice final
{
    /** PCI device functions. */
    SoftGpuDeviceFunction pciFunction;
};

static inline constexpr u32 SOFT_GPU_SSM_VERSION_1 = 1;
static inline constexpr u32 SOFT_GPU_SSM_VERSION = SOFT_GPU_SSM_VERSION_1;
