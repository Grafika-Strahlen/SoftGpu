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
    Cross,
    Dot, // Operand B contains the number of sequential register to normalize [2,4]
    Normalize // Operand B contains the number of sequential register to normalize [2,4]
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
    u32 ReplicationIndex : 2; // For pixels we replicate 4 times, we need to know which replication we are for the register file view.
    EFpuOp Operation : 3; // What operation is being performed on the operands
    EPrecision Precision : 2; // What precision is being used
    u32 Reserved : 24; // 26 reserved bits for alignment in x86, these can be removed in hardware.
    u32 OperandA : 8; // The first operand register. This gives a 256 register window.
    u32 OperandB : 8; // The second operand register. This gives a 256 register window.
    u32 OperandC : 8; // The third operand register. This is only used by FMA. This gives a 256 register window.
    u32 StorageRegister : 8;  // The storage register. This gives a 256 register window.
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
        , m_ReplicationIndex { }
        , m_StorageRegister{ }
        , m_StorageRegisterCount(0)
    { }

    void Clock() noexcept;

    void InitiateInstruction(FpuInstruction instructionInfo) noexcept;

    [[nodiscard]] bool ReadyToExecute() const noexcept
    {
        return m_ExecutionStage == 0;
    }
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
    void Cross(FpuInstruction instructionInfo) noexcept;
    void Dot(FpuInstruction instructionInfo) noexcept;
    void Normalize(FpuInstruction instructionInfo) noexcept;
private:
    ICore* m_Core;
    u32 m_ExecutionStage;
    u32 m_DispatchPort;
    u32 m_ReplicationIndex;
    u32 m_StorageRegister;
    u32 m_StorageRegisterCount;
};
