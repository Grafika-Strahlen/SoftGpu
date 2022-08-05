#pragma once

#include <Objects.hpp>
#include <NumTypes.hpp>

class StreamingMultiprocessor;

// Computes the address in the form of [BaseRegister + IndexRegister * 2**IndexExponent + Offset]
struct LoadStoreInstruction final
{
    u32 DispatchUnit : 1; // Which Dispatch Port invoked this.
    u32 ReadWrite : 1; // Loading = 0, Storing = 1
    u32 IndexExponent : 3; // Index multiplier can be either 1, 2, 4, 8, 16, 32, 64, or 0. 111 disables indexing, every other value is equal to 2**xxx
    u32 RegisterCount : 3; // Indicates how many registers in a sequence are being Loaded/Stored. This uses 1 based index. This is enough to store a full vec4d.
    u32 Pad0 : 8; // Pad for x86 alignment.
    u32 BaseRegister : 12; // The base register to address to. This points to a sequence of 2 registers.
    u32 IndexRegister : 12; // The index register to address to. This will be ignored if IndexExponent is 111
    u32 TargetRegister : 12; // The target register to Load or Store.
    u32 Pad1 : 6; // Pad for x86 alignment.
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
        , m_ExecutionStage(0)
        , m_DispatchPort{ }
        , m_RegisterCount{ }
        , m_Pad{ }
        , m_StartRegister{ }
    { }

    void Reset()
    {
        m_ExecutionStage = 0;
        m_DispatchPort = { };
        m_RegisterCount = { };
        m_Pad = { };
        m_StartRegister = { };
    }

    void Clock() noexcept;

    void Execute(LoadStoreInstruction instructionInfo) noexcept;
private:
    StreamingMultiprocessor* m_SM;
    u32 m_UnitIndex;
    u32 m_ExecutionStage;
    u8 m_DispatchPort : 1;
    u8 m_RegisterCount : 3;
    u8 m_Pad : 4;
    u16 m_StartRegister;
};
