#pragma once

#include <Common.hpp>
#include <Objects.hpp>
#include <NumTypes.hpp>

namespace riscv {

struct ControlBus final
{
    DEFAULT_CONSTRUCT_PU(ControlBus);
    DEFAULT_DESTRUCT(ControlBus);
    DEFAULT_CM_PU(ControlBus);
public:
    u32 RF_WriteBackEnable : 1;
    u32 RF_RS1_Address : 5;
    u32 RF_RS2_Address : 5;
    u32 RF_RD_Address : 5;
    u32 RF_ZeroWriteEnable : 1;
    u32 Pad0 : 15;

    u32 IR_Funct3 : 3;
    u32 IR_Funct12 : 12;
    u32 IR_Opcode : 6;
    u32 Pad1 : 11;

    u32 CPU_Sleep : 1;
    u32 CPU_Trap : 1;
    u32 CPU_Debug : 1;
    u32 Pad2 : 29;
};

}
