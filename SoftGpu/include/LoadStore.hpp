#pragma once

#include <Objects.hpp>
#include <NumTypes.hpp>

class StreamingMultiprocessor;

// Computes the address in the form of [BaseRegister + IndexRegister * 2**IndexExponent + Offset]
struct LoadStoreInstruction final
{
    u32 DispatchUnit : 1; // Which Dispatch Port invoked this.
    u32 ReplicationIndex : 2; // For pixels we replicate 4 times, we need to know which replication we are for the register file view.
    u32 ReadWrite : 1; // Loading = 0, Storing = 1
    u32 IndexExponent : 3; // Index multiplier can be either 1, 2, 4, 8, 16, 32, 64, or 0. 111 disables indexing, every other value is equal to 2**xxx
    u32 RegisterCount : 3; // Indicates how many registers in a sequence are being Loaded/Stored. This uses 1 based index. This is enough to store a full vec4d.
    u32 Pad0 : 6; // Pad for x86 alignment.
    u32 BaseRegister : 8; // The base register to address to, this gives a 256 register window. This points to a sequence of 2 registers.
    u32 IndexRegister : 8; // The index register to address to, this gives a 256 register window. This will be ignored if IndexExponent is 111
    u32 TargetRegister : 8; // The target register to Load or Store.
    u32 Pad1 : 24; // Pad for x86 alignment.
    i16 Offset; // A signed offset from the base register and index.
};

class LoadStore final
{
    DEFAULT_DESTRUCT(LoadStore);
    DELETE_CM(LoadStore);
public:
    LoadStore(StreamingMultiprocessor* const sm, const u32 unitIndex) noexcept
        : m_SM(sm)
        , m_UnitIndex(unitIndex)
    { }

    void Execute(LoadStoreInstruction instructionInfo) noexcept;
private:
    StreamingMultiprocessor* m_SM;
    u32 m_UnitIndex;
};
