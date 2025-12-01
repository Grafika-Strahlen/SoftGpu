/**
 * @file
 *
 * Copyright (c) 2025. Grafika Strahlen LLC
 * All rights reserved.
 */
#pragma once

#include <Objects.hpp>
#include <NumTypes.hpp>
#include <BitSet.hpp>
#include <cassert>
#include <climits>

#include "BitVector.hpp"

#if (defined(_M_X64) && _M_X64 == 100) || defined(__x86_64__)
    #define HAS_X86_INTRINSICS (1)
#else
    #define HAS_X86_INTRINSICS (0)
#endif

#define SET_HI_Z(X)

using WordType = u32;
static inline constexpr WordType WordBitCount = sizeof(WordType) * CHAR_BIT;

enum class EReadWrite : u32
{
    Read = 0,
    Write = 1
};

[[nodiscard]] static u32 BOOL_TO_BIT(const bool b) noexcept
{
    return b ? 1 : 0;
}

[[nodiscard]] static bool BIT_TO_BOOL(const u32 b) noexcept
{
    return b != 0;
}

// 4 bits, 3 if you don't care about don't care.
enum class StdULogic : u32
{
    Uninitialized,          // U
    MultipleDrivers,        // X
    Zero,                   // 1
    One,                    // 0
    HighImpedance,          // Z
    WeakMultipleDrivers,    // W
    WeakLow,                // L
    WeakHigh,               // H
    DontCare,               // -

    MAX_PRIME,
    MAX = MAX_PRIME - 1,
    U = Uninitialized,
    X = MultipleDrivers,
    O = Zero,
    I = One,
    Z = HighImpedance,
    W = WeakMultipleDrivers,
    L = WeakLow,
    H = WeakHigh,
    _ = DontCare
};

using StdLogic = StdULogic;

namespace vhdl::internal {

static constexpr StdLogic ResolutionTable[static_cast<u32>(StdLogic::MAX_PRIME)][static_cast<u32>(StdLogic::MAX_PRIME)] = {
    //       U            X            0            1            Z            W            L            H            -
    { StdLogic::U, StdLogic::U, StdLogic::U, StdLogic::U, StdLogic::U, StdLogic::U, StdLogic::U, StdLogic::U, StdLogic::U }, // U
    { StdLogic::U, StdLogic::X, StdLogic::X, StdLogic::X, StdLogic::X, StdLogic::X, StdLogic::X, StdLogic::X, StdLogic::X }, // X
    { StdLogic::U, StdLogic::X, StdLogic::O, StdLogic::X, StdLogic::O, StdLogic::O, StdLogic::O, StdLogic::O, StdLogic::X }, // 0
    { StdLogic::U, StdLogic::X, StdLogic::X, StdLogic::I, StdLogic::I, StdLogic::I, StdLogic::I, StdLogic::I, StdLogic::X }, // 1
    { StdLogic::U, StdLogic::X, StdLogic::O, StdLogic::I, StdLogic::Z, StdLogic::W, StdLogic::L, StdLogic::H, StdLogic::X }, // Z
    { StdLogic::U, StdLogic::X, StdLogic::O, StdLogic::I, StdLogic::W, StdLogic::W, StdLogic::W, StdLogic::W, StdLogic::X }, // W
    { StdLogic::U, StdLogic::X, StdLogic::O, StdLogic::I, StdLogic::L, StdLogic::W, StdLogic::L, StdLogic::W, StdLogic::X }, // L
    { StdLogic::U, StdLogic::X, StdLogic::O, StdLogic::I, StdLogic::H, StdLogic::W, StdLogic::W, StdLogic::H, StdLogic::X }, // H
    { StdLogic::U, StdLogic::X, StdLogic::X, StdLogic::X, StdLogic::X, StdLogic::X, StdLogic::X, StdLogic::X, StdLogic::X }, // -
};

}

[[maybe_unused]] [[nodiscard]] static StdLogic ResolveStdLogic(const StdLogic a, const StdLogic b) noexcept
{
    return vhdl::internal::ResolutionTable[static_cast<u32>(a)][static_cast<u32>(b)];
}

