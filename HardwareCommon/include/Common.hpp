#pragma once

#include <Objects.hpp>
#include <NumTypes.hpp>

#define SET_HI_Z(X)

[[nodiscard]] static u32 BOOL_TO_BIT(const bool b) noexcept
{
    return b ? 1 : 0;
}

[[nodiscard]] static bool BIT_TO_BOOL(const u32 b) noexcept
{
    return b != 0;
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
[[nodiscard]] bool FallingEdgeVHDL(const u32 bit, const Sensitivity sensitivity) noexcept
{
    return !bit && EventVHDL<Sensitivity, TargetSense>(sensitivity);
}

template<typename Sensitivity>
[[nodiscard]] bool ProcessVHDL(const Sensitivity trigger) noexcept
{
    return false;
}

template<typename Sensitivity, Sensitivity TargetSense, Sensitivity... RemainingSenses>
[[nodiscard]] bool ProcessVHDL(const Sensitivity trigger) noexcept
{
    if(trigger == TargetSense)
    {
        return true;
    }

    return ProcessVHDL<Sensitivity, RemainingSenses...>(trigger);
}

// Lots of stuff needs the main processor, so we'll just keep it defined here.
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
    void Processes(const Sensitivity trigger) noexcept

#define PROCESS_DECL(Name) \
    void Name(const Sensitivity trigger) noexcept

#define TRIGGER_SENSITIVITY(Sense) \
    Processes(Sensitivity::Sense)

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
