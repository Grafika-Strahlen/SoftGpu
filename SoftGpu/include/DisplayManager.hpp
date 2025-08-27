/**
 * @file
 *
 * Copyright (c) 2025. Grafika Strahlen LLC
 * All rights reserved.
 */
#pragma once

#include <NumTypes.hpp>
#include <Objects.hpp>
#include <Common.hpp>
#include <functional>
#include <atomic>
#include <cassert>
#include <cstring>

struct DisplayData final
{
    u32 Width : 16;
    u32 Height : 16;
    u32 BitsPerPixel;
    u32 RefreshRateNumerator : 16;
    u32 RefreshRateDenominator : 16;
    u32 Enable : 1;
    u32 VSyncEnable : 1;
    u32 Pad : 30;
    u64 Framebuffer;
    u32 FramebufferLowTmp;

    DisplayData() noexcept
        : Width(0)
        , Height(0)
        , BitsPerPixel(0)
        , RefreshRateNumerator(0)
        , RefreshRateDenominator(0)
        , Enable(0)
        , VSyncEnable(0)
        , Pad(0)
        , Framebuffer(0)
        , FramebufferLowTmp(0)
    { }

    [[nodiscard]] u32 FramebufferLow()  const noexcept { return static_cast<u32>(Framebuffer);       }
    [[nodiscard]] u32 FramebufferHigh() const noexcept { return static_cast<u32>(Framebuffer >> 32); }
};

//
// EDID block
//
#pragma pack(push, 1)
struct EdidBlock final
{
    DEFAULT_CONSTRUCT_PU(EdidBlock);
    DEFAULT_CM_PU(EdidBlock);
    DEFAULT_DESTRUCT(EdidBlock);

    u8  Header[8];                        //EDID header "00 FF FF FF FF FF FF 00"
    u16 ManufactureName;                  //EISA 3-character ID
    u16 ProductCode;                      //Vendor assigned code
    u32 SerialNumber;                     //32-bit serial number
    u8  WeekOfManufacture;                //Week number
    u8  YearOfManufacture;                //Year
    u8  EdidVersion;                      //EDID Structure Version
    u8  EdidRevision;                     //EDID Structure Revision
    u8  VideoInputDefinition;
    u8  MaxHorizontalImageSize;           //cm
    u8  MaxVerticalImageSize;             //cm
    u8  DisplayTransferCharacteristic;
    u8  FeatureSupport;
    u8  RedGreenLowBits;                  //Rx1 Rx0 Ry1 Ry0 Gx1 Gx0 Gy1Gy0
    u8  BlueWhiteLowBits;                 //Bx1 Bx0 By1 By0 Wx1 Wx0 Wy1 Wy0
    u8  RedX;                             //Red-x Bits 9 - 2
    u8  RedY;                             //Red-y Bits 9 - 2
    u8  GreenX;                           //Green-x Bits 9 - 2
    u8  GreenY;                           //Green-y Bits 9 - 2
    u8  BlueX;                            //Blue-x Bits 9 - 2
    u8  BlueY;                            //Blue-y Bits 9 - 2
    u8  WhiteX;                           //White-x Bits 9 - 2
    u8  WhiteY;                           //White-x Bits 9 - 2
    u8  EstablishedTimings[3];
    u8  StandardTimingIdentification[16];
    u8  DetailedTimingDescriptions[72];
    u8  ExtensionFlag;                    //Number of (optional) 128-byte EDID extension blocks to follow
    u8  Checksum;
};
#pragma pack(pop)

using DisplayUpdateCallback_f = ::std::function<void(u32 displayIndex, const DisplayData& displayData)>;

class Processor;

class DisplayManagerReceiverSample
{
public:
    void ReceiveDisplayManager_Acknowledge(const bool acknowledge) noexcept { }
    void ReceiveDisplayManager_Data(const u32 data) noexcept { }
    void ReceiveDisplayManager_VSyncEvent(const bool event) noexcept { }
};

enum class DisplayRequestPacketType : u32
{
    Edid = 0,
    DisplayInfo = 1
};

template<typename Receiver = DisplayManagerReceiverSample>
class DisplayManager final
{
    DEFAULT_DESTRUCT(DisplayManager);
    DELETE_CM(DisplayManager);
public:
    static constexpr uSys MaxDisplayCount = 8;