template<uSys ElementCountT, uSys TBitCountT = 4>
[[nodiscard]] static TBitVector<ElementCountT, TBitCountT, StdLogic> ResolveStdLogic(
    const TBitVector<ElementCountT, TBitCountT, StdLogic>& a,
    const TBitVector<ElementCountT, TBitCountT, StdLogic>& b
) noexcept
{
    TBitVector<ElementCountT, TBitCountT, StdLogic> res;

    for(uSys i = 0; i < ElementCountT; ++i)
    {
        res[i] = ResolveStdLogic(a[i], b[i]);
    }

    return res;
}

[[maybe_unused]] [[nodiscard]] static bool LOGIC_TO_BOOL(const StdLogic b) noexcept
{
    assert(b != StdLogic::X);
    assert(b != StdLogic::W);

    switch(b)
    {
        case StdLogic::U: return false;
        case StdLogic::X: return false;
        case StdLogic::O: return false;
        case StdLogic::I: return true;
        case StdLogic::Z: return false;
        case StdLogic::W: return false;
        case StdLogic::L: return false;
        case StdLogic::H: return true;
        case StdLogic::_: return false;
        default: return false;
    }
}

[[maybe_unused]] [[nodiscard]] static u32 LOGIC_TO_BIT(const StdLogic b) noexcept
{
    return BOOL_TO_BIT(LOGIC_TO_BOOL(b));
}

[[maybe_unused]] [[nodiscard]] static StdLogic BOOL_TO_LOGIC(const bool b) noexcept
{
    return b ? StdLogic::One : StdLogic::Zero;
}

[[maybe_unused]] [[nodiscard]] static StdLogic BOOL_TO_LOGIC_WEAK(const bool b) noexcept
{
    return b ? StdLogic::WeakHigh : StdLogic::WeakLow;
}

[[maybe_unused]] [[nodiscard]] static StdLogic BIT_TO_LOGIC(const u32 b) noexcept
{
    return BOOL_TO_LOGIC(BIT_TO_BOOL(b));
}

[[maybe_unused]] [[nodiscard]] static StdLogic BIT_TO_LOGIC_WEAK(const u32 b) noexcept
{
    return BOOL_TO_LOGIC_WEAK(BIT_TO_BOOL(b));
}

template<typename T, typename TBit>
static T GetBit(const T data, const TBit bit) noexcept
{
    const T bitT = static_cast<T>(bit);
    return (data >> bitT) & 0b1;
}

template<typename T, typename TBit>
static bool GetBitBool(const T data, const TBit bit) noexcept
{
    return BIT_TO_BOOL(GetBit(data, bit));
}

template<typename T, typename TBit>
static T SetBit(const T data, const TBit bit, const bool set) noexcept
{
    const T bitT = static_cast<T>(bit);
    if(set)
    {
        return data | (1 << bitT);
    }
    else
    {
        return data & ~(1 << bitT);
    }
}

template<typename Sensitivity, Sensitivity TargetSense>
[[nodiscard]] bool EventVHDL(const Sensitivity trigger) noexcept
{
    return trigger == TargetSense;
}

template<typename Sensitivity, Sensitivity TargetSense>
[[nodiscard]] bool RisingEdgeVHDL(const u32 bit, const Sensitivity sensitivity) noexcept
{
    return bit && EventVHDL<Sensitivity, TargetSense>(sensitivity);
}

template<typename Sensitivity, Sensitivity TargetSense>
[[maybe_unused]] [[nodiscard]] bool FallingEdgeVHDL(const u32 bit, const Sensitivity sensitivity) noexcept
{
    return !bit && EventVHDL<Sensitivity, TargetSense>(sensitivity);
}

template<typename Sensitivity>
[[maybe_unused]] [[nodiscard]] bool ProcessVHDL(const Sensitivity trigger) noexcept
{
    (void) trigger;
    return false;
}

