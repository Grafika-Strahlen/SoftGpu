#pragma once

#include <climits>
#include <cstring>
#include <NumTypes.hpp>
#include <Objects.hpp>
#include <TUMaths.hpp>

template<uSys ElementCountT, uSys TBitCountT = 1, typename T = bool>
class TBitVector final
{
    DEFAULT_DESTRUCT(TBitVector);
    DEFAULT_CM_PU(TBitVector);
public:
    static constexpr uSys BIT_COUNT = sizeof(uSys) * CHAR_BIT;
    static constexpr uSys WORD_COUNT = DivCeil<uSys>(ElementCountT * TBitCountT, BIT_COUNT);

    static constexpr uSys ELEMENT_COUNT = ElementCountT;
    static constexpr uSys T_BIT_COUNT = TBitCountT;
    using MappedType = T;

    static constexpr uSys T_MASK = (TBitCountT << 1) - 1;
public:
    class BitVectorRef final
    {
        DEFAULT_DESTRUCT(BitVectorRef);
    private:
        TBitVector& m_Vector;
        uSys m_Index;
    public:
        BitVectorRef(TBitVector& m_Vector, const uSys index) noexcept
            : m_Vector(m_Vector)
            , m_Index(index)
        { }

        BitVectorRef(const BitVectorRef& copy) noexcept = default;
        BitVectorRef(BitVectorRef&& move) noexcept = default;

        BitVectorRef& operator=(const BitVectorRef& copy) noexcept
        {
            this->operator=(static_cast<T>(copy));

            return *this;
        }

        BitVectorRef& operator=(BitVectorRef&& move) noexcept
        {
            this->operator=(static_cast<T>(move));

            return *this;
        }

        [[nodiscard]] operator T() const noexcept
        { return m_Vector[m_Index]; }

        BitVectorRef& operator=(const T val) noexcept
        {
            m_Vector.Set(m_Index, val);

            return *this;
        }
    };
public:
    TBitVector() noexcept
        : m_Bits()
    {
        (void) ::std::memset(m_Bits, 0x00, sizeof(m_Bits));
    }

    [[nodiscard]] T operator[](const uSys index) const noexcept
    {
        const uSys chunkIndexHigh = (index + 1) * T_BIT_COUNT;
        const uSys chunkIndexLow = index * T_BIT_COUNT;

        const uSys wordIndexHigh = chunkIndexHigh / BIT_COUNT;
        const uSys bitIndexHigh = chunkIndexHigh % BIT_COUNT;
        const uSys wordIndexLow = chunkIndexLow / BIT_COUNT;
        const uSys bitIndexLow = chunkIndexLow % BIT_COUNT;

        if(wordIndexHigh == wordIndexLow)
        {
            const uSys offsetMask = T_MASK << bitIndexLow;

            return static_cast<T>((m_Bits[wordIndexLow] & offsetMask) >> bitIndexLow);
        }
        else
        {
            const uSys offsetMaskLow = T_MASK << bitIndexLow;
            const uSys offsetMaskHigh = T_MASK >> (T_BIT_COUNT - bitIndexHigh);

            uSys value = (m_Bits[wordIndexLow] & offsetMaskLow) >> bitIndexLow;
            value |= (m_Bits[wordIndexHigh] & offsetMaskHigh) << (T_BIT_COUNT - bitIndexHigh);

            return value;
        }
    }

    [[nodiscard]] BitVectorRef operator[](const uSys index) noexcept
    {
        return BitVectorRef(*this, index);
    }

    [[nodiscard]] T At(const uSys index) const noexcept
    {
        const uSys chunkIndexHigh = (index + 1) * T_BIT_COUNT;
        const uSys chunkIndexLow = index * T_BIT_COUNT;

        const uSys wordIndexHigh = chunkIndexHigh / BIT_COUNT;
        const uSys bitIndexHigh = chunkIndexHigh % BIT_COUNT;
        const uSys wordIndexLow = chunkIndexLow / BIT_COUNT;
        const uSys bitIndexLow = chunkIndexLow % BIT_COUNT;

        if(wordIndexHigh >= WORD_COUNT)
        { return {}; }

        if(wordIndexLow >= WORD_COUNT)
        { return {}; }

        if(wordIndexHigh == wordIndexLow)
        {
            const uSys offsetMask = T_MASK << bitIndexLow;

            return static_cast<T>((m_Bits[wordIndexLow] & offsetMask) >> bitIndexLow);
        }
        else
        {
            const uSys offsetMaskLow = T_MASK << bitIndexLow;
            const uSys offsetMaskHigh = T_MASK >> (T_BIT_COUNT - bitIndexHigh);

            uSys value = (m_Bits[wordIndexLow] & offsetMaskLow) >> bitIndexLow;
            value |= (m_Bits[wordIndexHigh] & offsetMaskHigh) << (T_BIT_COUNT - bitIndexHigh);

            return value;
        }
    }

