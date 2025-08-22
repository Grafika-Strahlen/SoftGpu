#include "FPU.hpp"

#include "Core.hpp"
#include <Common.hpp>

#if HAS_X86_INTRINSICS
#include <immintrin.h>
#endif

#include <cstring>
#include <limits>
#include <cmath>

#if !HAS_X86_INTRINSICS
#define _MM_FROUND_TO_NEAREST_INT 0x00
#define _MM_FROUND_TO_NEG_INF 0x01
#define _MM_FROUND_TO_POS_INF 0x02
#define _MM_FROUND_TO_ZERO 0x03
#define _MM_FROUND_CUR_DIRECTION 0x04

#define _MM_FROUND_RAISE_EXC 0x00
#define _MM_FROUND_NO_EXC 0x08

//  Produce value of bit n.  n must be less than 32.
#define Bit(n)  ((uint32_t) 1 << (n))

//  Create a mask of n bits in the low bits.  n must be less than 32.
#define Mask(n) (Bit(n) - 1)

/*
 *  Convert an IEEE-754 16-bit binary floating-point encoding to an IEEE-754
 *  32-bit binary floating-point encoding.
 *
 *  This code has not been tested.
 *
 *  Sourced from https://stackoverflow.com/a/71125539/5899776 Licensed under CC BY-SA 4.0 https://creativecommons.org/licenses/by-sa/4.0/
 */
uint32_t Float16ToFloat32(uint16_t x)
{
    /*  Separate the sign encoding (1 bit starting at bit 15), the exponent
        encoding (5 bits starting at bit 10), and the primary significand
        (fraction) encoding (10 bits starting at bit 0).
    */
    uint32_t s = x >> 15;
    uint32_t e = x >> 10 & Mask( 5);
    uint32_t f = x       & Mask(10);

    //  Left-adjust the significand field.
    f <<= 23 - 10;

    //  Switch to handle subnormal numbers, normal numbers, and infinities/NaNs.
    switch (e)
    {
        //  Exponent code is subnormal.
        case 0:
            //  Zero does need any changes, but subnormals need normalization.
            if (f != 0)
            {
                /*  Set the 32-bit exponent code corresponding to the 16-bit
                    subnormal exponent.
                */
                e = 1 + (127 - 15);

                /*  Normalize the significand by shifting until its leading
                    bit moves out of the field.  (This code could benefit from
                    a find-first-set instruction or possibly using a conversion
                    from integer to floating-point to do the normalization.)
                */
                while (f < Bit(23))
                {
                    f <<= 1;
                    e -= 1;
                }

                //  Remove the leading bit.
                f &= Mask(23);
            }
            break;

        // Exponent code is normal.
        default:
            e += 127 - 15;  //  Adjust from 16-bit bias to 32-bit bias.
            break;

        //  Exponent code indicates infinity or NaN.
        case 31:
            e = 255;        //  Set 32-bit exponent code for infinity or NaN.
            break;
    }

    //  Assemble and return the 32-bit encoding.
    return s << 31 | e << 23 | f;
}


