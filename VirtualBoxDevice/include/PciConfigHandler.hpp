#pragma once

#include <VBox/msi.h>

#include <VBox/com/VirtualBox.h>

DECLCALLBACK(VBOXSTRICTRC) SoftGpuConfigRead(PPDMDEVINS deviceInstance, PPDMPCIDEV pPciDev, uint32_t uAddress, unsigned cb, uint32_t* pu32Value);
DECLCALLBACK(VBOXSTRICTRC) SoftGpuConfigWrite(PPDMDEVINS deviceInstance, PPDMPCIDEV pPciDev, uint32_t uAddress, unsigned cb, uint32_t u32Value);