    static constexpr u32 REGISTER_WIDTH = 0;
    static constexpr u32 REGISTER_HEIGHT = 1;
    static constexpr u32 REGISTER_BPP = 2;
    static constexpr u32 REGISTER_ENABLE = 3;
    static constexpr u32 REGISTER_REFRESH_RATE_NUMERATOR = 4;
    static constexpr u32 REGISTER_REFRESH_RATE_DENOMINATOR = 5;
    static constexpr u32 REGISTER_VSYNC_ENABLE = 6;
    static constexpr u32 REGISTER_FB_LOW = 7;
    static constexpr u32 REGISTER_FB_HIGH = 8;
private:
    SENSITIVITY_DECL(p_Reset_n, p_Clock);
    STD_LOGIC_DECL(p_RequestActive);

    SIGNAL_ENTITIES();
public:
    explicit DisplayManager(Receiver* const parent) noexcept
        : m_Parent(parent)
        , p_Reset_n(0)
        , p_Clock(0)
        , p_RequestActive(StdLogic::U)
        , p_RequestPacketType(DisplayRequestPacketType::Edid)
        , p_RequestReadWrite(EReadWrite::Read)
        , p_RequestDisplayIndex(0)
        , p_VSyncInterruptPending(0)
        , m_RequestHandled(0)
        , m_Pad0(0)
        , p_RequestRegister(0)
        , p_inout_Data(0)
        , m_DisplaysEdid{ }
        , m_Displays{ }
        , m_VSyncEvents{ }
        , m_UpdateCallback(nullptr)
    {
        for(uSys i = 0; i < ::std::size(m_VSyncEvents); ++i)
        {
            m_VSyncEvents[i] = 0;
        }
    }

    void SetResetN(const bool reset_n) noexcept
    {
        p_Reset_n = BOOL_TO_BIT(reset_n);

        TRIGGER_SENSITIVITY(p_Reset_n);
    }

    void SetClock(const bool clock) noexcept
    {
        p_Clock = BOOL_TO_BIT(clock);

        TRIGGER_SENSITIVITY(p_Clock);
    }

    void SetRequestActive(const StdLogic active) noexcept
    {
        STD_LOGIC_SET(p_RequestActive, active);
    }

    void SetRequestPacketType(const DisplayRequestPacketType packetType) noexcept
    {
        p_RequestPacketType = packetType;
    }

    void SetRequestReadWrite(const EReadWrite readWrite) noexcept
    {
        p_RequestReadWrite = readWrite;
    }

    void SetRequestDisplayIndex(const u32 displayIndex) noexcept
    {
        p_RequestDisplayIndex = displayIndex;
    }

    void SetRequestRegister(const u32 registerIndex) noexcept
    {
        p_RequestRegister = registerIndex;
    }

    void SetRequestData(const u32 data) noexcept
    {
        p_inout_Data = data;
    }

    [[nodiscard]] u32 GetResponseData() const noexcept
    {
        return p_inout_Data;
    }

    // Intended only for VBDevice.
    [[nodiscard]] EdidBlock& GetDisplayEdid(const uSys index) noexcept { return m_DisplaysEdid[index]; }
    [[nodiscard]] DisplayUpdateCallback_f& UpdateCallback() noexcept { return m_UpdateCallback; }

    void NotifyDisplayVSyncEvent(const u32 display) noexcept
    {
        ++m_VSyncEvents[display];
    }
private:
    PROCESSES_DECL()
    {
        STD_LOGIC_PROCESS_RESET_HANDLER(p_Clock);
        PROCESS_ENTER(HandleReset, p_Reset_n)
        PROCESS_ENTER(TriggerInterrupt, p_Reset_n, p_Clock)
        PROCESS_ENTER(HandlePacket, p_Reset_n, p_Clock)
    }

    PROCESS_DECL(HandleReset)
    {
        if(!BIT_TO_BOOL(p_Reset_n))
        {
            for(uSys i = 0; i < ::std::size(m_DisplaysEdid); ++i)
            {
                ::new(&m_DisplaysEdid[i]) EdidBlock();
            }

            for(uSys i = 0; i < ::std::size(m_Displays); ++i)
            {
                ::new(&m_Displays[i]) DisplayData();
            }
        }
    }