//-----------------------------------------------------
// Float-to-half conversion -- general case, including
// zeroes, denormalized numbers and exponent overflows.
//-----------------------------------------------------
// This code is sourced from https://github.com/mitsuba-renderer/openexr/blob/master/IlmBase/Half/half.cpp#L85 Licensed under BSD 3-Clause.
u16 Float32ToFloat16(u32 i) noexcept
{
    //
    // Our floating point number, f, is represented by the bit
    // pattern in integer i.  Disassemble that bit pattern into
    // the sign, s, the exponent, e, and the significand, m.
    // Shift s into the position where it will go in in the
    // resulting half number.
    // Adjust e, accounting for the different exponent bias
    // of float and half (127 versus 15).
    //

    i32 s = static_cast<i32>( (i >> 16) & 0x00008000);
    i32 e = static_cast<i32>(((i >> 23) & 0x000000ff) - (127 - 15));
    i32 m = static_cast<i32>(  i        & 0x007fffff);

    //
    // Now reassemble s, e and m into a half:
    //

    if(e <= 0)
    {
	    if(e < -10)
	    {
	        //
	        // E is less than -10.  The absolute value of f is
	        // less than HALF_MIN (f may be a small normalized
	        // float, a denormalized float or a zero).
	        //
	        // We convert f to a half zero with the same sign as f.
	        //

	        return s;
	    }

	    //
	    // E is between -10 and 0.  F is a normalized float
	    // whose magnitude is less than HALF_NRM_MIN.
	    //
	    // We convert f to a denormalized half.
	    //

	    //
	    // Add an explicit leading 1 to the significand.
	    //

	    m = m | 0x00800000;

	    //
	    // Round to m to the nearest (10+e)-bit value (with e between
	    // -10 and 0); in case of a tie, round to the nearest even value.
	    //
	    // Rounding may cause the significand to overflow and make
	    // our number normalized.  Because of the way a half's bits
	    // are laid out, we don't have to treat this case separately;
	    // the code below will handle it correctly.
	    //

	    int t = 14 - e;
	    int a = (1 << (t - 1)) - 1;
	    int b = (m >> t) & 1;

	    m = (m + a + b) >> t;

	    //
	    // Assemble the half from s, e (zero) and m.
	    //

	    return s | m;
    }
    else if(e == 0xff - (127 - 15))
    {
	    if(m == 0)
	    {
	        //
	        // F is an infinity; convert f to a half
	        // infinity with the same sign as f.
	        //

	        return s | 0x7c00;
	    }
	    else
	    {
	        //
	        // F is a NAN; we produce a half NAN that preserves
	        // the sign bit and the 10 leftmost bits of the
	        // significand of f, with one exception: If the 10
	        // leftmost bits are all zero, the NAN would turn
	        // into an infinity, so we have to set at least one
	        // bit in the significand.
	        //

	        m >>= 13;
	        return s | 0x7c00 | m | (m == 0);
	    }
    }
    else
    {
	    //
	    // E is greater than zero.  F is a normalized float.
	    // We try to convert f to a normalized half.
	    //

	    //
	    // Round to m to the nearest 10-bit value.  In case of
	    // a tie, round to the nearest even value.
	    //

	    m = m + 0x00000fff + ((m >> 13) & 1);

	    if(m & 0x00800000)
	    {
	        m =  0;		// overflow in significand,
	        e += 1;		// adjust exponent
	    }

	    //
	    // Handle exponent overflow
	    //

	    if(e > 30)
	    {
	        // overflow ();	// Cause a hardware floating point overflow;
	        return s | 0x7c00;	// if this returns, the half becomes an
	    }   			// infinity with the same sign as f.

	    //
	    // Assemble the half from s, e and m.
	    //

	    return s | (e << 10) | (m >> 13);
    }
}

f32 _cvtsh_ss(const u16 f) noexcept
{
    const u32 floatBits = Float16ToFloat32(f);
    return ::std::bit_cast<f32>(floatBits);
}

u16 _cvtss_sh(const f32 f, int) noexcept
{
    const u32 floatBits = ::std::bit_cast<u32>(f);
    return Float32ToFloat16(floatBits);
}

#endif

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

#if HAS_X86_INTRINSICS
    const __m128 valueAV = _mm_set_ss(valueA);
    const __m128 valueBV = _mm_set_ss(valueB);
    const __m128 valueCV = _mm_set_ss(valueC);

    const __m128 resultV = _mm_fmadd_ss(valueAV, valueBV, valueCV);

    return _mm_cvtss_f32(resultV);
#else
    return valueA * valueB + valueC;
#endif
}

f64 Fpu::FmaF64(const f64 valueA, const f64 valueB, const f64 valueC) noexcept
{
    m_ExecutionStage = 20;

#if HAS_X86_INTRINSICS
    const __m128d valueAV = _mm_set_sd(valueA);
    const __m128d valueBV = _mm_set_sd(valueB);
    const __m128d valueCV = _mm_set_sd(valueC);

    const __m128d resultV = _mm_fmadd_sd(valueAV, valueBV, valueCV);

    return _mm_cvtsd_f64(resultV);
#else
    return valueA * valueB + valueC;
#endif
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
