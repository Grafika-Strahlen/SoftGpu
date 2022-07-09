#include "FPU.hpp"
#include "Core.hpp"

#include <immintrin.h>
#include <cstring>
#include <limits>
#include <cmath>

void Fpu::Clock() noexcept
{
    if(m_ExecutionStage == 0)
    {
        return;
    }
    else if(m_ExecutionStage == 1)
    {
        m_Core->ReportReady();
    }

    --m_ExecutionStage;
}

void Fpu::ExecuteInstruction(const LoadedFpuInstruction instructionInfo) noexcept
{
    m_DispatchPort = instructionInfo.DispatchPort;
    m_StorageRegister = instructionInfo.StorageRegister;
    
    if(instructionInfo.Precision == EPrecision::Single)
    {
        m_StorageRegisterCount = 1;

        if(instructionInfo.Operation == EFpuOp::NegateAbs)
        {
            const u32 result = NegateAbsF32(static_cast<u32>(instructionInfo.OperandA), instructionInfo.OperandB);
            m_Core->PrepareRegisterWrite(false, instructionInfo.StorageRegister, result);
            return;
        }

        f32 valueA;
        (void) ::std::memcpy(&valueA, &instructionInfo.OperandA, sizeof(valueA));

        f32 result;

        switch(instructionInfo.Operation)
        {
            case EFpuOp::BasicBinOp:
            case EFpuOp::Fma:
            case EFpuOp::Compare:
            {
                f32 valueB;
                (void) ::std::memcpy(&valueB, &instructionInfo.OperandB, sizeof(valueB));

                switch(instructionInfo.Operation)
                {
                    case EFpuOp::BasicBinOp:
                    {
                        result = BasicBinOpF32(valueA, valueB, static_cast<EBinOp>(instructionInfo.OperandC));
                        break;
                    }
                    case EFpuOp::Fma:
                    {
                        f32 valueC;
                        (void) ::std::memcpy(&valueC, &instructionInfo.OperandC, sizeof(valueC));

                        result = FmaF32(valueA, valueB, valueC);
                        break;
                    }
                    case EFpuOp::Compare:
                    {
                        const u32 compResult = CompareF32(valueA, valueB);
                        m_Core->PrepareRegisterWrite(false, instructionInfo.StorageRegister, compResult);
                        return;
                    }
                    default: break;
                }

                break;
            }
            case EFpuOp::Round:
                result = RoundF32(static_cast<ERoundingMode>(instructionInfo.OperandB), valueA);
                break;
            default:
                result = ::std::numeric_limits<f32>::quiet_NaN();
                break;
        }

        u32 resultU;
        (void) ::std::memcpy(&resultU, &result, sizeof(result));

        m_Core->PrepareRegisterWrite(false, instructionInfo.StorageRegister, resultU);
    }
    else if(instructionInfo.Precision == EPrecision::Half)
    {
        m_StorageRegisterCount = 1;

        if(instructionInfo.Operation == EFpuOp::NegateAbs)
        {
            const u32 result = NegateAbsF16(static_cast<u32>(instructionInfo.OperandA), instructionInfo.OperandB);
            m_Core->PrepareRegisterWrite(false, instructionInfo.StorageRegister, result);
            return;
        }

        const f32 valueA = _cvtsh_ss(static_cast<u16>(instructionInfo.OperandA));

        f32 result;

        switch(instructionInfo.Operation)
        {
            case EFpuOp::BasicBinOp:
            case EFpuOp::Fma:
            case EFpuOp::Compare:
            {
                const f32 valueB = _cvtsh_ss(static_cast<u16>(instructionInfo.OperandB));

                switch(instructionInfo.Operation)
                {
                    case EFpuOp::BasicBinOp:
                    {
                        result = BasicBinOpF32(valueA, valueB, static_cast<EBinOp>(instructionInfo.OperandC));
                        break;
                    }
                    case EFpuOp::Fma:
                    {
                        const f32 valueC = _cvtsh_ss(static_cast<u16>(instructionInfo.OperandC));

                        result = FmaF32(valueA, valueB, valueC);
                        break;
                    }
                    case EFpuOp::Compare:
                    {
                        const u32 compResult = CompareF32(valueA, valueB);
                        m_Core->PrepareRegisterWrite(false, instructionInfo.StorageRegister, compResult);
                        return;
                    }
                    default:
                        result = ::std::numeric_limits<f32>::quiet_NaN();
                        break;
                }

                break;
            }
            case EFpuOp::Round:
            {
                const u16 resultU = RoundF16(static_cast<ERoundingMode>(instructionInfo.OperandB), valueA);
                m_Core->PrepareRegisterWrite(false, instructionInfo.StorageRegister, resultU);
                return;
            }
            default:
                result = ::std::numeric_limits<f32>::quiet_NaN();
                break;
        }

        const u32 resultU = _cvtss_sh(result, _MM_FROUND_CUR_DIRECTION);
        m_Core->PrepareRegisterWrite(false, instructionInfo.StorageRegister, resultU);
    }
    else if(instructionInfo.Precision == EPrecision::Double)
    {
        m_StorageRegisterCount = 2;

        if(instructionInfo.Operation == EFpuOp::NegateAbs)
        {
            const u64 result = NegateAbsF64(instructionInfo.OperandA, instructionInfo.OperandB);
            m_Core->PrepareRegisterWrite(true, instructionInfo.StorageRegister, result);
            return;
        }

        f64 valueA;
        ::std::memcpy(&valueA, &instructionInfo.OperandA, sizeof(valueA));

        f64 result;

        switch(instructionInfo.Operation)
        {
            case EFpuOp::BasicBinOp:
            case EFpuOp::Fma:
            case EFpuOp::Compare:
            {
                f64 valueB;
                ::std::memcpy(&valueB, &instructionInfo.OperandB, sizeof(valueB));

                switch(instructionInfo.Operation)
                {
                    case EFpuOp::BasicBinOp:
                    {
                        result = BasicBinOpF64(valueA, valueB, static_cast<EBinOp>(instructionInfo.OperandC));
                        break;
                    }
                    case EFpuOp::Fma:
                    {
                        f64 valueC;
                        ::std::memcpy(&valueC, &instructionInfo.OperandC, sizeof(valueC));

                        result = FmaF64(valueA, valueB, valueC);
                        break;
                    }
                    case EFpuOp::Compare:
                    {
                        const u32 compResult = CompareF64(valueA, valueB);
                        m_Core->PrepareRegisterWrite(false, instructionInfo.StorageRegister, compResult);
                        return;
                    }
                    default: break;
                }

                break;
            }
            case EFpuOp::Round:
                result = RoundF64(static_cast<ERoundingMode>(instructionInfo.OperandB), valueA);
                break;
            default:
                result = ::std::numeric_limits<f32>::quiet_NaN();
                break;
        }

        u64 resultU;
        ::std::memcpy(&resultU, &result, sizeof(result));

        m_Core->PrepareRegisterWrite(true, instructionInfo.StorageRegister, resultU);
    }
}