    PROCESS_DECL(TriggerInterrupt)
    {
        if(!BIT_TO_BOOL(p_Reset_n))
        {
            for(uSys i = 0; i < ::std::size(m_VSyncEvents); ++i)
            {
                m_VSyncEvents[i] = 0;
            }
        }
        else if(RISING_EDGE(p_Clock))
        {
            if(!BIT_TO_BOOL(p_VSyncInterruptPending))
            {
                // Check to see if any VSync events have been counted.

                // A variable to coalesce all event counts.
                u32 eventCoalesce = 0;

                // Bitwise OR each of the event counters.
                for(uSys i = 0; i < ::std::size(m_VSyncEvents); ++i)
                {
                    eventCoalesce |= m_VSyncEvents[i];
                }

                // Coalesce down to a single bit. Basically checking if eventCoalesce != 0.
                eventCoalesce = (eventCoalesce & 0xFFFF) | (eventCoalesce >> 16);
                eventCoalesce = (eventCoalesce & 0xFF) | (eventCoalesce >> 8);
                eventCoalesce = (eventCoalesce & 0xF) | (eventCoalesce >> 4);
                eventCoalesce = (eventCoalesce & 0x3) | (eventCoalesce >> 2);
                eventCoalesce = (eventCoalesce & 0x1) | (eventCoalesce >> 1);

                if(BIT_TO_BOOL(eventCoalesce))
                {
                    m_Parent->ReceiveDisplayManager_VSyncEvent(true);
                }
            }
            else
            {
                m_Parent->ReceiveDisplayManager_VSyncEvent(false);
            }
        }
    }

    PROCESS_DECL(HandlePacket)
    {
        if(!BIT_TO_BOOL(p_Reset_n))
        {
            p_inout_Data = 0;
        }
        else if(RISING_EDGE(p_Clock))
        {
            if(!LOGIC_TO_BOOL(p_RequestActive))
            {
                m_RequestHandled = BOOL_TO_BIT(false);
                m_Parent->ReceiveDisplayManager_Acknowledge(false);
                return;
            }
            else if(BIT_TO_BOOL(m_RequestHandled))
            {
                return;
            }

            if(p_RequestPacketType == DisplayRequestPacketType::Edid)
            {
                const u32 edidOffset = p_RequestRegister * sizeof(u32);
                u8* const edidPointer = reinterpret_cast<u8*>(&m_DisplaysEdid[p_RequestDisplayIndex]) + edidOffset;

                static_assert(sizeof(u32) == 4, "We are anticipating a u32 to be 4 bytes for this function.");

                if(p_RequestReadWrite == EReadWrite::Read)
                {
                    (void) ::std::memcpy(&p_inout_Data, edidPointer, sizeof(p_inout_Data));
                    m_Parent->ReceiveDisplayManager_Data(p_inout_Data);
                }
                else if(p_RequestReadWrite == EReadWrite::Write)
                {
                    (void) ::std::memcpy(edidPointer, &p_inout_Data, sizeof(p_inout_Data));
                }
            }
            else if(p_RequestPacketType == DisplayRequestPacketType::DisplayInfo)
            {
                if(p_RequestReadWrite == EReadWrite::Read)
                {
                    switch(p_RequestRegister)
                    {
                        case REGISTER_WIDTH:
                            p_inout_Data = m_Displays[p_RequestDisplayIndex].Width;
                            break;
                        case REGISTER_HEIGHT:
                            p_inout_Data = m_Displays[p_RequestDisplayIndex].Height;
                            break;
                        case REGISTER_BPP:
                            p_inout_Data = m_Displays[p_RequestDisplayIndex].BitsPerPixel;
                            break;
                        case REGISTER_ENABLE:
                            p_inout_Data = m_Displays[p_RequestDisplayIndex].Enable;
                            break;
                        case REGISTER_REFRESH_RATE_NUMERATOR:
                            p_inout_Data = m_Displays[p_RequestDisplayIndex].RefreshRateNumerator;
                            break;
                        case REGISTER_REFRESH_RATE_DENOMINATOR:
                            p_inout_Data = m_Displays[p_RequestDisplayIndex].RefreshRateDenominator;
                            break;
                        case REGISTER_VSYNC_ENABLE:
                            p_inout_Data = m_Displays[p_RequestDisplayIndex].VSyncEnable;
                            break;
                        case REGISTER_FB_LOW:
                            p_inout_Data = m_Displays[p_RequestDisplayIndex].FramebufferLow();
                            break;
                        case REGISTER_FB_HIGH:
                            p_inout_Data = m_Displays[p_RequestDisplayIndex].FramebufferHigh();
                            break;
                        default:
                            break;
                    }
                    m_Parent->ReceiveDisplayManager_Data(p_inout_Data);
                }
                else if(p_RequestReadWrite == EReadWrite::Write)
                {
                    switch(p_RequestRegister)
                    {
                        case REGISTER_WIDTH:
                            m_Displays[p_RequestDisplayIndex].Width = p_inout_Data;
                            break;
                        case REGISTER_HEIGHT:
                            m_Displays[p_RequestDisplayIndex].Height = p_inout_Data;
                            break;
                        case REGISTER_BPP:
                            m_Displays[p_RequestDisplayIndex].BitsPerPixel = p_inout_Data;
                            break;
                        case REGISTER_ENABLE:
                            m_Displays[p_RequestDisplayIndex].Enable = p_inout_Data;
                            break;
                        case REGISTER_REFRESH_RATE_NUMERATOR:
                            m_Displays[p_RequestDisplayIndex].RefreshRateNumerator = p_inout_Data;
                            break;
                        case REGISTER_REFRESH_RATE_DENOMINATOR:
                            m_Displays[p_RequestDisplayIndex].RefreshRateDenominator = p_inout_Data;
                            break;
                        case REGISTER_VSYNC_ENABLE:
                            m_Displays[p_RequestDisplayIndex].VSyncEnable = p_inout_Data;
                            if(m_Displays[p_RequestDisplayIndex].VSyncEnable)
                            {
                                ++m_VSyncEvents[p_RequestDisplayIndex];
                            }
                            break;
                        case REGISTER_FB_LOW:
                            m_Displays[p_RequestDisplayIndex].FramebufferLowTmp = p_inout_Data;
                            break;
                        case REGISTER_FB_HIGH:
                        {
                            m_Displays[p_RequestDisplayIndex].Framebuffer = (static_cast<u64>(p_inout_Data) << 32) |
                                m_Displays[p_RequestDisplayIndex].FramebufferLowTmp;
                            break;
                        }
                        default:
                            break;
                    }

                    if(m_UpdateCallback)
                    {
                        m_UpdateCallback(p_RequestDisplayIndex, m_Displays[p_RequestDisplayIndex]);
                    }
                }
            }

            m_RequestHandled = BOOL_TO_BIT(true);
            m_Parent->ReceiveDisplayManager_Acknowledge(true);
        }
    }
private:
    Receiver* m_Parent;

