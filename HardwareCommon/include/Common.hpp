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

// Lots of stuff needs the main processor, so we'll just keep it defined here.
class Processor;

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
    }