    void Set(const uSys index, const T value) noexcept
    {
        const uSys chunkIndexHigh = (index + 1) * T_BIT_COUNT;
        const uSys chunkIndexLow = index * T_BIT_COUNT;

        const uSys wordIndexHigh = chunkIndexHigh / BIT_COUNT;
        const uSys bitIndexHigh = chunkIndexHigh % BIT_COUNT;
        const uSys wordIndexLow = chunkIndexLow / BIT_COUNT;
        const uSys bitIndexLow = chunkIndexLow % BIT_COUNT;

        if(wordIndexHigh >= WORD_COUNT)
        { return; }

        if(wordIndexLow >= WORD_COUNT)
        { return; }

        if(wordIndexHigh == wordIndexLow)
        {
            const uSys offsetMask = T_MASK << bitIndexLow;

            m_Bits[wordIndexLow] = ((static_cast<uSys>(value) << bitIndexLow) & offsetMask) | (m_Bits[wordIndexLow] & ~offsetMask);
        }
        else
        {
            const uSys offsetMaskLow = T_MASK << bitIndexLow;
            const uSys offsetMaskHigh = T_MASK >> (T_BIT_COUNT - bitIndexHigh);

            m_Bits[wordIndexLow] = ((static_cast<uSys>(value) << bitIndexLow) & offsetMaskLow) | (m_Bits[wordIndexLow] & ~offsetMaskLow);
            m_Bits[wordIndexHigh] = ((static_cast<uSys>(value) >> (T_BIT_COUNT - bitIndexHigh)) & offsetMaskHigh) | (m_Bits[wordIndexHigh] & ~offsetMaskHigh);
        }
    }

    template<uSys HighInclusiveT, uSys LowInclusiveT = 0>
    auto Sub() const noexcept
    {
        TBitVector<HighInclusiveT - LowInclusiveT + 1, TBitCountT, T> ret;
        for(uSys r = LowInclusiveT, w = 0; r <= HighInclusiveT; ++r, ++w)
        {
            ret.Set(w, At(r));
        }
        return ret;
    }

    template<uSys ElementCountLowT>
    auto Concat(const TBitVector<ElementCountLowT, TBitCountT, T>& low);

    void CopyToRaw(uSys* raw) const noexcept
    {
        (void) ::std::memcpy(raw, m_Bits, sizeof(m_Bits));
    }

    void CopyFromRaw(const uSys* raw) noexcept
    {
        (void) ::std::memcpy(m_Bits, raw, sizeof(m_Bits));
    }
private:
    uSys m_Bits[WORD_COUNT];
};

template<uSys ElementCountHighT, uSys ElementCountLowT, uSys TBitCountT = 1, typename T = bool>
static auto Merge(const TBitVector<ElementCountHighT, TBitCountT, T>& high, const TBitVector<ElementCountLowT, TBitCountT, T>& low) noexcept
{
    TBitVector<ElementCountHighT + ElementCountLowT, TBitCountT, T> ret;

    for(uSys i = 0; i < ElementCountLowT; ++i)
    {
        ret.Set(i, low.Get(i));
    }

    for(uSys i = 0; i < ElementCountHighT; ++i)
    {
        ret.Set(i + ElementCountLowT, low.Get(i));
    }

    return ret;
}

template<uSys ElementCountT, uSys TBitCountT, typename T>
template<uSys ElementCountLowT>
auto TBitVector<ElementCountT, TBitCountT, T>::Concat(const TBitVector<ElementCountLowT, TBitCountT, T>& low)
{
    return Merge<ElementCountT, ElementCountLowT, TBitCountT, T>(*this, low);
}
