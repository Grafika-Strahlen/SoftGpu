#pragma once

#include <Objects.hpp>
#include <NumTypes.hpp>

class StreamingMultiprocessor;

// Computes the address in the form of [BaseRegister + IndexRegister * 2**IndexExponent + Offset]
struct LoadStoreInstruction final
{
    u32 Reserved : 2; // 2 reserved bits for alignment in x86, these can be removed in hardware.
    u32 DispatchUnit : 1; // Which Dispatch Port invoked this.
    u32 ReadWrite : 1; // Reading = 0, Writing = 1
    u32 IndexExponent : 3; // Index multiplier can be either 1, 2, 4, 8, 16, 32, 64, or 0. 111 disables indexing, every other value is equal to 2**xxx
    u32 RegisterCount : 3; // Indicates how many registers in a sequence are being Loaded/Stored. This uses 1 based index. This is enough to store a full vec4d.
    u32 BaseRegister : 8; // The base register to address to, this gives a 256 register window. This points to a sequence of 2 registers.
    u32 IndexRegister : 8; // The index register to address to, this gives a 256 register window. This will be ignored if IndexExponent is 111
    u32 TargetRegister : 8; // The target register to Load or Store.
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
