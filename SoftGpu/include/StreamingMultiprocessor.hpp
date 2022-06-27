#pragma once

#include "RegisterFile.hpp"
#include "LoadStore.hpp"

class Processor;

class StreamingMultiprocessor final
{
    DEFAULT_DESTRUCT(StreamingMultiprocessor);
    DELETE_CM(StreamingMultiprocessor);
public:
    StreamingMultiprocessor(Processor* const processor, const u32 smIndex) noexcept
        : m_Processor(processor)
        , m_RegisterFile{ }
        , m_LdSt { { this, 0 }, { this, 1 }, { this, 2 }, { this, 3 } }
        , m_DispatchPorts { 0, 0 }
        , m_SMIndex(smIndex)
    { }

    void Clock() noexcept
    {
        
    }

    [[nodiscard]] u32 Read(u64 address) noexcept;
    void Write(u64 address, u32 value) noexcept;
    void Prefetch(u64 address) noexcept;

    [[nodiscard]] u32 GetRegister(const u32 dispatchPort, const u32 targetRegister) const noexcept
    {
        return m_RegisterFile.GetRegister(m_DispatchPorts[dispatchPort], targetRegister);
    }

    void SetRegister(const u32 dispatchPort, const u32 targetRegister, const u32 value) noexcept
    {
        m_RegisterFile.SetRegister(m_DispatchPorts[dispatchPort], targetRegister, value);
    }

    void ReportCoreReady(const u32 coreIndex) noexcept
    {
        
    }
private:
    Processor* m_Processor;
    RegisterFile m_RegisterFile;
    LoadStore m_LdSt[4];
    u32 m_DispatchPorts[2];
    u32 m_SMIndex;
};
