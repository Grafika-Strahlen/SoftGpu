#pragma once

#include <Objects.hpp>
#include <NumTypes.hpp>

enum class EFpuOp : u32
{
    BasicBinOp = 0, // Operand C contains the actual operation.
    Fma,
    Round, // Operand B contains the rounding mode.
    Compare,
    NegateAbs, // If operand b is 0 Negate, if 1 Abs (implemented as !0)
};

enum class EBinOp : u32
{
    Add = 0,
    Subtract,
    Multiply,
    Divide,
    Remainder
};

enum class ERoundingMode : u32
{
    Truncate = 0, // Round toward 0
    Ceiling, // Round toward +inf
    Floor, // Round toward -inf
    RoundTieToEven
};

enum class EPrecision : u32
{
    Single = 0,
    Half,
    Double
};

struct CompareFlags final
{
    union
    {
        struct
        {
            u32 Equal : 1;
            u32 Greater : 1;
            u32 Pad : 30;
        };
        u32 Value;
    };
};

struct FpuInstruction final
{
    u32 DispatchPort : 1; // Which Dispatch Port invoked this.
    EFpuOp Operation : 3; // What operation is being performed on the operands
    EPrecision Precision : 2; // What precision is being used
    u32 Reserved0 : 26; // Reserved bits for alignment in x86, these can be removed in hardware.
    u64 OperandA : 12; // The first operand register.
    u64 OperandB : 12; // The second operand register.
    u64 OperandC : 12; // The third operand register.
    u64 StorageRegister : 12;  // The storage register. This gives a 256 register window.
    u64 Reserved1 : 16;  // 26 reserved bits for alignment in x86, these can be removed in hardware.
};

struct LoadedFpuInstruction final
{
    u32 DispatchPort : 1; // Which Dispatch Port invoked this.
    EFpuOp Operation : 3; // What operation is being performed on the operands
    EPrecision Precision : 2; // What precision is being used
    u32 StorageRegister : 12;  // The storage register.
    u32 Reserved : 14; // Reserved bits for alignment in x86, these can be removed in hardware.
    u64 OperandA; // The first operand.
    u64 OperandB; // The second operand.
    u64 OperandC; // The third operand.
};

class ICore;

class Fpu final
{
    DEFAULT_DESTRUCT(Fpu);
    DELETE_CM(Fpu);
public:
    Fpu(ICore* const core) noexcept
        : m_Core(core)
        , m_ExecutionStage(0)
        , m_DispatchPort{ }
        , m_ReplicationIndex{ }
        , m_StorageRegister{ }
        , m_StorageRegisterCount(0)
    { }

    void Reset()
    {
        m_ExecutionStage = 0;
        m_DispatchPort = { };
        m_ReplicationIndex = { };
        m_StorageRegister = { };
        m_StorageRegisterCount = 0;
    }

    void Clock() noexcept;

    void ExecuteInstruction(LoadedFpuInstruction instructionInfo) noexcept;
private:
    [[nodiscard]] f32 BasicBinOpF32(f32 valueA, f32 valueB, EBinOp op) noexcept;
    [[nodiscard]] f64 BasicBinOpF64(f64 valueA, f64 valueB, EBinOp op) noexcept;
    [[nodiscard]] f32 FmaF32(f32 valueA, f32 valueB, f32 valueC) noexcept;
    [[nodiscard]] f64 FmaF64(f64 valueA, f64 valueB, f64 valueC) noexcept;
    [[nodiscard]] f32 RoundF32(ERoundingMode roundingMode, f32 value) noexcept;
    [[nodiscard]] u16 RoundF16(ERoundingMode roundingMode, f32 value) noexcept;
    [[nodiscard]] f64 RoundF64(ERoundingMode roundingMode, f64 value) noexcept;
    [[nodiscard]] u32 CompareF32(f32 valueA, f32 valueB) noexcept;
    [[nodiscard]] u32 CompareF64(f64 valueA, f64 valueB) noexcept;
    [[nodiscard]] u32 NegateAbsF32(u32 value, bool isAbs) noexcept;
    [[nodiscard]] u32 NegateAbsF16(u32 value, bool isAbs) noexcept;
    [[nodiscard]] u64 NegateAbsF64(u64 value, bool isAbs) noexcept;
private:
    ICore* m_Core;
    u32 m_ExecutionStage;
    u32 m_DispatchPort;
    u32 m_ReplicationIndex;
    u32 m_StorageRegister;
    u32 m_StorageRegisterCount;
};

[[nodiscard]] inline u8 RequiredRegisterCount(const EFpuOp op) noexcept
{
    switch(op)
    {
        case EFpuOp::BasicBinOp: return 1;
        case EFpuOp::Fma: return 2;
        case EFpuOp::Round: return 0;
        case EFpuOp::Compare: return 1;
        case EFpuOp::NegateAbs: return 0;
        default: return 0;
    }
}
