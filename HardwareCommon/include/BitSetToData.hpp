#pragma once

#include <BitSet.hpp>

template<typename T>
void CopyFromBitSet(const BitSetC<sizeof(T) * CHAR_BIT>& bitSet, T& t) noexcept
{
    uSys wordBlock[BitSetC<sizeof(T) * CHAR_BIT>::WORD_COUNT];
    bitSet.CopyToRaw(wordBlock);

    (void) ::std::memcpy(&t, wordBlock, sizeof(T));
}

template<typename T>
T CopyFromBitSet(const BitSetC<sizeof(T)* CHAR_BIT>& bitSet) noexcept
{
    T ret;
    CopyFromBitSet(bitSet, ret);

    return ret;
}

template<typename T>
void CopyToBitSet(T& t, BitSetC<sizeof(T)* CHAR_BIT>& bitSet) noexcept
{
    uSys wordBlock[BitSetC<sizeof(T) * CHAR_BIT>::WORD_COUNT];
    (void) ::std::memcpy(wordBlock, &t, sizeof(T));

    bitSet.CopyFromRaw(wordBlock);
}

template<typename T>
BitSetC<sizeof(T)* CHAR_BIT> CopyToBitSet(T& t) noexcept
{
    BitSetC<sizeof(T)* CHAR_BIT> ret;
    CopyToBitSet(t, ret);

    return ret;
}
