#pragma once

#include <Objects.hpp>
#include <NumTypes.hpp>

#include <cstring>

#ifndef UNIT_TEST
  #define UNIT_TEST 0
#endif

struct InputSlotData final
{
    // The pointer to the buffer holding the data. This is includes the necessary offset. This a 1 byte granularity
    u64 BufferPtr;
    // The size of the individual components. A component would a single float from a vector, etc.
    u8 ComponentSize;
    // The total number of components.
    u8 ComponentCount;
    // The stride from the beginning of a block of data to the next block of data.
    u32 Stride;
};

class GraphicsPipeline;

class InputAssembler final
{
    DEFAULT_DESTRUCT(InputAssembler);
    DELETE_CM(InputAssembler);
public:
    InputAssembler(GraphicsPipeline* const graphicsPipeline) noexcept
        : m_GraphicsPipeline(graphicsPipeline)
        , m_EnabledInputCount(0)
        , m_AssembledInputWordCount(0)
        , m_InputLayout{ }
    { }

    void LoadInputLayout0(const u8 enabledInputCount, const u32 assembledInputWordCount) noexcept
    {
        m_EnabledInputCount = enabledInputCount;
        m_AssembledInputWordCount = assembledInputWordCount;
    }

    void LoadInputLayout1(const u32 slot, const InputSlotData data) noexcept
    {
        (void) ::std::memcpy(&m_InputLayout[slot], &data, sizeof(data));
    }

    void AssembleInput(u32 index, u32 writeAddress) noexcept;
public:
#if UNIT_TEST
    // Tests to see if the Input Assembler is able to load 1, 2, 4, 8 byte sized inputs from offsets 0, 1, 2, 3 of a word.
    static void TestLayoutDecoding() noexcept;
#endif
private:
    GraphicsPipeline* m_GraphicsPipeline;

    // The number of m_InputLayout elements that are active, this uses 1 based indexing.
    u8 m_EnabledInputCount : 4;
    // The number of 32 bit words required to represent the data.
    u32 m_AssembledInputWordCount;
    InputSlotData m_InputLayout[16];
};
