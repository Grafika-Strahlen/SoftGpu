#pragma once

#include <Objects.hpp>
#include "FPU.hpp"

class StreamingMultiprocessor;

// This polymorphism would map to the FPU and FPU/ALU simply receiving data lines for which they can perform register calls.
class ICore
{
    DEFAULT_CONSTRUCT_PO(ICore);
    DEFAULT_DESTRUCT_VI(ICore);
    DELETE_CM(ICore);
public:
    [[nodiscard]] virtual u32 GetRegister(u32 dispatchPort, u32 targetRegister) const noexcept = 0;
    virtual void SetRegister(u32 dispatchPort, u32 targetRegister, u32 value) noexcept = 0;

    virtual void ReportReady() const noexcept = 0;
    [[nodiscard]] virtual bool IsReady() const noexcept = 0;
};

class FpCore final : public ICore
{
    DEFAULT_DESTRUCT(FpCore);
    DELETE_CM(FpCore);
public:
    FpCore(StreamingMultiprocessor* const sm, const u32 coreIndex) noexcept
        : m_SM(sm)
        , m_CoreIndex(coreIndex)
        , m_Fpu(this)
    { }

    [[nodiscard]] u32 GetRegister(u32 dispatchPort, u32 targetRegister) const noexcept override;
    void SetRegister(u32 dispatchPort, u32 targetRegister, u32 value) noexcept override;

    void ReportReady() const noexcept override;

    [[nodiscard]] bool IsReady() const noexcept override
    {
        return m_Fpu.ReadyToExecute();
    }
private:
    StreamingMultiprocessor* m_SM;
    u32 m_CoreIndex;
    Fpu m_Fpu;
};
