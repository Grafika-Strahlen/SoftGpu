#pragma once

#include <Objects.hpp>
#include <NumTypes.hpp>
#include <cstring>

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
    static inline constexpr u32 MAX_EXECUTION_STAGE = 24;
public:
    LoadStore(StreamingMultiprocessor* const sm, const u32 unitIndex) noexcept
        : m_SM(sm)
        , m_UnitIndex(unitIndex)
        , m_ExecutionStage(0)
        , m_Instruction{}
        , m_SuccessfulHigh(false)
        , m_UnsuccessfulHigh(false)
        , m_SuccessfulLow(false)
        , m_UnsuccessfulLow(false)
        , m_Address(0)
        , m_IndexRegister(0)
        , m_CurrentRegister(0)
    { }

    void Reset()
    {
        m_ExecutionStage = 0;
        (void) ::std::memset(&m_Instruction, 0, sizeof(m_Instruction));
        m_SuccessfulHigh = false;
        m_UnsuccessfulHigh = false;
        m_SuccessfulLow = false;
        m_UnsuccessfulLow = false;
        m_Address = 0;
        m_IndexRegister = 0;
        m_CurrentRegister = 0;
    }

    void Clock() noexcept
    {
        switch(m_ExecutionStage)
        {
            case 0: return;
            case  1: Pipeline23(); break;
            case  2: Pipeline22(); break;
            case  3: Pipeline21(); break;
            case  4: Pipeline20(); break;
            case  5: Pipeline19(); break;
            case  6: Pipeline18(); break;
            case  7: Pipeline17(); break;
            case  8: Pipeline16(); break;
            case  9: Pipeline15(); break;
            case 10: Pipeline14(); break;
            case 11: Pipeline13(); break;
            case 12: Pipeline12(); break;
            case 13: Pipeline11(); break;
            case 14: Pipeline10(); break;
            case 15: Pipeline9();  break;
            case 16: Pipeline8();  break;
            case 17: Pipeline7();  break;
            case 18: Pipeline6();  break;
            case 19: Pipeline5();  break;
            case 20: Pipeline4();  break;
            // case 21: Pipeline3();  break;
            // case 22: Pipeline2();  break;
            case 21: PipelineReleaseReadLockBaseRegister();  break;
            case 22: PipelineReadBaseRegister();  break;
            default: break;
        }

        --m_ExecutionStage;
    }

    void PrepareExecution(LoadStoreInstruction instructionInfo) noexcept
    {
        (void) ::std::memcpy(&m_Instruction, &instructionInfo, sizeof(instructionInfo));
        m_ExecutionStage = MAX_EXECUTION_STAGE;
    }

    // void Execute(LoadStoreInstruction instructionInfo) noexcept;
private:
    // Read high and low base register.
    void PipelineReadBaseRegister() noexcept;

    // Release the read locks on the base register.
    void PipelineReleaseReadLockBaseRegister() noexcept;

    // // Read base register low.
    // void Pipeline0() noexcept;

    // // Release read lock on base register low.
    // void Pipeline1() noexcept;

    // // Read base register high.
    // void Pipeline2() noexcept;

    // // Release read lock on base register high.
    // void Pipeline3() noexcept;

    // Read index register.
    void Pipeline4() noexcept;

    // Release read lock on index register.
    void Pipeline5() noexcept;

    // Prefetch memory.
    void Pipeline6() noexcept;

    // Handle register 0.
    void Pipeline7() noexcept
    {
        PipelineRWHandler(0);
    }

    // Release lock on register 0.
    void Pipeline8() noexcept
    {
        PipelineReleaseHandler(0);
    }
    
    // Handle register 1.
    void Pipeline9() noexcept
    {
        PipelineRWHandler(1);
    }

    // Release lock on register 1.
    void Pipeline10() noexcept
    {
        PipelineReleaseHandler(1);
    }

    // Handle register 2.
    void Pipeline11() noexcept
    {
        PipelineRWHandler(2);
    }

    // Release lock on register 2.
    void Pipeline12() noexcept
    {
        PipelineReleaseHandler(2);
    }

    // Handle register 3.
    void Pipeline13() noexcept
    {
        PipelineRWHandler(3);
    }

    // Release lock on register 3.
    void Pipeline14() noexcept
    {
        PipelineReleaseHandler(3);
    }

    // Handle register 4.
    void Pipeline15() noexcept
    {
        PipelineRWHandler(4);
    }

    // Release lock on register 4.
    void Pipeline16() noexcept
    {
        PipelineReleaseHandler(4);
    }

    // Handle register 5.
    void Pipeline17() noexcept
    {
        PipelineRWHandler(5);
    }

    // Release lock on register 5.
    void Pipeline18() noexcept
    {
        PipelineReleaseHandler(5);
    }

    // Handle register 6.
    void Pipeline19() noexcept
    {
        PipelineRWHandler(6);
    }

    // Release lock on register 6.
    void Pipeline20() noexcept
    {
        PipelineReleaseHandler(6);
    }

    // Handle register 6.
    void Pipeline21() noexcept
    {
        PipelineRWHandler(7);
    }

    // Release lock on register 6.
    void Pipeline22() noexcept
    {
        PipelineReleaseHandler(7);
    }

    // Report that this unit is ready.
    void Pipeline23() noexcept;

    void PipelineRWHandler(u32 index) noexcept;
    void PipelineReleaseHandler(u32 index) noexcept;
private:
    StreamingMultiprocessor* m_SM;
    u32 m_UnitIndex;

    u32 m_ExecutionStage;

    LoadStoreInstruction m_Instruction;

    bool m_SuccessfulHigh;
    bool m_UnsuccessfulHigh;
    bool m_SuccessfulLow;
    bool m_UnsuccessfulLow;

    union
    {
        struct
        {
            u32 m_BaseAddressLow;
            u32 m_BaseAddressHigh;
        };
        u64 m_Address;
    };

    union
    {
        u32 m_IndexRegister;
        u32 m_TargetValue;
    };

    u16 m_CurrentRegister;
};