f32 Fpu::BasicBinOpF32(const f32 valueA, const f32 valueB, const EBinOp op) noexcept
{
    switch(op)
    {
        case EBinOp::Add:
            m_ExecutionStage = 4;
            return valueA + valueB;
        case EBinOp::Subtract:
            m_ExecutionStage = 4;
            return valueA - valueB;
        case EBinOp::Multiply:
            m_ExecutionStage = 5;
            return valueA * valueB;
        case EBinOp::Divide:
            m_ExecutionStage = 6;
            return valueA / valueB;
        case EBinOp::Remainder:
            m_ExecutionStage = 6;
            return ::std::fmod(valueA, valueB);
        default:
            m_ExecutionStage = 1;
            return ::std::numeric_limits<f32>::quiet_NaN();
    }
}

f64 Fpu::BasicBinOpF64(const f64 valueA, const f64 valueB, const EBinOp op) noexcept
{
    switch(op)
    {
        case EBinOp::Add:
            m_ExecutionStage = 8;
            return valueA + valueB;
        case EBinOp::Subtract:
            m_ExecutionStage = 8;
            return valueA - valueB;
        case EBinOp::Multiply:
            m_ExecutionStage = 10;
            return valueA * valueB;
        case EBinOp::Divide:
            m_ExecutionStage = 12;
            return valueA / valueB;
        case EBinOp::Remainder:
            m_ExecutionStage = 12;
            return ::std::fmod(valueA, valueB);
        default:
            m_ExecutionStage = 1;
            return ::std::numeric_limits<f64>::quiet_NaN();
    }
}

f32 Fpu::FmaF32(const f32 valueA, const f32 valueB, const f32 valueC) noexcept
{
    m_ExecutionStage = 10;

    const __m128 valueAV = _mm_set_ss(valueA);
    const __m128 valueBV = _mm_set_ss(valueB);
    const __m128 valueCV = _mm_set_ss(valueC);

    const __m128 resultV = _mm_fmadd_ss(valueAV, valueBV, valueCV);

    return _mm_cvtss_f32(resultV);
}

