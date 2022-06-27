#include "LoadStore.hpp"
#include "StreamingMultiprocessor.hpp"

void LoadStore::Execute(const LoadStoreInstruction instructionInfo) noexcept
{
    // Compute the 64 bit base address
    const u32 baseAddressLow = m_SM->GetRegister(instructionInfo.DispatchUnit, instructionInfo.BaseRegister);
    const u32 baseAddressHigh = m_SM->GetRegister(instructionInfo.DispatchUnit, instructionInfo.BaseRegister + 1u);

    u64 address = (static_cast<u64>(baseAddressHigh) << 32) | baseAddressLow;

    // If the exponent is not 111 then account for the indexing register.
    if(instructionInfo.IndexExponent != 7u)
    {
        const u32 indexValue = m_SM->GetRegister(instructionInfo.DispatchUnit, instructionInfo.IndexRegister);
        address += static_cast<u64>(indexValue) * (1u << static_cast<u32>(instructionInfo.IndexExponent));
    }

    // Add the offset
    address += instructionInfo.Offset;

    // Here we would get a lock on the cache from the other load store units.
    //   We'll use something similar to the staggered buffer so that once all the address info is calculated
    // we will perform the loads and stores from Ld/St unit 0 -> 3. We can also add a flag that indicates
    // all units are performing Loads, in which case locking isn't necessary, though we'd need multiple comm
    // lines to access the cache.

    {
        // With 8 registers we can at most be in two cache lines.
        // If the last register is in another cache line we'll prefetch it. This has no effect in software, but would have a substantial effect in hardware.

        const u64 maxAddress = address + instructionInfo.RegisterCount + 1u;

        // The lower 3 bits are just the index into the cache line.
        const u64 addressSetIndex = address >> 3;
        const u64 maxAddressSetIndex = maxAddress >> 3;
        if(addressSetIndex != maxAddressSetIndex)
        {
            m_SM->Prefetch(maxAddress);
        }
    }

    // If 1 then write.
    if(instructionInfo.ReadWrite)
    {
        // Iterate through each register that is being written.
        for(u32 i = 0; i < instructionInfo.RegisterCount + 1u; ++i)
        {
            const u32 value = m_SM->GetRegister(instructionInfo.DispatchUnit, instructionInfo.TargetRegister + i);
            m_SM->Write(address + i, value);
        }
    }
    else // Else Read
    {
        // Iterate through each register that is being read.
        for(u32 i = 0; i < instructionInfo.RegisterCount + 1u; ++i)
        {
            const u32 value = m_SM->Read(address + i);
            m_SM->SetRegister(instructionInfo.DispatchUnit, instructionInfo.TargetRegister + i, value);
        }
    }
}