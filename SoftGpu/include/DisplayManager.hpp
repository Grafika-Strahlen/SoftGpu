#pragma once

#include <NumTypes.hpp>
#include <Objects.hpp>
#include <functional>
#include <atomic>

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

struct DisplayDataPacket final
{
    DEFAULT_CONSTRUCT_PU(DisplayDataPacket);
    DEFAULT_DESTRUCT(DisplayDataPacket);
    DEFAULT_CM_PU(DisplayDataPacket);

    u32 BusActive : 1;
    u32 PacketType : 1;
    u32 Read : 1;
    u32 DisplayIndex : 3;
    u32 Pad0 : 25;
    union
    {
        EdidBlock* EdidBusAssign;
        struct
        {
            u32 Register;
            u32* Value;
        };
    };
};

using DisplayUpdateCallback_f = ::std::function<void(u32 displayIndex, const DisplayData& displayData)>;

class Processor;

class DisplayManager final
{
    //DEFAULT_CONSTRUCT_PU(DisplayManager);
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
public:
    DisplayManager(Processor* const processor) noexcept
        : m_Processor(processor)
        , m_DisplaysEdid{ }
        , m_Displays{ }
        , m_CurrentPacket()
        , m_UpdateCallback(nullptr)
        , m_VSyncEvent(0)
    { }

    void Reset() noexcept
    {
        for(uSys i = 0; i < MaxDisplayCount; ++i)
        {
            ::new(&m_Displays[i]) EdidBlock;
        }
    }

