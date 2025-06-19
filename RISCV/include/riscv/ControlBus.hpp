#pragma once

#include <Common.hpp>

namespace riscv {

struct ControlBus final
{
    DEFAULT_CONSTRUCT_PU(ControlBus);
    DEFAULT_DESTRUCT(ControlBus);
    DEFAULT_CM_PU(ControlBus);
public:
    u32 IF_Fence : 1;
    u32 IF_Reset : 1;
    u32 IF_Acknowledge : 1;
    u32 Pad0 : 29;

    u32 PC_Current;
    u32 PC_Next;
    u32 PC_Return;

    u32 RF_WriteBackEnable : 1;
    u32 RF_RS1_Address : 5;
    u32 RF_RS2_Address : 5;
    u32 RF_RD_Address : 5;
    u32 RF_ZeroWriteEnable : 1;
    u32 Pad1 : 15;

    u32 ALU_OperationSelect : 3;
    u32 ALU_Subtract : 1;
    u32 ALU_OperandSelectA : 1; // 0 -> RS1, 1 -> PC
    u32 ALU_OperandSelectB : 1; // 0 -> RS2, 1 -> IMM
    u32 ALU_Unsigned : 1;
    u32 ALU_BaseCoProcessorTrigger : 1;
    u32 ALU_CFUCoProcessorTrigger : 1;
    u32 ALU_FPUCoProcessorTrigger : 1;
    u32 Pad2 : 22;
    u32 ALU_Immediate;

    u32 IR_Function3 : 3;
    u32 IR_Function12 : 12;
    u32 IR_Opcode : 6;
    u32 Pad4 : 11;

    u32 CPU_Sleep : 1;
    u32 CPU_Trap : 1;
    u32 CPU_Debug : 1;
    u32 Pad5 : 29;
public:
    static constexpr u32 Function3_ALU_AddSubtract       = 0b000;
    static constexpr u32 Function3_ALU_ShiftLeft         = 0b001;
    static constexpr u32 Function3_ALU_SetOnLess         = 0b010;
    static constexpr u32 Function3_ALU_SetOnLessUnsigned = 0b011;
    static constexpr u32 Function3_ALU_XOR               = 0b100;
    static constexpr u32 Function3_ALU_ShiftRight        = 0b101;
    static constexpr u32 Function3_ALU_OR                = 0b110;
    static constexpr u32 Function3_ALU_AND               = 0b111;
};

}
