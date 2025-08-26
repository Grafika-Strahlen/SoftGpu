#pragma once

#include <NumTypes.hpp>

#include "PciProtocol.hpp"

namespace pcie {

#pragma pack(push, 1)
struct TlpHeader final
{
    static inline constexpr u32 FORMAT_3_DW_HEADER_NO_DATA   = 0b00;
    static inline constexpr u32 FORMAT_4_DW_HEADER_NO_DATA   = 0b01;
    static inline constexpr u32 FORMAT_3_DW_HEADER_WITH_DATA = 0b10;
    static inline constexpr u32 FORMAT_4_DW_HEADER_WITH_DATA = 0b11;

    static inline constexpr u32 TYPE_MEMORY_REQUEST             = 0b0'0000;
    static inline constexpr u32 TYPE_MEMORY_READ_REQUEST_LOCKED = 0b0'0001;
    static inline constexpr u32 TYPE_IO_REQUEST                 = 0b0'0010;
    static inline constexpr u32 TYPE_CONFIG_TYPE_0_REQUEST      = 0b0'0100;
    static inline constexpr u32 TYPE_CONFIG_TYPE_1_REQUEST      = 0b0'0101;
    static inline constexpr u32 TYPE_MESSAGE                    = 0b1'0000;
    static inline constexpr u32 TYPE_COMPLETION                 = 0b0'1010;
    static inline constexpr u32 TYPE_COMPLETION_LOCKED_READ     = 0b0'1011;

public:
    u32 Reserved0 : 1;
    u32 Fmt : 2;
    u32 Type : 5;
    u32 Reserved1 : 1;
    u32 TC : 3;
    u32 Reserved2 : 4;
    u32 TD : 1;
    u32 EP : 1;
    u32 Attr : 2;
    u32 Reserved3 : 2;
    u32 Length : 10;
};

static_assert(sizeof(TlpHeader) == 4, "TLP Header was not 4 bytes in length.");
#pragma pack(pop)

}
