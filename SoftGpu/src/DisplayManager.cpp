#include "DisplayManager.hpp"
#include "Processor.hpp"
#include "PCIControlRegisters.hpp"

void DisplayManager::HandleVSyncEvent() noexcept
{
    // Just some legacy code.
    u32 m_VSyncEvent = m_VSyncEvents[0];

    if(m_VSyncEvent > 0)
    {
        const u32 vsyncDisplay = m_VSyncEvent - 1;
        m_VSyncEvent = 0;

        if(m_Displays[vsyncDisplay].VSyncEnable)
        {
            m_Parent->SetInterrupt(PciControlRegisters::MSG_INTERRUPT_VSYNC_DISPLAY_0 + vsyncDisplay);
        }
    }
}
