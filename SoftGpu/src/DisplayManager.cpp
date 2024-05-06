#include "DisplayManager.hpp"
#include "Processor.hpp"
#include "PCIControlRegisters.hpp"

void DisplayManager::HandleVSyncEvent() noexcept
{
    if(m_VSyncEvent > 0)
    {
        const u32 vsyncDisplay = m_VSyncEvent - 1;
        m_VSyncEvent = 0;

        if(m_Displays[vsyncDisplay].VSyncEnable)
        {
            m_Processor->SetInterrupt(PciControlRegisters::MSG_INTERRUPT_VSYNC_DISPLAY_0 + vsyncDisplay);
        }
    }
}
