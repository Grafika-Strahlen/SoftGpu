#include "LoadStore.hpp"
#include "StreamingMultiprocessor.hpp"

void LoadStore::PipelineReadBaseRegister() noexcept
{
    RegisterFile::CommandPacket packetHigh;
    packetHigh.Command = RegisterFile::ECommand::ReadRegister;
    packetHigh.Successful = &m_SuccessfulHigh;
    packetHigh.Unsuccessful = &m_UnsuccessfulHigh;

    RegisterFile::CommandPacket packetLow;
    packetLow.Command = RegisterFile::ECommand::ReadRegister;
    packetLow.Successful = &m_SuccessfulLow;
    packetLow.Unsuccessful = &m_UnsuccessfulLow;

    // Does the base register start at high?
    if(m_Instruction.BaseRegister & 0x1)
    {
        packetHigh.TargetRegister = (m_Instruction.BaseRegister + 1u) >> 1u;
        packetLow.TargetRegister = (m_Instruction.BaseRegister + 1u) >> 1u;

        packetHigh.Value = &m_BaseAddressLow;
        packetLow.Value = &m_BaseAddressHigh;
    }
    // The base register starts at low
    else
    {
        packetHigh.TargetRegister = m_Instruction.BaseRegister >> 1u;
        packetLow.TargetRegister = m_Instruction.BaseRegister >> 1u;

        packetHigh.Value = &m_BaseAddressHigh;
        packetLow.Value = &m_BaseAddressLow;
    }

    m_SM->InvokeRegisterFileHigh(m_UnitIndex, packetHigh);
    m_SM->InvokeRegisterFileLow(m_UnitIndex, packetLow);
}

void LoadStore::PipelineReleaseReadLockBaseRegister() noexcept
{
    if((!m_SuccessfulHigh && !m_UnsuccessfulHigh) || (!m_SuccessfulLow && !m_UnsuccessfulLow))
    {
        // Unit hasn't run yet, this can't actually happen in software.
        --m_ExecutionStage;
    }

    // If we failed to read, just go back a stage.
    if(!m_UnsuccessfulHigh || !m_UnsuccessfulLow)
    {
        --m_ExecutionStage;
    }

    RegisterFile::CommandPacket packetHigh;
    packetHigh.Command = RegisterFile::ECommand::Unlock;
    packetHigh.Value = nullptr;
    packetHigh.Successful = &m_SuccessfulHigh;
    packetHigh.Unsuccessful = &m_UnsuccessfulHigh;

    RegisterFile::CommandPacket packetLow;
    packetLow.Command = RegisterFile::ECommand::Unlock;
    packetLow.Value = nullptr;
    packetLow.Successful = &m_SuccessfulHigh;
    packetLow.Unsuccessful = &m_UnsuccessfulHigh;

    // Does the base register start at high?
    if(m_Instruction.BaseRegister & 0x1)
    {
        packetHigh.TargetRegister = (m_Instruction.BaseRegister + 1u) >> 1u;
        packetLow.TargetRegister = (m_Instruction.BaseRegister + 1u) >> 1u;
    }
    // The base register starts at low
    else
    {
        packetHigh.TargetRegister = m_Instruction.BaseRegister >> 1u;
        packetLow.TargetRegister = m_Instruction.BaseRegister >> 1u;
    }

    m_SM->InvokeRegisterFileHigh(m_UnitIndex, packetHigh);
    m_SM->InvokeRegisterFileLow(m_UnitIndex, packetLow);
    
}