    void Clock(bool risingEdge = true) noexcept
    {
        if(!risingEdge)
        {
            HandleVSyncEvent();
        }
        else
        {
            if(!m_CurrentPacket.BusActive)
            {
                return;
            }

            if(m_CurrentPacket.PacketType == 0)
            {
                if(m_CurrentPacket.Read)
                {
                    // Not required in hardware, we just represent the bus with a pointer.
                    if(m_CurrentPacket.EdidBusAssign)
                    {
                        *m_CurrentPacket.EdidBusAssign = m_DisplaysEdid[m_CurrentPacket.DisplayIndex];
                    }
                }
                else
                {
                    // Not required in hardware, we just represent the bus with a pointer.
                    if(m_CurrentPacket.EdidBusAssign)
                    {
                        m_DisplaysEdid[m_CurrentPacket.DisplayIndex] = *m_CurrentPacket.EdidBusAssign;
                    }
                }
            }
            else if(m_CurrentPacket.PacketType == 1)
            {
                // Not required in hardware, we just represent the bus with a pointer.
                if(!m_CurrentPacket.Value)
                {
                    return;
                }

                if(m_CurrentPacket.Read)
                {
                    switch(m_CurrentPacket.Register)
                    {
                        case REGISTER_WIDTH:
                            *m_CurrentPacket.Value = m_Displays[m_CurrentPacket.DisplayIndex].Width;
                            break;
                        case REGISTER_HEIGHT:
                            *m_CurrentPacket.Value = m_Displays[m_CurrentPacket.DisplayIndex].Height;
                            break;
                        case REGISTER_BPP:
                            *m_CurrentPacket.Value = m_Displays[m_CurrentPacket.DisplayIndex].BitsPerPixel;
                            break;
                        case REGISTER_ENABLE:
                            *m_CurrentPacket.Value = m_Displays[m_CurrentPacket.DisplayIndex].Enable;
                            break;
                        case REGISTER_REFRESH_RATE_NUMERATOR:
                            *m_CurrentPacket.Value = m_Displays[m_CurrentPacket.DisplayIndex].RefreshRateNumerator;
                            break;
                        case REGISTER_REFRESH_RATE_DENOMINATOR:
                            *m_CurrentPacket.Value = m_Displays[m_CurrentPacket.DisplayIndex].RefreshRateDenominator;
                            break;
                        case REGISTER_VSYNC_ENABLE:
                            *m_CurrentPacket.Value = m_Displays[m_CurrentPacket.DisplayIndex].VSyncEnable;
                            break;
                        case REGISTER_FB_LOW:
                            *m_CurrentPacket.Value = m_Displays[m_CurrentPacket.DisplayIndex].FramebufferLow();
                            break;
                        case REGISTER_FB_HIGH:
                            *m_CurrentPacket.Value = m_Displays[m_CurrentPacket.DisplayIndex].FramebufferHigh();
                            break;
                        default:
                            break;
                    }
                }
                else
                {
                    switch(m_CurrentPacket.Register)
                    {
                        case REGISTER_WIDTH:
                            m_Displays[m_CurrentPacket.DisplayIndex].Width = *m_CurrentPacket.Value;
                            break;
                        case REGISTER_HEIGHT:
                            m_Displays[m_CurrentPacket.DisplayIndex].Height = *m_CurrentPacket.Value;
                            break;
                        case REGISTER_BPP:
                            m_Displays[m_CurrentPacket.DisplayIndex].BitsPerPixel = *m_CurrentPacket.Value;
                            break;
                        case REGISTER_ENABLE:
                            m_Displays[m_CurrentPacket.DisplayIndex].Enable = *m_CurrentPacket.Value;
                            break;
                        case REGISTER_REFRESH_RATE_NUMERATOR:
                            m_Displays[m_CurrentPacket.DisplayIndex].RefreshRateNumerator = *m_CurrentPacket.Value;
                            break;
                        case REGISTER_REFRESH_RATE_DENOMINATOR:
                            m_Displays[m_CurrentPacket.DisplayIndex].RefreshRateDenominator = *m_CurrentPacket.Value;
                            break;
                        case REGISTER_VSYNC_ENABLE:
                            m_Displays[m_CurrentPacket.DisplayIndex].VSyncEnable = *m_CurrentPacket.Value;
                            if(m_Displays[m_CurrentPacket.DisplayIndex].VSyncEnable)
                            {
                                SetDisplayVSyncEvent(m_CurrentPacket.DisplayIndex);
                            }
                            break;
                        case REGISTER_FB_LOW:
                            m_Displays[m_CurrentPacket.DisplayIndex].FramebufferLowTmp = *m_CurrentPacket.Value;
                            break;
                        case REGISTER_FB_HIGH:
                        {
                            m_Displays[m_CurrentPacket.DisplayIndex].Framebuffer = (static_cast<u64>(*m_CurrentPacket.Value) << 32) |
                                m_Displays[m_CurrentPacket.DisplayIndex].FramebufferLowTmp;
                            break;
                        }
                        default:
                            break;
                    }

                    if(m_UpdateCallback)
                    {
                        m_UpdateCallback(m_CurrentPacket.DisplayIndex, m_Displays[m_CurrentPacket.DisplayIndex]);
                    }
                }
            }
        }
    }

    void SetBus(const DisplayDataPacket& bus) noexcept
    {
        m_CurrentPacket = bus;
    }

    void ResetBus() noexcept
    {
        m_CurrentPacket.BusActive = 0;
        m_CurrentPacket.PacketType = 0;
        m_CurrentPacket.Read = 0;
        m_CurrentPacket.DisplayIndex = 0;
        m_CurrentPacket.EdidBusAssign = nullptr;
        m_CurrentPacket.Register = 0;
        m_CurrentPacket.Value = nullptr;
    }

    // Intended only for VBDevice.
    [[nodiscard]] EdidBlock& GetDisplayEdid(const uSys index) noexcept { return m_DisplaysEdid[index]; }
    [[nodiscard]] DisplayUpdateCallback_f& UpdateCallback() noexcept { return m_UpdateCallback; }

    void SetDisplayVSyncEvent(const u32 display) noexcept
    {
        m_VSyncEvent = display + 1;
    }
private:
    void HandleVSyncEvent() noexcept;
private:
    Processor* m_Processor;
    EdidBlock m_DisplaysEdid[MaxDisplayCount];
    DisplayData m_Displays[MaxDisplayCount];
    DisplayDataPacket m_CurrentPacket;

    DisplayUpdateCallback_f m_UpdateCallback;
    ::std::atomic_uint32_t m_VSyncEvent;
};