template<typename Sensitivity, Sensitivity TargetSense, Sensitivity... RemainingSenses>
[[maybe_unused]] [[nodiscard]] bool ProcessVHDL(const Sensitivity trigger) noexcept
{
    if(trigger == TargetSense)
    {
        return true;
    }

    return ProcessVHDL<Sensitivity, RemainingSenses...>(trigger);
}

template<u32 Count, typename T>
[[maybe_unused]] [[nodiscard]] T ReplicateToVector(const bool value) noexcept
{
    T ret = 0;
    for(T i = 0; i < Count; ++i)
    {
        ret |= BOOL_TO_BIT(value) << i;
    }
    return ret;
}

// Lots of stuff needs the main processor, so we'll just keep it defined here.
// TODO: Remove
class Processor;

#define SENSITIVITY_DECL(...) \
    enum class Sensitivity : u8                                                                         \
    {                                                                                                   \
        __VA_ARGS__                                                                                     \
    }

#define SIGNAL_ENTITIES() \
    template<Sensitivity TargetSense>                                                                   \
    [[nodiscard]] bool Event(const Sensitivity trigger) noexcept                                        \
    {                                                                                                   \
        return EventVHDL<Sensitivity, TargetSense>(trigger);                                            \
    }                                                                                                   \
                                                                                                        \
    template<Sensitivity TargetSense>                                                                   \
    [[nodiscard]] bool RisingEdge(const u32 bit, const Sensitivity sensitivity) noexcept                \
    {                                                                                                   \
        return RisingEdgeVHDL<Sensitivity, TargetSense>(bit, sensitivity);                              \
    }                                                                                                   \
                                                                                                        \
    template<Sensitivity TargetSense>                                                                   \
    [[nodiscard]] bool FallingEdge(const u32 bit, const Sensitivity sensitivity) noexcept               \
    {                                                                                                   \
        return FallingEdgeVHDL<Sensitivity, TargetSense>(bit, sensitivity);                             \
    }                                                                                                   \
                                                                                                        \
    template<Sensitivity TargetSense, Sensitivity... RemainingSenses>                                   \
    [[nodiscard]] bool Process(const Sensitivity trigger) noexcept                                      \
    {                                                                                                   \
        if(trigger == TargetSense)                                                                      \
        {                                                                                               \
            return true;                                                                                \
        }                                                                                               \
                                                                                                        \
        return ProcessVHDL<Sensitivity, RemainingSenses...>(trigger);                                   \
    }

#define PROCESSES_DECL() \
    void Processes([[maybe_unused]] const Sensitivity trigger) noexcept

#define PROCESS_DECL(Name) \
    void Name([[maybe_unused]] const Sensitivity trigger) noexcept

#define TRIGGER_SENSITIVITY(Sense) \
    Processes(Sensitivity::Sense)

#define EVENT(Sense) \
    Event<Sensitivity::Sense>(trigger)

#define RISING_EDGE(Sense) \
    RisingEdge<Sensitivity::Sense>(Sense, trigger)

#define FALLING_EDGE(Sense) \
    FallingEdge<Sensitivity::Sense>(Sense, trigger)

#define RISING_EDGE_MUX(Sense) \
    RisingEdge<Sensitivity::Sense>(Sense(), trigger)

#define FALLING_EDGE_MUX(Sense) \
    FallingEdge<Sensitivity::Sense>(Sense(), trigger)

#define VHDL_INTERNAL_EXPAND(X) X
#define VHDL_INTERNAL_CAT(A, B) A ## B
#define VHDL_INTERNAL_SELECT(NAME, NUM) VHDL_INTERNAL_CAT(NAME, NUM)
#define VHDL_INTERNAL_GET_COUNT(_1, _2, _3, _4, _5, _6, _7, _8, N, ...) N
#define VHDL_INTERNAL_VA_SIZE(...) VHDL_INTERNAL_EXPAND(VHDL_INTERNAL_GET_COUNT(__VA_ARGS__, 8, 7, 6, 5, 4, 3, 2, 1))