    u32 p_Reset_n : 1;
    u32 p_Clock : 1;
    StdLogic p_RequestActive : 3;
    DisplayRequestPacketType p_RequestPacketType : 1;
    EReadWrite p_RequestReadWrite : 1;
    u32 p_RequestDisplayIndex : 3;
    u32 p_VSyncInterruptPending : 1;
    u32 m_RequestHandled : 1;
    u32 m_Pad0 : 20;
    u32 p_RequestRegister;
    u32 p_inout_Data;

    EdidBlock m_DisplaysEdid[MaxDisplayCount];
    DisplayData m_Displays[MaxDisplayCount];

    /**
     * Counters of the current number of queued VSync events.
     *
     *   The Driver will read the number of queued VSync events, and notify
     * Windows of the appropriate number of interrupts.
     *
     *   This is born of an interest to have parity between the simulation
     * and the hardware. At it's simplest this could just be 3 bits for
     * tracking which display interrupted. The problem is that the
     * simulation may not be able to keep up with VSync events, especially
     * if there are 8 360 Hz displays (which this is designed to handle).
     * Thus, there is a need to count how many events come. We could assert
     * an interrupt for each VSync event, but GPT5 had a better idea:
     * send a single interrupt and the let driver query the number of
     * queued VSync events. It'd been a while since I reviewed
     * DXGKCB_NOTIFY_INTERRUPT and had forgotten that you can call it
     * multiple times from DxgkDdiInterruptRoutine.
     *
     *   This is an atomic because its being triggered from the Virtual Box
     * Device. Thus, there is a thread boundary between us, and we need to
     * use atomics to safely interact with this counter. In hardware this
     * would just be a register. This can also feasibly be much smaller.
     * With a 360 Hz display, a 21-bit integer could track an hours worth
     * of updates. A 16-bit integer could track about 3 minutes. For the
     * simulation 3 minutes might be too small, so we'll stick with a fat
     * 32-bit integer for now. ChatGPT recommended a 64-bit integer, but I
     * see no need for that.
     */
    ::std::atomic_uint32_t m_VSyncEvents[MaxDisplayCount];

    DisplayUpdateCallback_f m_UpdateCallback;
};
