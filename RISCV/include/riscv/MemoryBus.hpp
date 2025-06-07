#pragma once

#include <Common.hpp>

namespace riscv {

struct MemoryBusRequest final
{
    DEFAULT_CONSTRUCT_PU(MemoryBusRequest);
    DEFAULT_DESTRUCT(MemoryBusRequest);
    DEFAULT_CM_PU(MemoryBusRequest);
public:
    u32 Address;
    u32 WriteData;
    u32 ByteEnable : 4;
    u32 ReadWrite : 1; // Read = 0, Write = 1
    u32 Source : 1; // Data Access = 0, Instruction Fetch = 1
    u32 Atomic : 1;
    u32 AtomicOperation : 4;
    u32 Fence : 1;
    u32 Pad0 : 20;
};

struct MemoryBusResponse final
{
    DEFAULT_CONSTRUCT_PU(MemoryBusResponse);
    DEFAULT_DESTRUCT(MemoryBusResponse);
    DEFAULT_CM_PU(MemoryBusResponse);
public:
    u32 Data;
    u32 Acknowledge : 1; // Data is in undefined state if this is 0
    u32 Error : 1;
    u32 Pad0 : 30;
};

}