#define VHDL_SENSITIVITY_1(Sense1) Sensitivity::Sense1
#define VHDL_SENSITIVITY_2(Sense1, Sense2) VHDL_SENSITIVITY_1(Sense1), VHDL_SENSITIVITY_1(Sense2)
#define VHDL_SENSITIVITY_3(Sense1, Sense2, Sense3) VHDL_SENSITIVITY_2(Sense1, Sense2), VHDL_SENSITIVITY_1(Sense3)
#define VHDL_SENSITIVITY_4(Sense1, Sense2, Sense3, Sense4) VHDL_SENSITIVITY_3(Sense1, Sense2, Sense3), VHDL_SENSITIVITY_1(Sense4)
#define VHDL_SENSITIVITY_5(Sense1, Sense2, Sense3, Sense4, Sense5) VHDL_SENSITIVITY_4(Sense1, Sense2, Sense3, Sense4), VHDL_SENSITIVITY_1(Sense5)
#define VHDL_SENSITIVITY_6(Sense1, Sense2, Sense3, Sense4, Sense5, Sense6) VHDL_SENSITIVITY_5(Sense1, Sense2, Sense3, Sense4, Sense5), VHDL_SENSITIVITY_1(Sense6)
#define VHDL_SENSITIVITY_7(Sense1, Sense2, Sense3, Sense4, Sense5, Sense6, Sense7) VHDL_SENSITIVITY_6(Sense1, Sense2, Sense3, Sense4, Sense5, Sense6), VHDL_SENSITIVITY_1(Sense7)
#define VHDL_SENSITIVITY_8(Sense1, Sense2, Sense3, Sense4, Sense5, Sense6, Sense7, Sense8) VHDL_SENSITIVITY_7(Sense1, Sense2, Sense3, Sense4, Sense5, Sense6, Sense7), VHDL_SENSITIVITY_1(Sense8)

#define VHDL_SENSITIVITY_N_ARGS(N, ...) VHDL_INTERNAL_EXPAND(VHDL_INTERNAL_SELECT(VHDL_SENSITIVITY_, N)(__VA_ARGS__))
#define VHDL_SENSITIVITY_ARGS(...) VHDL_SENSITIVITY_N_ARGS(VHDL_INTERNAL_VA_SIZE(__VA_ARGS__), __VA_ARGS__)

#define PROCESS_BEGIN(...) \
    if(Process<VHDL_SENSITIVITY_ARGS(__VA_ARGS__)>(trigger))

#define PROCESS_ENTER(ProcessName, ...) \
    PROCESS_BEGIN(__VA_ARGS__)                                                                          \
    {                                                                                                   \
        ProcessName(trigger);                                                                           \
    }

#define ENTER_PROCESS_M(Name) \
    Name(trigger)


#define STD_LOGIC_DECL(...) \
    enum class StdLogicVars : u8                                                                        \
    {                                                                                                   \
        __VA_ARGS__                                                                                     \
    };                                                                                                  \
    void StdLogicResetHandler() noexcept                                                                \
    {                                                                                                   \
        m_StdLogicTracker = 0;                                                                          \
    }                                                                                                   \
    u64 m_StdLogicTracker = 0                                                                           \

#define STD_LOGIC_PROCESS_RESET_HANDLER(...) \
    PROCESS_BEGIN(__VA_ARGS__)                                                                          \
    {                                                                                                   \
        StdLogicResetHandler();                                                                         \
    }

#define STD_LOGIC_SET(Name, Value) \
    if((m_StdLogicTracker & (1 << static_cast<u32>(StdLogicVars::Name))) == 0)                          \
    {                                                                                                   \
        Name = Value;                                                                                   \
    }                                                                                                   \
    else                                                                                                \
    {                                                                                                   \
        Name = ResolveStdLogic(Name, Value);                                                            \
    }                                                                                                   \
    m_StdLogicTracker |= (1 << static_cast<u32>(StdLogicVars::Name))

#define STD_ULOGIC_SET(Name, Value) \
    assert((m_StdLogicTracker & (1 << static_cast<u32>(StdLogicVars::Name))) == 0);                     \
    Name = Value;                                                                                       \
    m_StdLogicTracker |= (1 << static_cast<u32>(StdLogicVars::Name))
