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
    uint8_t         FunctionId;
    /** Device function name. */
    char            FunctionName[31];
    /** MMIO region BAR0 name. */
    char            MmioBar0Name[32];
    /** MMIO region BAR1 name. */
    char            MmioBar1Name[32];
    /** MMIO region Expansion ROM name. */
    char            MmioExpansionRomName[32];
    /** The MMIO region BAR0 handle. */
    IOMMMIOHANDLE   hMmioBar0;
    /** The MMIO region BAR1 handle. */
    IOMMMIOHANDLE   hMmioBar1;
    /** The MMIO region Expansion ROM handle. */
    IOMMMIOHANDLE   hMmioExpansionRom;
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
