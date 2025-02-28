#pragma once

#include <Objects.hpp>
#include <NumTypes.hpp>

struct GDDR5BusSpec final
{
    u32 CK_t : 1;
    u32 CK_c : 1;
    u32 WCK01_t : 1;
    u32 WCK01_c : 1;
    u32 WCK23_t : 1;
    u32 WCK23_c : 1;
    u32 CKE_n : 1;
    u32 CS_n : 1;
    u32 RAS_n : 1;
    u32 CAS_n : 1;
    u32 WE_n : 1;
    u32 BA : 4;
    u32 A : 14;
    u32 Pad0 : 3;
    u32 DQ;
    u32 DBI_n : 4;
    u32 EDC : 4;
    u32 ABI_n : 4;
    u32 RESET_n : 1;
    u32 Pad1 : 19;
};

class GDDR5ChipController final
{
private:
    enum class Sensitivity
    {
        Reset = 0,
        Clock
    };

    template<Sensitivity TargetSense>
    bool RisingEdge(const u32 bit, const Sensitivity sensitivity) noexcept
    {
        return bit && sensitivity == TargetSense;
    }

    template<Sensitivity TargetSense>
    bool FallingEdge(const u32 bit, const Sensitivity sensitivity) noexcept
    {
        return !bit && sensitivity == TargetSense;
    }
public:
    GDDR5ChipController()
        : p_Reset_n(0)
        , p_Clock(0)
        , p_Pad0(0)
        , m_Chip{}
        , m_InReset(1)
        , m_Pad1(0)
    {
        m_Chip.CK_t = 0;
        m_Chip.CK_c = 0;

        m_Chip.WCK01_t = 0;
        m_Chip.WCK01_c = 0;
        m_Chip.WCK23_t = 0;
        m_Chip.WCK23_c = 0;

        m_Chip.CKE_n = 0;

        m_Chip.RESET_n = 0;
    }

    void SetReset(const bool reset_n) noexcept
    {
        p_Reset_n = reset_n;

        OnProcess(Sensitivity::Reset);
    }

    void SetClock(const bool clock) noexcept
    {
        p_Clock = clock;

        OnProcess(Sensitivity::Clock);
    }
private:
    void OnProcess(const Sensitivity trigger)
    {
        if(FallingEdge<Sensitivity::Reset>(p_Reset_n, trigger))
        {
            OnResetProcess();
        }

        if(trigger == Sensitivity::Clock)
        {
            OnClockProcess();
        }
    }

    void OnResetProcess() noexcept
    {
        // Tell the chip to reset.
        m_Chip.RESET_n = 0;
        // Wait until the next rising clock cycle to clear the reset.
        m_InReset = 1;
    }

    void OnClockProcess() noexcept
    {
        if(m_InReset)
        {
            // Clear a reset on the falling edge.
            if(!p_Clock)
            {
                m_Chip.RESET_n = 1;
                m_Chip.CKE_n = 0;
                m_InReset = false;
            }
            return;
        }

        const u32 clockComplement = p_Clock ? 0 : 1;

        m_Chip.CK_t = p_Clock;
        m_Chip.CK_c = clockComplement;

        m_Chip.WCK01_t = p_Clock;
        m_Chip.WCK01_c = clockComplement;
        m_Chip.WCK23_t = p_Clock;
        m_Chip.WCK23_c = clockComplement;
    }
private:
    u32 p_Reset_n : 1;
    u32 p_Clock : 1;
    u32 p_Pad0 : 30;

    GDDR5BusSpec m_Chip;
    u32 m_InReset : 1;
    u32 m_Pad1 : 31;
};

class GDDR5Controller final
{
public:
    static inline constexpr uSys CHIP_COUNT = 4;
public:
    void Reset() noexcept
    {
        for(u32 i = 0; i < CHIP_COUNT; ++i)
        {
            m_Chips[i].SetReset(false);
            m_Chips[i].SetReset(true);
        }
    }

    void Clock(bool risingEdge = true) noexcept
    {
        for(u32 i = 0; i < CHIP_COUNT; ++i)
        {
            m_Chips[i].SetClock(risingEdge);
        }
    }
private:
    GDDR5ChipController m_Chips[CHIP_COUNT];
};