// void LoadStore::Pipeline0() noexcept
// {
//     RegisterFile::CommandPacket packet;
//     packet.Command = RegisterFile::ECommand::ReadRegister;
//     packet.TargetRegister = m_Instruction.BaseRegister;
//     packet.Value = &m_BaseAddressLow;
//     packet.Successful = &m_SuccessfulHigh;
//     packet.Unsuccessful = &m_UnsuccessfulHigh;
//
//     m_SM->InvokeRegisterFile(m_UnitIndex, packet);
// }
//
// void LoadStore::Pipeline1() noexcept
// {
//     if(!m_SuccessfulHigh && !m_UnsuccessfulHigh)
//     {
//         // Unit hasn't run yet, this can't actually happen in software.
//         --m_ExecutionStage;
//     }
//
//     // If we failed to read, just go back a stage.
//     if(!m_UnsuccessfulHigh)
//     {
//         --m_ExecutionStage;
//     }
//
//     RegisterFile::CommandPacket packet;
//     packet.Command = RegisterFile::ECommand::Unlock;
//     packet.TargetRegister = m_Instruction.BaseRegister;
//     packet.Value = nullptr;
//     packet.Successful = &m_SuccessfulHigh;
//     packet.Unsuccessful = &m_UnsuccessfulHigh;
//
//     m_SM->InvokeRegisterFile(m_UnitIndex, packet);
// }
//
// void LoadStore::Pipeline2() noexcept
// {
//     if(!m_SuccessfulHigh && !m_UnsuccessfulHigh)
//     {
//         // Unit hasn't run yet, this can't actually happen in software.
//         --m_ExecutionStage;
//     }
//
//     // If we failed to read, just go back a stage.
//     if(!m_UnsuccessfulHigh)
//     {
//         --m_ExecutionStage;
//     }
//
//     RegisterFile::CommandPacket packet;
//     packet.Command = RegisterFile::ECommand::ReadRegister;
//     packet.TargetRegister = m_Instruction.BaseRegister + 1u;
//     packet.Value = &m_BaseAddressHigh;
//     packet.Successful = &m_SuccessfulHigh;
//     packet.Unsuccessful = &m_UnsuccessfulHigh;
//
//     m_SM->InvokeRegisterFile(m_UnitIndex, packet);
// }
//
// void LoadStore::Pipeline3() noexcept
// {
//     if(!m_SuccessfulHigh && !m_UnsuccessfulHigh)
//     {
//         // Unit hasn't run yet, this can't actually happen in software.
//         --m_ExecutionStage;
//     }
//
//     // If we failed to read, just go back a stage.
//     if(!m_UnsuccessfulHigh)
//     {
//         --m_ExecutionStage;
//     }
//
//     RegisterFile::CommandPacket packet;
//     packet.Command = RegisterFile::ECommand::Unlock;
//     packet.TargetRegister = m_Instruction.BaseRegister + 1u;
//     packet.Value = nullptr;
//     packet.Successful = &m_SuccessfulHigh;
//     packet.Unsuccessful = &m_UnsuccessfulHigh;
//
//     m_SM->InvokeRegisterFile(m_UnitIndex, packet);
// }

void LoadStore::Pipeline4() noexcept
{
    if(!m_SuccessfulHigh && !m_UnsuccessfulHigh)
    {
        // Unit hasn't run yet, this can't actually happen in software.
        --m_ExecutionStage;
    }

    // If we failed to read, just go back a stage.
    if(!m_UnsuccessfulHigh)
    {
        --m_ExecutionStage;
    }

    RegisterFile::CommandPacket resetPacket {};
    resetPacket.Command = RegisterFile::ECommand::Reset;
    resetPacket.TargetRegister = 0;
    resetPacket.Value = nullptr;
    resetPacket.Successful = nullptr;
    resetPacket.Unsuccessful = nullptr;

    // If the exponent is 111 then ignore the indexing register.
    if(m_Instruction.IndexExponent == 7u)
    {
        m_SM->InvokeRegisterFileHigh(m_UnitIndex, resetPacket);
        m_SM->InvokeRegisterFileLow(m_UnitIndex, resetPacket);
        return;
    }

    RegisterFile::CommandPacket packet {};
    packet.Command = RegisterFile::ECommand::ReadRegister;
    packet.TargetRegister = m_Instruction.IndexRegister;
    packet.Value = &m_IndexRegister;
    packet.Successful = &m_SuccessfulHigh;
    packet.Unsuccessful = &m_UnsuccessfulHigh;

    if(m_Instruction.IndexRegister & 0x1)
    {
        m_SM->InvokeRegisterFileHigh(m_UnitIndex, packet);
        m_SM->InvokeRegisterFileLow(m_UnitIndex, resetPacket);
    }
    else
    {
        m_SM->InvokeRegisterFileLow(m_UnitIndex, packet);
        m_SM->InvokeRegisterFileHigh(m_UnitIndex, resetPacket);
    }
}

void LoadStore::Pipeline5() noexcept
{
    if(!m_SuccessfulHigh && !m_UnsuccessfulHigh)
    {
        // Unit hasn't run yet, this can't actually happen in software.
        --m_ExecutionStage;
    }

    // If we failed to read, just go back a stage.
    if(!m_UnsuccessfulHigh)
    {
        --m_ExecutionStage;
    }

    RegisterFile::CommandPacket resetPacket {};
    resetPacket.Command = RegisterFile::ECommand::Reset;
    resetPacket.TargetRegister = 0;
    resetPacket.Value = nullptr;
    resetPacket.Successful = nullptr;
    resetPacket.Unsuccessful = nullptr;

    // If the exponent is 111 then ignore the indexing register.
    if(m_Instruction.IndexExponent == 7u)
    {
        m_SM->InvokeRegisterFileHigh(m_UnitIndex, resetPacket);
        m_SM->InvokeRegisterFileLow(m_UnitIndex, resetPacket);
        return;
    }

    m_Address += static_cast<u64>(m_IndexRegister) * (1u << static_cast<u32>(m_Instruction.IndexExponent));

    RegisterFile::CommandPacket packet {};
    packet.Command = RegisterFile::ECommand::Unlock;
    packet.TargetRegister = m_Instruction.IndexRegister;
    packet.Value = nullptr;
    packet.Successful = &m_SuccessfulHigh;
    packet.Unsuccessful = &m_UnsuccessfulHigh;

    if(m_Instruction.IndexRegister & 0x1)
    {
        m_SM->InvokeRegisterFileHigh(m_UnitIndex, packet);
        m_SM->InvokeRegisterFileLow(m_UnitIndex, resetPacket);
    }
    else
    {
        m_SM->InvokeRegisterFileLow(m_UnitIndex, packet);
        m_SM->InvokeRegisterFileHigh(m_UnitIndex, resetPacket);
    }
}