f64 Fpu::FmaF64(const f64 valueA, const f64 valueB, const f64 valueC) noexcept
{
    m_ExecutionStage = 20;

    const __m128d valueAV = _mm_set_sd(valueA);
    const __m128d valueBV = _mm_set_sd(valueB);
    const __m128d valueCV = _mm_set_sd(valueC);

    const __m128d resultV = _mm_fmadd_sd(valueAV, valueBV, valueCV);

    return _mm_cvtsd_f64(resultV);
}

f32 Fpu::RoundF32(const ERoundingMode ERoundingMode, const f32 value) noexcept
{
    m_ExecutionStage = 4;

    switch(ERoundingMode)
    {
        case ERoundingMode::Truncate: return ::std::trunc(value);
        case ERoundingMode::Ceiling: return ::std::ceil(value);
        case ERoundingMode::Floor: return ::std::floor(value);
        case ERoundingMode::RoundTieToEven: return::std::round(value);
        default: return ::std::numeric_limits<f32>::quiet_NaN();
    }
}

u16 Fpu::RoundF16(const ERoundingMode ERoundingMode, const f32 value) noexcept
{
    m_ExecutionStage = 3;

    switch(ERoundingMode)
    {
        case ERoundingMode::Truncate:
        {
            const f32 result = ::std::trunc(value);
            return _cvtss_sh(result, _MM_FROUND_TO_ZERO | _MM_FROUND_NO_EXC);
        }
            break;
        case ERoundingMode::Ceiling:
        {
            const f32 result = ::std::ceil(value);
            return _cvtss_sh(result, _MM_FROUND_TO_POS_INF | _MM_FROUND_NO_EXC);
        }
            break;
        case ERoundingMode::Floor:
        {
            const f32 result = ::std::floor(value);
            return _cvtss_sh(result, _MM_FROUND_TO_NEG_INF | _MM_FROUND_NO_EXC);
        }
            break;
        case ERoundingMode::RoundTieToEven:
        {
            const f32 result = ::std::round(value);
            return _cvtss_sh(result, _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC);
        }
            break;
        default:
        {
            const f32 resultF = ::std::numeric_limits<f64>::quiet_NaN();
            u32 result;
            (void) ::std::memcpy(&result, &resultF, sizeof(u32));
            return static_cast<u16>(result);
        }
    }
}

f64 Fpu::RoundF64(const ERoundingMode ERoundingMode, const f64 value) noexcept
{
    m_ExecutionStage = 5;

    switch(ERoundingMode)
    {
        case ERoundingMode::Truncate: return ::std::trunc(value);
        case ERoundingMode::Ceiling: return ::std::ceil(value);
        case ERoundingMode::Floor: return ::std::floor(value);
        case ERoundingMode::RoundTieToEven: return::std::round(value);
        default: return ::std::numeric_limits<f64>::quiet_NaN();
    }
}

u32 Fpu::CompareF32(const f32 valueA, const f32 valueB) noexcept
{
    m_ExecutionStage = 4;
    CompareFlags result;
    result.Pad = 0;

    if(valueA == valueB)
    {
        result.Equal = 1;
    }
    else
    {
        result.Equal = 0;

        if(valueA > valueB)
        {
            result.Greater = 1;
        }
        else
        {
            result.Greater = 0;
        }
    }

    return result.Value;
}

u32 Fpu::CompareF64(const f64 valueA, const f64 valueB) noexcept
{
    m_ExecutionStage = 8;
    CompareFlags result;
    result.Pad = 0;

    if(valueA == valueB)
    {
        result.Equal = 1;
    }
    else
    {
        result.Equal = 0;

        if(valueA > valueB)
        {
            result.Greater = 1;
        }
        else
        {
            result.Greater = 0;
        }
    }

    return result.Value;
}

u32 Fpu::NegateAbsF32(u32 value, bool isAbs) noexcept
{
    m_ExecutionStage = 1;
    return isAbs ? value & 0x7FFFFFFF : value ^ 0x80000000;
}

u32 Fpu::NegateAbsF16(u32 value, bool isAbs) noexcept
{
    m_ExecutionStage = 1;
    return isAbs ? value & 0x7FFF : value ^ 0x8000;
}

u64 Fpu::NegateAbsF64(u64 value, bool isAbs) noexcept
{
    m_ExecutionStage = 1;
    return isAbs ? value & 0x7FFFFFFFFFFFFFFF : value ^ 0x8000000000000000;
}
