/**
 * @file
 *
 * Copyright (c) 2025. Grafika Strahlen LLC
 * All rights reserved.
 */
#include "InputAssembler.hpp"
#include "GraphicsPipeline.hpp"

#include <cstring>

#if UNIT_TEST
#include <ConPrinter.hpp>

  #define STORE_IA_WORD(VALUE, ADDRESS) StoreIaWordTest(VALUE, ADDRESS)
  #define READ_MEM(ADDRESS) ReadMemTest(ADDRESS)
#else
  #define STORE_IA_WORD(VALUE, ADDRESS) m_GraphicsPipeline->StoreIAWord(VALUE, ADDRESS)
  #define READ_MEM(ADDRESS) m_GraphicsPipeline->Read(ADDRESS)
#endif

#if UNIT_TEST
static void StoreIaWordTest(u32 value, u32 address);
static u32 ReadMemTest(u64 address);
#endif

void InputAssembler::AssembleInput(const u32 index, u32 writeAddress) noexcept
{
    for(u32 i = 0; i < m_EnabledInputCount; ++i)
    {
        u64 inputAddress = m_InputLayout[i].BufferPtr + (static_cast<u64>(m_InputLayout[i].Stride) * index);

        for(u32 j = 0; j < m_InputLayout[i].ComponentCount; ++j)
        {
            const u64 wordOffset = inputAddress & 0x3;
            const u64 wordAddress = inputAddress >> 2;

            switch(m_InputLayout[i].ComponentSize)
            {
                case 1:
                {
                    const u32 word = READ_MEM(wordAddress);

                    u8 bytes[sizeof(word)];
                    (void) ::std::memcpy(bytes, &word, sizeof(word));

                    STORE_IA_WORD(bytes[wordOffset], writeAddress);
                    ++writeAddress;
                    break;
                }
                case 2:
                {
                    const u32 word = READ_MEM(wordAddress);

                    u8 bytes[sizeof(word)];
                    (void) ::std::memcpy(bytes, &word, sizeof(word));

                    if(wordOffset == 3)
                    {
                        const u32 secondWord = READ_MEM(wordAddress + 1);

                        u8 secondBytes[sizeof(secondWord)];
                        (void) ::std::memcpy(secondBytes, &secondWord, sizeof(secondWord));

                        const u32 value = (static_cast<u16>(secondBytes[0]) << 8u) | bytes[3];

                        STORE_IA_WORD(value, writeAddress);
                    }
                    else
                    {
                        const u32 value = (static_cast<u16>(bytes[wordOffset + 1]) << 8u) | bytes[wordOffset];

                        STORE_IA_WORD(value, writeAddress);
                    }
                    
                    ++writeAddress;
                    break;
                }
                case 4:
                {
                    const u32 word = READ_MEM(wordAddress);

                    if(wordOffset == 0)
                    {
                        STORE_IA_WORD(word, writeAddress);
                        ++writeAddress;
                        break;
                    }

                    const u32 secondWord = READ_MEM(wordAddress + 1);

                    u8 bytes[sizeof(word) * 2];
                    (void) ::std::memcpy(bytes, &word, sizeof(word));
                    (void) ::std::memcpy(bytes + sizeof(word), &secondWord, sizeof(secondWord));

                    u32 value;
                    (void) ::std::memcpy(&value, bytes + wordOffset, sizeof(value));
                    STORE_IA_WORD(value, writeAddress);
                        
                    ++writeAddress;
                    break;
                }
                case 8:
                {
                    const u32 word = READ_MEM(wordAddress);
                    const u32 secondWord = READ_MEM(wordAddress + 1);

                    if(wordOffset == 0)
                    {
                        STORE_IA_WORD(word, writeAddress);
                        STORE_IA_WORD(secondWord, writeAddress + 1);
                        writeAddress += 2;
                        break;
                    }

                    const u32 thirdWord = READ_MEM(wordAddress + 2);

                    u8 bytes[sizeof(word) * 3];
                    (void) ::std::memcpy(bytes, &word, sizeof(word));
                    (void) ::std::memcpy(bytes + sizeof(word), &secondWord, sizeof(secondWord));
                    (void) ::std::memcpy(bytes + sizeof(word) * 2, &thirdWord, sizeof(thirdWord));

                    u32 value0;
                    (void) ::std::memcpy(&value0, bytes + wordOffset, sizeof(value0));
                    u32 value1;
                    (void) ::std::memcpy(&value1, bytes + wordOffset + sizeof(value0), sizeof(value1));
                    STORE_IA_WORD(value0, writeAddress);
                    STORE_IA_WORD(value1, writeAddress + 1);

                    writeAddress += 2;
                    break;
                }
                default:
                    break;
            }


            inputAddress += m_InputLayout[i].ComponentSize;
        }

    }
}