void LoadStore::Pipeline6() noexcept
{
    if(!m_SuccessfulHigh && !m_UnsuccessfulHigh)
    {
        // Unit hasn't run yet, this can't actually happen in software.
        --m_ExecutionStage;
    }

    // If we failed to read, just go back a stage.
    if(!m_UnsuccessfulHigh)
    {
        --m_ExecutionStage;
    }

    RegisterFile::CommandPacket resetPacket {};
    resetPacket.Command = RegisterFile::ECommand::Reset;
    resetPacket.TargetRegister = 0;
    resetPacket.Value = nullptr;
    resetPacket.Successful = nullptr;
    resetPacket.Unsuccessful = nullptr;

    m_SM->InvokeRegisterFileHigh(m_UnitIndex, resetPacket);
    m_SM->InvokeRegisterFileLow(m_UnitIndex, resetPacket);

    // Being able to read up to 8 registers we can be in at most be in two cache lines.
    // If the last register is in another cache line we'll prefetch it. This has no effect in software, but would have a substantial effect in hardware.

    const u64 maxAddress = m_Address + m_Instruction.RegisterCount;

    // The lower 3 bits are just the index into the cache line.
    const u64 addressSetIndex = m_Address >> 3;
    const u64 maxAddressSetIndex = maxAddress >> 3;
    if(addressSetIndex != maxAddressSetIndex)
    {
        m_SM->Prefetch(maxAddress);
    }
}

void LoadStore::Pipeline23() noexcept
{
    m_SM->ReportLdStReady(m_UnitIndex);
}

void LoadStore::PipelineRWHandler(const u32 index) noexcept
{
    if(!m_SuccessfulHigh && !m_UnsuccessfulHigh)
    {
        // Unit hasn't run yet, this can't actually happen in software.
        --m_ExecutionStage;
    }

    // If we failed to read, just go back a stage.
    if(!m_UnsuccessfulHigh)
    {
        --m_ExecutionStage;
    }

    RegisterFile::CommandPacket resetPacket {};
    resetPacket.Command = RegisterFile::ECommand::Reset;
    resetPacket.TargetRegister = 0;
    resetPacket.Value = nullptr;
    resetPacket.Successful = nullptr;
    resetPacket.Unsuccessful = nullptr;

    if(m_Instruction.RegisterCount < index)
    {
        m_SM->InvokeRegisterFileHigh(m_UnitIndex, resetPacket);
        m_SM->InvokeRegisterFileLow(m_UnitIndex, resetPacket);
        return;
    }

    RegisterFile::CommandPacket packet {};
    packet.TargetRegister = m_CurrentRegister;
    packet.Value = &m_TargetValue;
    packet.Successful = &m_SuccessfulHigh;
    packet.Unsuccessful = &m_UnsuccessfulHigh;

    if(m_Instruction.ReadWrite)
    {
        packet.Command = RegisterFile::ECommand::WriteRegister;
    }
    else
    {
        // This needs to be changed to handle pipelining.
        m_TargetValue = m_SM->Read(m_Address);
        packet.Command = RegisterFile::ECommand::ReadRegister;
    }

    if(m_Instruction.IndexRegister & 0x1)
    {
        m_SM->InvokeRegisterFileHigh(m_UnitIndex, packet);
        m_SM->InvokeRegisterFileLow(m_UnitIndex, resetPacket);
    }
    else
    {
        m_SM->InvokeRegisterFileLow(m_UnitIndex, packet);
        m_SM->InvokeRegisterFileHigh(m_UnitIndex, resetPacket);
    }
}