#if UNIT_TEST
static u32 TestWordBuf[128];

static void StoreIaWordTest(const u32 value, const u32 address)
{
    TestWordBuf[address] = value;
}

static u32 ReadMemTest(const u64 address)
{
    return *reinterpret_cast<u32*>(static_cast<uPtr>(address << 2));  // NOLINT(performance-no-int-to-ptr)
}

void InputAssembler::TestLayoutDecoding() noexcept
{
    InputAssembler ia(nullptr);

    ia.LoadInputLayout0(4, 5);

    const u8 mem[] = {
        0x12,
        0x34,
        0x56,
        0x78,
        0x41, 0x31, 0xCD,
        0x26, 0x59, 0xCD,
        0x58, 0x53, 0xCD,
        0x93, 0x97, 0xCD,
        0x18, 0x28, 0x18, 0x27, 0xCD,
        0x45, 0x90, 0x45, 0x28, 0xCD,
        0x28, 0x60, 0x53, 0x23, 0xCD,
        0x26, 0x35, 0x71, 0x74, 0xCD,
        0xDF, 0x9B, 0x57, 0x13, 0xE0, 0xAC, 0x68, 0x24, 0xCD,
        0x04, 0x03, 0x02, 0x01, 0x08, 0x07, 0x06, 0x05, 0xCD,
        0x14, 0x13, 0x12, 0x11, 0x18, 0x17, 0x16, 0x15, 0xCD,
        0x24, 0x23, 0x22, 0x21, 0x28, 0x27, 0x26, 0x25, 0xCD,
    };

    InputSlotData slotData;
    slotData.BufferPtr = static_cast<u64>(reinterpret_cast<uPtr>(&mem[0]));
    slotData.ComponentSize = 1;
    slotData.ComponentCount = 1;
    slotData.Stride = 1;

    ia.LoadInputLayout1(0, slotData);

    slotData.BufferPtr = static_cast<u64>(reinterpret_cast<uPtr>(&mem[4]));
    slotData.ComponentSize = 2;
    slotData.Stride = 3;

    ia.LoadInputLayout1(1, slotData);

    slotData.BufferPtr = static_cast<u64>(reinterpret_cast<uPtr>(&mem[16]));
    slotData.ComponentSize = 4;
    slotData.Stride = 5;

    ia.LoadInputLayout1(2, slotData);

    slotData.BufferPtr = static_cast<u64>(reinterpret_cast<uPtr>(&mem[36]));
    slotData.ComponentSize = 8;
    slotData.Stride = 9;

    ia.LoadInputLayout1(3, slotData);

    ia.AssembleInput(0, 0);
    ia.AssembleInput(1, 5);
    ia.AssembleInput(2, 10);
    ia.AssembleInput(3, 15);

    for(u32 i = 0; i < 20; i += 5)
    {
        ConPrinter::Print("0x{XP} 0x{XP} 0x{XP} 0x{XP} 0x{XP}\n", TestWordBuf[i + 0], TestWordBuf[i + 1], TestWordBuf[i + 2], TestWordBuf[i + 3], TestWordBuf[i + 4]);
    }
}
#endif