void LoadStore::PipelineReleaseHandler(const u32 index) noexcept
{
    if(!m_SuccessfulHigh && !m_UnsuccessfulHigh)
    {
        // Unit hasn't run yet, this can't actually happen in software.
        --m_ExecutionStage;
    }

    // If we failed to read, just go back a stage.
    if(!m_UnsuccessfulHigh)
    {
        --m_ExecutionStage;
    }

    RegisterFile::CommandPacket resetPacket {};
    resetPacket.Command = RegisterFile::ECommand::Reset;
    resetPacket.TargetRegister = 0;
    resetPacket.Value = nullptr;
    resetPacket.Successful = nullptr;
    resetPacket.Unsuccessful = nullptr;

    if(m_Instruction.RegisterCount < index)
    {
        m_SM->InvokeRegisterFileHigh(m_UnitIndex, resetPacket);
        m_SM->InvokeRegisterFileLow(m_UnitIndex, resetPacket);
        return;
    }

    RegisterFile::CommandPacket packet {};
    packet.Command = RegisterFile::ECommand::Unlock;
    packet.TargetRegister = m_CurrentRegister;
    packet.Value = &m_TargetValue;
    packet.Successful = &m_SuccessfulHigh;
    packet.Unsuccessful = &m_UnsuccessfulHigh;

    // This needs to be changed to handle pipelining.
    m_SM->Write(m_Address, m_TargetValue);

    ++m_Address;
    ++m_CurrentRegister;

    if(m_Instruction.IndexRegister & 0x1)
    {
        m_SM->InvokeRegisterFileHigh(m_UnitIndex, packet);
        m_SM->InvokeRegisterFileLow(m_UnitIndex, resetPacket);
    }
    else
    {
        m_SM->InvokeRegisterFileLow(m_UnitIndex, packet);
        m_SM->InvokeRegisterFileHigh(m_UnitIndex, resetPacket);
    }
}

// void LoadStore::Execute(const LoadStoreInstruction instructionInfo) noexcept
// {
//     // Compute the 64 bit base address
//     const u32 baseAddressLow = m_SM->GetRegister(instructionInfo.BaseRegister);
//     const u32 baseAddressHigh = m_SM->GetRegister(instructionInfo.BaseRegister + 1u);
//
//     m_SM->ReleaseRegisterContestation(instructionInfo.BaseRegister);
//     m_SM->ReleaseRegisterContestation(instructionInfo.BaseRegister + 1u);
//
//     u64 address = (static_cast<u64>(baseAddressHigh) << 32) | baseAddressLow;
//
//     // If the exponent is not 111 then account for the indexing register.
//     if(instructionInfo.IndexExponent != 7u)
//     {
//         const u32 indexValue = m_SM->GetRegister(instructionInfo.IndexRegister);
//         m_SM->ReleaseRegisterContestation(instructionInfo.IndexRegister);
//         address += static_cast<u64>(indexValue) * (1u << static_cast<u32>(instructionInfo.IndexExponent));
//     }
//
//     // Add the offset
//     address += instructionInfo.Offset;
//
//     // Here we would get a lock on the cache from the other load store units.
//     //   We'll use something similar to the staggered buffer so that once all the address info is calculated
//     // we will perform the loads and stores from Ld/St unit 0 -> 3. We can also add a flag that indicates
//     // all units are performing Loads, in which case locking isn't necessary, though we'd need multiple comm
//     // lines to access the cache.
//
//     {
//         // With 8 registers we can at most be in two cache lines.
//         // If the last register is in another cache line we'll prefetch it. This has no effect in software, but would have a substantial effect in hardware.
//
//         const u64 maxAddress = address + instructionInfo.RegisterCount;
//
//         // The lower 3 bits are just the index into the cache line.
//         const u64 addressSetIndex = address >> 3;
//         const u64 maxAddressSetIndex = maxAddress >> 3;
//         if(addressSetIndex != maxAddressSetIndex)
//         {
//             m_SM->Prefetch(maxAddress);
//         }
//     }
//
//     m_DispatchPort = instructionInfo.DispatchUnit;
//     m_RegisterCount = instructionInfo.RegisterCount;
//     m_StartRegister = instructionInfo.TargetRegister;
//     m_ExecutionStage = 5;
//
//     // If 1 then write.
//     if(instructionInfo.ReadWrite)
//     {
//         // Iterate through each register that is being written.
//         for(u32 i = 0; i < instructionInfo.RegisterCount + 1u; ++i)
//         {
//             const u32 value = m_SM->GetRegister(instructionInfo.TargetRegister + i);
//             m_SM->Write(address + i, value);
//         }
//     }
//     else // Else Read
//     {
//         // Iterate through each register that is being read.
//         for(u32 i = 0; i < instructionInfo.RegisterCount + 1u; ++i)
//         {
//             const u32 value = m_SM->Read(address + i);
//             m_SM->SetRegister(instructionInfo.TargetRegister + i, value);
//         }
//     }
// }
