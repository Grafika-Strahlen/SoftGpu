/**
 * @file
 *
 * Copyright (c) 2025. Grafika Strahlen LLC
 * All rights reserved.
 */
#pragma once

#include <Objects.hpp>
#include <NumTypes.hpp>
#include <ConPrinter.hpp>

#include "RegisterFile.hpp"

/**
 * \brief A buddy allocator for registers.
 *
 *   Blocks of registers are stored in 2048, 1536, 1024, 768, 512, and
 * 256 chunks. These chunks used fixed offsets making it trivial to
 * search for a merge target.
 *
 * The Block groupings can be represented as the following table:
 *   Each number represents the multiple of 256 the block has.
 *   Blocks oscillate between two lines to show the packing.
 *       The index of a block is based on the offset from the start,
 *     thus blocks that oscillate between two lines do not have
 *     sequential indices (except for the single 256 blocks).
 *   The indices of each block are listed outside the table.
 *
 * .--------.
 * |88888888| 0
 * |        |
 * |        |
 * |666666  | 0
 * |        |
 * |        |
 * | 666666 | 1
 * |        |
 * |        |
 * |  666666| 2
 * |        |
 * |        |
 * |4444    | 0
 * |    4444|  4
 * |        |
 * | 4444   | 1
 * |        |
 * |        |
 * |  4444  | 2
 * |        |
 * |        |
 * |   4444 | 3
 * |        |
 * |        |
 * |333     | 0
 * |   333  |  3
 * |        |
 * | 333    | 1
 * |    333 |  4
 * |        |
 * |  333   | 2
 * |     333|  5
 * |        |
 * |22  22  | 0 4
 * |  22  22|  2 6
 * |        |
 * | 22  22 | 1 5
 * |   22   |  3
 * |        |
 * |        |
 * |1 1 1 1 | 0 2 4 6
 * | 1 1 1 1|  1 3 5 7
 * *--------*
 */
class RegisterAllocator final
{
    DEFAULT_DESTRUCT(RegisterAllocator);
    DELETE_CM(RegisterAllocator);
private:
    struct RegisterBlockPair final
    {
        u16 Available : 1;
        u16 PrevFreeSub : 4;
        u16 PrevFreeSup : 1;
        u16 PrevFreeValid : 1;
        u16 NextFreeSub : 4;
        u16 NextFreeSup : 1;
        u16 NextFreeValid : 1;
        u16 Pad : 3;
    };

    union SlabIndex final
    {
        struct
        {
            u8 SubBlock : 3;
            u8 Block2048 : 1;
            u8 Valid : 1;
            u8 Pad : 3;
        } Parts;
        u8 CombinedIndex;
    };

    enum class SlabSize : u8
    {
        S2048,
        S1536,
        S1024,
        S768,
        S512,
        S256,
    };

    static inline constexpr u32 INVALID_INDEX_PTR_VALUE = 0x10;
    static inline constexpr u32 INVALID_LIST_HEAD_VALUE = 0x8000;
public:
    RegisterAllocator() noexcept
        : m_2048Blocks{ }
        , m_1536Blocks{ }
        , m_1024Blocks{ }
        , m_768Blocks{ }
        , m_512Blocks{ }
        , m_256Blocks{ }
        , m_2048Head{ .Parts = { 0, 0, true, 0 } }
        , m_1536Head{ .Parts = { 0, 0, false, 0 } }
        , m_1024Head{ .Parts = { 0, 0, false, 0 } }
        , m_768Head{ .Parts = { 0, 0, false, 0 } }
        , m_512Head{ .Parts = { 0, 0, false, 0 } }
        , m_256Head{ .Parts = { 0, 0, false, 0 } }
    {
        m_2048Blocks[0][0].Available = true;
        m_2048Blocks[0][0].NextFreeSub = 0;
        m_2048Blocks[0][0].NextFreeSup = 1;
        m_2048Blocks[0][0].PrevFreeSub = 0;
        m_2048Blocks[0][0].PrevFreeSup = 0;
        m_2048Blocks[0][0].Pad = 0;
        m_2048Blocks[1][0].Available = true;
        m_2048Blocks[1][0].NextFreeSub = 0;
        m_2048Blocks[1][0].NextFreeSup = 0;
        m_2048Blocks[1][0].PrevFreeSub = 0;
        m_2048Blocks[1][0].PrevFreeSup = 0;
        m_2048Blocks[1][0].Pad = 0;
    }

    void Reset() noexcept
    {
        m_2048Blocks[0][0].Available = true;
        m_2048Blocks[0][0].NextFreeSub = 0;
        m_2048Blocks[0][0].NextFreeSup = 1;
        m_2048Blocks[0][0].PrevFreeSub = 0;
        m_2048Blocks[0][0].PrevFreeSup = 0;
        m_2048Blocks[0][0].Pad = 0;
        m_2048Blocks[1][0].Available = true;
        m_2048Blocks[1][0].NextFreeSub = 0;
        m_2048Blocks[1][0].NextFreeSup = 0;
        m_2048Blocks[1][0].PrevFreeSub = 0;
        m_2048Blocks[1][0].PrevFreeSup = 0;
        m_2048Blocks[1][0].Pad = 0;

        (void) ::std::memset(m_1536Blocks, 0, sizeof(m_1536Blocks));
        (void) ::std::memset(m_1024Blocks, 0, sizeof(m_1024Blocks));
        (void) ::std::memset(m_768Blocks, 0, sizeof(m_768Blocks));
        (void) ::std::memset(m_512Blocks, 0, sizeof(m_512Blocks));
        (void) ::std::memset(m_256Blocks, 0, sizeof(m_256Blocks));

        m_2048Head.Parts.SubBlock = 0;
        m_2048Head.Parts.Block2048 = 0;
        m_2048Head.Parts.Valid = true;
        m_2048Head.Parts.Pad = 0;
        m_1536Head.CombinedIndex = 0;
        m_1024Head.CombinedIndex = 0;
        m_768Head.CombinedIndex = 0;
        m_512Head.CombinedIndex = 0;
        m_256Head.CombinedIndex = 0;
    }

    // Register Count uses 1 based indexing.
    [[nodiscard]] u16 AllocateRegisterBlock(const u16 registerCount) noexcept
    {
        if(registerCount < 256)
        {
            if(m_256Head.Parts.Valid)
            {
                const u16 ret = GetRegisterFromSlab(SlabSize::S512, m_256Head);
                RedistributeSlabs(SlabSize::S256, m_256Head, ComputeRegisterCount(registerCount));

                return ret;
            }
        }

        if(registerCount < 512)
        {
            if(m_512Head.Parts.Valid)
            {
                const u16 ret = GetRegisterFromSlab(SlabSize::S512, m_512Head);
                RedistributeSlabs(SlabSize::S512, m_512Head, ComputeRegisterCount(registerCount));

                return ret;
            }
        }

        if(registerCount < 768)
        {
            if(m_768Head.Parts.Valid)
            {
                const u16 ret = GetRegisterFromSlab(SlabSize::S768, m_768Head);
                RedistributeSlabs(SlabSize::S768, m_768Head, ComputeRegisterCount(registerCount));

                return ret;
            }
        }

        if(registerCount < 1024)
        {
            if(m_1024Head.Parts.Valid)
            {
                const u16 ret = GetRegisterFromSlab(SlabSize::S1024, m_1024Head);
                RedistributeSlabs(SlabSize::S1024, m_1024Head, ComputeRegisterCount(registerCount));

                return ret;
            }
        }

        if(registerCount < 1536)
        {
            if(m_1536Head.Parts.Valid)
            {
                const u16 ret = GetRegisterFromSlab(SlabSize::S1536, m_1536Head);
                RedistributeSlabs(SlabSize::S1536, m_1536Head, ComputeRegisterCount(registerCount));

                return ret;
            }
        }
        
        if(m_2048Head.Parts.Valid)
        {
            const u16 ret = GetRegisterFromSlab(SlabSize::S2048, m_2048Head);
            RedistributeSlabs(SlabSize::S2048, m_2048Head, ComputeRegisterCount(registerCount));

            return ret;
        }

        return 0xFFFF;
    }

    // Register count uses 1 based indexing.
    void FreeRegisterBlock(const u16 registerBase, const u16 registerCount) noexcept
    {
        switch(ComputeRegisterCount(registerCount))
        {
            case SlabSize::S2048:
            {
                const SlabIndex slabIndex = GetSlabFromRegister(registerBase);
                Insert2048(slabIndex);
                break;
            }
            case SlabSize::S1536:
                Free1536(registerBase);
                break;
            case SlabSize::S1024:
                Free1024(registerBase);
                break;
            case SlabSize::S768:
                Free768(registerBase);
                break;
            case SlabSize::S512:
                Free512(registerBase);
                break;
            case SlabSize::S256:
                Free256(registerBase);
                break;
            default: break;
        }
    }

    bool CheckFree() noexcept
    {
        if(!m_2048Blocks[0][0].Available || !m_2048Blocks[1][0].Available)
        {
            return false;
        }

        if(!m_2048Head.Parts.Valid)
        {
            return false;
        }

        if(m_1536Head.Parts.Valid || m_1024Head.Parts.Valid || m_768Head.Parts.Valid || m_512Head.Parts.Valid || m_256Head.Parts.Valid)
        {
            return false;
        }

        return true;
    }
private:
    [[nodiscard]] static SlabSize ComputeRegisterCount(const u16 targetRegisterCount) noexcept
    {
        if(targetRegisterCount >= 1536)
        {
            return SlabSize::S2048;
        }
        else if(targetRegisterCount >= 1024)
        {
            return SlabSize::S1536;
        }
        else if(targetRegisterCount >= 768)
        {
            return SlabSize::S1024;
        }
        else if(targetRegisterCount >= 512)
        {
            return SlabSize::S768;
        }
        else if(targetRegisterCount >= 256)
        {
            return SlabSize::S512;
        }
        else
        {
            return SlabSize::S256;
        }
    }

    [[nodiscard]] static u16 GetRegisterFromSlab(const SlabSize slab, const SlabIndex index) noexcept
    {
        if(slab == SlabSize::S2048)
        {
            // Blocks2048 * 2048
            return static_cast<u16>(index.Parts.Block2048 << 11);
        }

        if(index.Parts.Block2048 != 0)
        {
            return static_cast<u16>((index.Parts.SubBlock << 8) + index.Parts.Block2048 * 2048);
        }

        // SubBlock * 256
        return static_cast<u16>(index.Parts.SubBlock << 8);
    }

    [[nodiscard]] static SlabIndex GetSlabFromRegister(const u16 registerBase) noexcept
    {
        if(registerBase >= 2048)
        {
            //   The first sub expression wil find the index within the 2048 block,
            // the second subexpression will it into the correct 2048 block
            return {
                .Parts = {
                    // ((registerBase % 2048) / 256)
                    .SubBlock = static_cast<u8>((registerBase & 0x7FF) >> 8),
                    // (registerBase / 2048)
                    // This is technically just 1 currently.
                    .Block2048 = static_cast<u8>(registerBase >> 11),
                    .Valid = false,
                    .Pad = 0
                }
            };
        }

        // registerBase / 256
        return {
            .Parts = {
                .SubBlock = static_cast<u8>(registerBase >> 8),
                .Block2048 = 0,
                .Pad = 0
            }
        };
    }

    [[nodiscard]] static u16 GetSlabSize(const SlabSize slab) noexcept
    {
        switch(slab)
        {
            case SlabSize::S2048: return 2048;
            case SlabSize::S1536: return 1536;
            case SlabSize::S1024: return 1024;
            case SlabSize::S768:  return 768;
            case SlabSize::S512:  return 512;
            case SlabSize::S256:  return 256;
            default: return 0;
        }
    }

    /**
     * \brief Removes a block from a list. Presumably VHDL/Verilog functions allow for mutating input arrays.
     * \tparam SupCount The number of 2048 register columns supported. This should always be 2.
     * \tparam SubCount The number of sub blocks supported.
     * \param head The Head pointer for this size block.
     * \param blockList The List array for this size block.
     * \param slabIndex The index to remove.
     */
    template<uSys SupCount, uSys SubCount>
    static void RemoveBlock(SlabIndex& head, RegisterBlockPair(&blockList)[SupCount][SubCount], const SlabIndex slabIndex) noexcept
    {
        blockList[slabIndex.Parts.Block2048][slabIndex.Parts.SubBlock].Available = false;
        const RegisterBlockPair block = blockList[slabIndex.Parts.Block2048][slabIndex.Parts.SubBlock];

        if(slabIndex.CombinedIndex == head.CombinedIndex)
        {
            head.Parts.Block2048 = block.NextFreeSup;
            head.Parts.SubBlock = block.NextFreeSub;
            head.Parts.Valid = blockList[block.NextFreeSup][block.NextFreeSub].Available;
        }
        else if(block.PrevFreeValid)
        {
            blockList[block.PrevFreeSup][block.PrevFreeSub].NextFreeSup = block.NextFreeSup;
            blockList[block.PrevFreeSup][block.PrevFreeSub].NextFreeSub = block.NextFreeSub;
            blockList[block.PrevFreeSup][block.PrevFreeSub].NextFreeValid = block.NextFreeValid;
        }

        if(block.NextFreeValid)
        {
            blockList[block.NextFreeSup][block.NextFreeSub].PrevFreeSup = block.PrevFreeSup;
            blockList[block.NextFreeSup][block.NextFreeSub].PrevFreeSub = block.PrevFreeSub;
            blockList[block.NextFreeSup][block.NextFreeSub].PrevFreeValid = block.PrevFreeValid;
        }
    }

    /**
     * \brief Insert a block into the head of a list. Presumably VHDL/Verilog functions allow for mutating input arrays.
     * \tparam SupCount The number of 2048 register columns supported. This should always be 2.
     * \tparam SubCount The number of sub blocks supported.
     * \param head The Head pointer for this size block.
     * \param blockList The List array for this size block.
     * \param slabIndex The index to insert.
     */
    template<uSys SupCount, uSys SubCount>
    static void InsertBlock(SlabIndex& head, RegisterBlockPair(&blockList)[SupCount][SubCount], const SlabIndex slabIndex) noexcept
    {
        blockList[slabIndex.Parts.Block2048][slabIndex.Parts.SubBlock].Available = true;
        blockList[slabIndex.Parts.Block2048][slabIndex.Parts.SubBlock].NextFreeSup = head.Parts.Block2048;
        blockList[slabIndex.Parts.Block2048][slabIndex.Parts.SubBlock].NextFreeSub = head.Parts.SubBlock;
        blockList[slabIndex.Parts.Block2048][slabIndex.Parts.SubBlock].NextFreeValid = head.Parts.Valid;
        blockList[slabIndex.Parts.Block2048][slabIndex.Parts.SubBlock].PrevFreeValid = false;
        blockList[head.Parts.Block2048][head.Parts.SubBlock].PrevFreeSup = slabIndex.Parts.Block2048;
        blockList[head.Parts.Block2048][head.Parts.SubBlock].PrevFreeSub = slabIndex.Parts.SubBlock;
        blockList[head.Parts.Block2048][head.Parts.SubBlock].PrevFreeValid = true;
        head.Parts.Block2048 = slabIndex.Parts.Block2048;
        head.Parts.SubBlock = slabIndex.Parts.SubBlock;
        head.Parts.Valid = true;
    }
private:
    void RedistributeSlabs(const SlabSize slab, const SlabIndex slabIndex, const SlabSize allocateSize) noexcept
    {
        switch(slab)
        {
            case SlabSize::S2048:
                Remove2048(slabIndex);
                break;
            case SlabSize::S1536:
                Remove1536(slabIndex);
                break;
            case SlabSize::S1024:
                Remove1024(slabIndex);
                break;
            case SlabSize::S768:
                Remove768(slabIndex);
                break;
            case SlabSize::S512:
                Remove512(slabIndex);
                break;
            case SlabSize::S256:
                Remove256(slabIndex);
                break;
            default: break;
        }

        u16 diffSize = GetSlabSize(slab) - GetSlabSize(allocateSize);
        u16 newBlockHead = GetRegisterFromSlab(slab, slabIndex) + GetSlabSize(allocateSize);

        if(diffSize >= 1536)
        {
            Insert1536(GetSlabFromRegister(newBlockHead));

            diffSize -= 1536;
            newBlockHead += 1536;
        }

        if(diffSize >= 1024)
        {
            Insert1024(GetSlabFromRegister(newBlockHead));

            diffSize -= 1024;
            newBlockHead += 1024;
        }

        if(diffSize >= 768)
        {
            Insert768(GetSlabFromRegister(newBlockHead));

            diffSize -= 768;
            newBlockHead += 768;
        }

        if(diffSize >= 512)
        {
            Insert512(GetSlabFromRegister(newBlockHead));

            diffSize -= 512;
            newBlockHead += 512;
        }

        if(diffSize >= 256)
        {
            Insert256(GetSlabFromRegister(newBlockHead));

            diffSize -= 256;
            newBlockHead += 256;
        }

        if(diffSize != 0)
        {
            ConPrinter::Print("Unable to correctly distribute register blocks. Remaining registers: {}\n", diffSize);
            assert(diffSize != 0);
        }
    }

    [[nodiscard]] RegisterBlockPair GetRegisterBlockPair(const SlabSize slab, const SlabIndex index) noexcept
    {
        switch(slab)
        {
            case SlabSize::S2048: return m_2048Blocks[index.Parts.Block2048][index.Parts.SubBlock];
            case SlabSize::S1536: return m_1536Blocks[index.Parts.Block2048][index.Parts.SubBlock];
            case SlabSize::S1024: return m_1024Blocks[index.Parts.Block2048][index.Parts.SubBlock];
            case SlabSize::S768:  return m_768Blocks[index.Parts.Block2048][index.Parts.SubBlock];
            case SlabSize::S512:  return m_512Blocks[index.Parts.Block2048][index.Parts.SubBlock];
            case SlabSize::S256:  return m_256Blocks[index.Parts.Block2048][index.Parts.SubBlock];
            default: return {};
        }
    }

    void Remove2048(const SlabIndex slabIndex) noexcept
    {
        RemoveBlock(m_2048Head, m_2048Blocks, slabIndex);
    }

    void Remove1536(const SlabIndex slabIndex) noexcept
    {
        RemoveBlock(m_1536Head, m_1536Blocks, slabIndex);
    }

    void Remove1024(const SlabIndex slabIndex) noexcept
    {
        RemoveBlock(m_1024Head, m_1024Blocks, slabIndex);
    }

    void Remove768(const SlabIndex slabIndex) noexcept
    {
        RemoveBlock(m_768Head, m_768Blocks, slabIndex);
    }

    void Remove512(const SlabIndex slabIndex) noexcept
    {
        RemoveBlock(m_512Head, m_512Blocks, slabIndex);
    }

    void Remove256(const SlabIndex slabIndex) noexcept
    {
        RemoveBlock(m_256Head, m_256Blocks, slabIndex);
    }

    void Insert2048(const SlabIndex slabIndex) noexcept
    {
        InsertBlock(m_2048Head, m_2048Blocks, slabIndex);
    }

    void Insert1536(const SlabIndex slabIndex) noexcept
    {
        InsertBlock(m_1536Head, m_1536Blocks, slabIndex);
    }

    void Insert1024(const SlabIndex slabIndex) noexcept
    {
        InsertBlock(m_1024Head, m_1024Blocks, slabIndex);
    }

    void Insert768(const SlabIndex slabIndex) noexcept
    {
        InsertBlock(m_768Head, m_768Blocks, slabIndex);
    }

    void Insert512(const SlabIndex slabIndex) noexcept
    {
        InsertBlock(m_512Head, m_512Blocks, slabIndex);
    }

    void Insert256(const SlabIndex slabIndex) noexcept
    {
        InsertBlock(m_256Head, m_256Blocks, slabIndex);
    }

    void Free1536(const u16 registerBase) noexcept
    {
        const SlabIndex slabIndex = GetSlabFromRegister(registerBase);
        
        // Is this the head of the block?
        if(slabIndex.Parts.SubBlock == 0)
        {
            if(m_512Blocks[slabIndex.Parts.Block2048][6].Available)
            {
                const SlabIndex nextBlock = {
                    .Parts {
                        .SubBlock = 6,
                        .Block2048 = slabIndex.Parts.Block2048,
                        .Valid = true,
                        .Pad = 0
                    }
                };

                Remove512(nextBlock);
                Insert2048(slabIndex);
                return;
            }
        }
        // Is this the tail of the block?
        else if(slabIndex.Parts.SubBlock == 2)
        {
            if(m_512Blocks[slabIndex.Parts.Block2048][0].Available)
            {
                const SlabIndex firstBlock = {
                    .Parts {
                        .SubBlock = 0,
                        .Block2048 = slabIndex.Parts.Block2048,
                        .Valid = true,
                        .Pad = 0
                    }
                };

                Remove512(firstBlock);
                Insert2048(firstBlock);
                return;
            }
        }
        // Is this the middle of the block?
        // If so we have to check for 2 256 blocks.
        else
        {
            if(m_256Blocks[slabIndex.Parts.Block2048][0].Available && m_256Blocks[slabIndex.Parts.Block2048][7].Available)
            {
                const SlabIndex headBlock256 = {
                    .Parts {
                        .SubBlock = 0,
                        .Block2048 = slabIndex.Parts.Block2048,
                        .Valid = true,
                        .Pad = 0
                    }
                };
                const SlabIndex tailBlock256 = {
                    .Parts {
                        .SubBlock = 7,
                        .Block2048 = slabIndex.Parts.Block2048,
                        .Valid = true,
                        .Pad = 0
                    }
                };

                Remove512(headBlock256);
                Remove512(tailBlock256);
                Insert2048(headBlock256);
                return;
            }
        }

        // If we can't find a merge target, or we're in the middle of the block, just store into the 1536 list.
        Insert1536(slabIndex);
    }

    void Free1024(const u16 registerBase) noexcept
    {
        const SlabIndex slabIndex = GetSlabFromRegister(registerBase);
        
        // Is this the head of the block?
        // If so we can check for 1024 blocks, we also need to check for 512 blocks.
        if(slabIndex.Parts.SubBlock == 0)
        {
            const SlabIndex nextBlock = {
                .Parts {
                    .SubBlock = 4,
                    .Block2048 = slabIndex.Parts.Block2048,
                    .Valid = true,
                    .Pad = 0
                }
            };

            if(m_1024Blocks[slabIndex.Parts.Block2048][4].Available)
            {
                Remove1024(nextBlock);
                Insert2048(slabIndex);
                return;
            }
            
            if(m_512Blocks[slabIndex.Parts.Block2048][4].Available)
            {
                Remove512(nextBlock);
                Insert1536(slabIndex);
                return;
            }
        }
        // Is this the tail of the block?
        // If so we can check for 1024 blocks, we also need to check for 512 blocks.
        else if(slabIndex.Parts.SubBlock == 4)
        {
            if(m_1024Blocks[slabIndex.Parts.Block2048][0].Available)
            {
                const SlabIndex firstBlock = {
                    .Parts {
                        .SubBlock = 0,
                        .Block2048 = slabIndex.Parts.Block2048,
                        .Valid = true,
                        .Pad = 0
                    }
                };

                Remove1024(firstBlock);
                Insert2048(firstBlock);
                return;
            }
            
            if(m_512Blocks[slabIndex.Parts.Block2048][2].Available)
            {
                const SlabIndex slab512 = {
                    .Parts {
                        .SubBlock = 2,
                        .Block2048 = slabIndex.Parts.Block2048,
                        .Valid = true,
                        .Pad = 0
                    }
                };

                Remove512(slab512);
                Insert1536(slab512);
                return;
            }
        }
        // Is this the second block.
        // If so we need to check for a 512 tail block, or a 256 head block with either a 256 or 768 tail block.
        else if(slabIndex.Parts.SubBlock == 1)
        {
            if(m_512Blocks[slabIndex.Parts.Block2048][5].Available)
            {
                const SlabIndex slab512 = {
                    .Parts {
                        .SubBlock = 5,
                        .Block2048 = slabIndex.Parts.Block2048,
                        .Valid = true,
                        .Pad = 0
                    }
                };

                Remove512(slab512);
                Insert1536(slabIndex);
                return;
            }

            // Is there a head 256 block?
            // If so we need to check for either a 256 or 768 tail block.
            // We can skip a check for a tail 256 and 512 block as those would've already been merged.
            if(m_256Blocks[slabIndex.Parts.Block2048][0].Available)
            {
                const SlabIndex headBlock = {
                    .Parts {
                        .SubBlock = 5,
                        .Block2048 = slabIndex.Parts.Block2048,
                        .Valid = true,
                        .Pad = 0
                    }
                };

                const SlabIndex tailBlock = {
                    .Parts {
                        .SubBlock = 5,
                        .Block2048 = slabIndex.Parts.Block2048,
                        .Valid = true,
                        .Pad = 0
                    }
                };

                if(m_256Blocks[slabIndex.Parts.Block2048][5].Available)
                {
                    Remove256(headBlock);
                    Remove256(tailBlock);
                    Insert1536(headBlock);
                    return;
                }

                if(m_768Blocks[slabIndex.Parts.Block2048][5].Available)
                {
                    Remove256(headBlock);
                    Remove768(tailBlock);
                    Insert2048(headBlock);
                    return;
                }
            }
        }
        // Is this the third block.
        // If so we need to check for a 512 head block and/or tail block, or a 256 head and tail block.
        else if(slabIndex.Parts.SubBlock == 2)
        {
            if(m_512Blocks[slabIndex.Parts.Block2048][0].Available)
            {
                const SlabIndex headSlab = {
                    .Parts {
                        .SubBlock = 0,
                        .Block2048 = slabIndex.Parts.Block2048,
                        .Valid = true,
                        .Pad = 0
                    }
                };

                Remove512(headSlab);
                Free1536(registerBase - 512);
                return;
            }

            const SlabIndex tailSlab = {
                .Parts {
                    .SubBlock = 6,
                    .Block2048 = slabIndex.Parts.Block2048,
                    .Valid = true,
                    .Pad = 0
                }
            };

            if(m_512Blocks[slabIndex.Parts.Block2048][6].Available)
            {
                Remove512(tailSlab);
                Free1536(registerBase);
                return;
            }

            if(m_256Blocks[slabIndex.Parts.Block2048][1].Available && m_256Blocks[slabIndex.Parts.Block2048][6].Available)
            {
                const SlabIndex headSlab = {
                    .Parts {
                        .SubBlock = 1,
                        .Block2048 = slabIndex.Parts.Block2048,
                        .Valid = true,
                        .Pad = 0
                    }
                };

                Remove256(headSlab);
                Remove256(tailSlab);
                Insert1536(headSlab);
                return;
            }
        }
        // Is this the fourth block.
        // If so we need to check for a 512 head block, or a 256 tail block with either a 256 or 768 head block.
        else if(slabIndex.Parts.SubBlock == 3)
        {
            if(m_512Blocks[slabIndex.Parts.Block2048][1].Available)
            {
                const SlabIndex slab512 = {
                    .Parts {
                        .SubBlock = 1,
                        .Block2048 = slabIndex.Parts.Block2048,
                        .Valid = true,
                        .Pad = 0
                    }
                };

                Remove512(slab512);
                Insert1536(slab512);
                return;
            }

            // Is there a tail 256 block?
            // If so we need to check for either a 256 or 768 head block.
            // We can skip a check for a head 256 and 512 block as those would've already been merged.
            if(m_256Blocks[slabIndex.Parts.Block2048][7].Available)
            {
                const SlabIndex tailBlock = {
                    .Parts {
                        .SubBlock = 7,
                        .Block2048 = slabIndex.Parts.Block2048,
                        .Valid = true,
                        .Pad = 0
                    }
                };

                if(m_256Blocks[slabIndex.Parts.Block2048][2].Available)
                {
                    const SlabIndex headBlock = {
                        .Parts {
                            .SubBlock = 2,
                            .Block2048 = slabIndex.Parts.Block2048,
                            .Valid = true,
                            .Pad = 0
                        }
                    };

                    Remove256(headBlock);
                    Remove256(tailBlock);
                    Insert1536(headBlock);
                    return;
                }

                if(m_768Blocks[slabIndex.Parts.Block2048][0].Available)
                {
                    const SlabIndex headBlock = {
                        .Parts {
                            .SubBlock = 0,
                            .Block2048 = slabIndex.Parts.Block2048,
                            .Valid = true,
                            .Pad = 0
                        }
                    };

                    Remove768(headBlock);
                    Remove256(tailBlock);
                    Insert2048(headBlock);
                    return;
                }
            }
        }

        // If we can't find a merge target, or we're in the middle of the block, just store into the 1024 list.
        Insert1024(slabIndex);
    }

    void Free768(const u16 registerBase) noexcept
    {
        const SlabIndex slabIndex = GetSlabFromRegister(registerBase);

        // Can this block be followed by a 768 block?
        // If so we can check for a tail 256 or 768 block.
        if(slabIndex.Parts.SubBlock == 0 || slabIndex.Parts.SubBlock == 1 || slabIndex.Parts.SubBlock == 2)
        {
            const SlabIndex nextBlock = {
                .Parts {
                    .SubBlock = static_cast<u8>(slabIndex.Parts.SubBlock + 3),
                    .Block2048 = slabIndex.Parts.Block2048,
                    .Valid = true,
                    .Pad = 0
                }
            };

            if(m_256Blocks[slabIndex.Parts.Block2048][slabIndex.Parts.SubBlock + 3].Available)
            {
                Remove256(nextBlock);
                Free1024(registerBase);
                return;
            }

            if(m_768Blocks[slabIndex.Parts.Block2048][slabIndex.Parts.SubBlock + 3].Available)
            {
                Remove768(nextBlock);
                Free1536(registerBase);
                return;
            }
        }
        // Can this block be preceded by a 768 block?
        // If so we can check for a head 256 or 768 block.
        else if(slabIndex.Parts.SubBlock == 3 || slabIndex.Parts.SubBlock == 4 || slabIndex.Parts.SubBlock == 5)
        {
            if(m_256Blocks[slabIndex.Parts.Block2048][slabIndex.Parts.SubBlock - 1].Available)
            {
                const SlabIndex nextBlock = {
                    .Parts {
                        .SubBlock = static_cast<u8>(slabIndex.Parts.SubBlock - 1),
                        .Block2048 = slabIndex.Parts.Block2048,
                        .Valid = true,
                        .Pad = 0
                    }
                };

                Remove256(nextBlock);
                Free1024(registerBase - 256);
                return;
            }

            if(m_768Blocks[slabIndex.Parts.Block2048][slabIndex.Parts.SubBlock - 3].Available)
            {
                const SlabIndex nextBlock = {
                    .Parts {
                        .SubBlock = static_cast<u8>(slabIndex.Parts.SubBlock - 3),
                        .Block2048 = slabIndex.Parts.Block2048,
                        .Valid = true,
                        .Pad = 0
                    }
                };

                Remove768(nextBlock);
                Free1536(registerBase - 768);
                return;
            }
        }

        // Is this the head block?
        // If so we can check for a tail 1024 and followed by a 256 block.
        if(slabIndex.Parts.SubBlock == 0)
        {
            if(m_1024Blocks[slabIndex.Parts.Block2048][3].Available && m_256Blocks[slabIndex.Parts.Block2048][7].Available)
            {
                const SlabIndex tail1024 = {
                    .Parts {
                        .SubBlock = 3,
                        .Block2048 = slabIndex.Parts.Block2048,
                        .Valid = true,
                        .Pad = 0
                    }
                };
                const SlabIndex tail256 = {
                    .Parts {
                        .SubBlock = 7,
                        .Block2048 = slabIndex.Parts.Block2048,
                        .Valid = true,
                        .Pad = 0
                    }
                };

                Remove1024(tail1024);
                Remove256(tail256);
                Insert2048(slabIndex);
                return;
            }
        }
        // Is this the tail block?
        // If so we can check for a head 1024 and preceded by a 256 block.
        else if(slabIndex.Parts.SubBlock == 5)
        {
            if(m_1024Blocks[slabIndex.Parts.Block2048][1].Available && m_256Blocks[slabIndex.Parts.Block2048][0].Available)
            {
                const SlabIndex head1024 = {
                    .Parts {
                        .SubBlock = 1,
                        .Block2048 = slabIndex.Parts.Block2048,
                        .Valid = true,
                        .Pad = 0
                    }
                };
                const SlabIndex head256 = {
                    .Parts {
                        .SubBlock = 0,
                        .Block2048 = slabIndex.Parts.Block2048,
                        .Valid = true,
                        .Pad = 0
                    }
                };

                Remove1024(head1024);
                Remove256(head256);
                Insert2048(head256);
                return;
            }
        }

        // Can this block be preceded by only a 256 block?
        if(slabIndex.Parts.SubBlock == 1 || slabIndex.Parts.SubBlock == 2)
        {
            if(m_256Blocks[slabIndex.Parts.Block2048][slabIndex.Parts.SubBlock - 1].Available)
            {
                const SlabIndex head256 = {
                    .Parts {
                        .SubBlock = static_cast<u8>(slabIndex.Parts.SubBlock - 1),
                        .Block2048 = slabIndex.Parts.Block2048,
                        .Valid = true,
                        .Pad = 0
                    }
                };

                Remove256(head256);
                Free1024(registerBase - 256);
                return;
            }
        }
        // Can this block be followed by only a 256 block?
        else if(slabIndex.Parts.SubBlock == 3 || slabIndex.Parts.SubBlock == 4)
        {
            if(m_256Blocks[slabIndex.Parts.Block2048][slabIndex.Parts.SubBlock + 3].Available)
            {
                const SlabIndex head256 = {
                    .Parts {
                        .SubBlock = static_cast<u8>(slabIndex.Parts.SubBlock + 3),
                        .Block2048 = slabIndex.Parts.Block2048,
                        .Valid = true,
                        .Pad = 0
                    }
                };

                Remove256(head256);
                Free1024(registerBase);
                return;
            }
        }

        Insert768(slabIndex);
    }

    void Free512(const u16 registerBase) noexcept
    {
        const SlabIndex slabIndex = GetSlabFromRegister(registerBase);

        // Is this the head block?
        // If so it can be followed by a 1536 or 1024 block.
        if(slabIndex.Parts.SubBlock == 0)
        {
            const SlabIndex tail {
                .Parts {
                    .SubBlock = 2,
                    .Block2048 = slabIndex.Parts.Block2048,
                    .Valid = true,
                    .Pad = 0
                }
            };

            if(m_1024Blocks[slabIndex.Parts.Block2048][2].Available)
            {
                Remove1024(tail);
                // We don't need to call free as this is not followed by a 512 block; if it was it would've already been merged into a 1536 block.
                Insert1536(slabIndex);
                return;
            }

            if(m_1536Blocks[slabIndex.Parts.Block2048][2].Available)
            {
                Remove1536(tail);
                Insert2048(slabIndex);
                return;
            }
        }
        // Is this the tail block?
        // If so it can be preceded by a 1536 or 1024 block.
        else if(slabIndex.Parts.SubBlock == 6)
        {
            if(m_1024Blocks[slabIndex.Parts.Block2048][2].Available)
            {
                const SlabIndex head {
                    .Parts {
                        .SubBlock = 2,
                        .Block2048 = slabIndex.Parts.Block2048,
                        .Valid = true,
                        .Pad = 0
                    }
                };

                Remove1024(head);
                // We don't need to call free as this is not preceded by a 512 block; if it was it would've already been merged into a 1536 block.
                Insert1536(head);
                return;
            }

            if(m_1536Blocks[slabIndex.Parts.Block2048][0].Available)
            {
                const SlabIndex head {
                    .Parts {
                        .SubBlock = 0,
                        .Block2048 = slabIndex.Parts.Block2048,
                        .Valid = true,
                        .Pad = 0
                    }
                };

                Remove1536(head);
                Insert2048(head);
                return;
            }
        }
        // Is this the the second block?
        // If so it can be followed by a 1024 block.
        else if(slabIndex.Parts.SubBlock == 1)
        {
            if(m_1024Blocks[slabIndex.Parts.Block2048][3].Available)
            {
                const SlabIndex tail {
                    .Parts {
                        .SubBlock = 3,
                        .Block2048 = slabIndex.Parts.Block2048,
                        .Valid = true,
                        .Pad = 0
                    }
                };

                Remove1024(tail);
                Free1536(registerBase);
                return;
            }
        }
        // Is this the second to last block?
        // If so it can be preceded by a 1024 block.
        else if(slabIndex.Parts.SubBlock == 5)
        {
            if(m_1024Blocks[slabIndex.Parts.Block2048][0].Available)
            {
                const SlabIndex head {
                    .Parts {
                        .SubBlock = 2,
                        .Block2048 = slabIndex.Parts.Block2048,
                        .Valid = true,
                        .Pad = 0
                    }
                };

                Remove1024(head);
                Free1536(registerBase - 1024);
                return;
            }
        }

        // Is this not the head block?
        // If so this can always be preceded by a 256 or 512 block.
        if(slabIndex.Parts.SubBlock != 0)
        {
            if(m_256Blocks[slabIndex.Parts.Block2048][slabIndex.Parts.SubBlock - 1].Available)
            {
                const SlabIndex head256 {
                    .Parts {
                        .SubBlock = static_cast<u8>(slabIndex.Parts.SubBlock - 1),
                        .Block2048 = slabIndex.Parts.Block2048,
                        .Valid = true,
                        .Pad = 0
                    }
                };

                Remove256(head256);
                Free768(registerBase - 256);
                return;
            }

            if(m_512Blocks[slabIndex.Parts.Block2048][slabIndex.Parts.SubBlock - 2].Available)
            {
                const SlabIndex head512 {
                    .Parts {
                        .SubBlock = static_cast<u8>(slabIndex.Parts.SubBlock - 2),
                        .Block2048 = slabIndex.Parts.Block2048,
                        .Valid = true,
                        .Pad = 0
                    }
                };

                Remove512(head512);
                Free1024(registerBase - 512);
                return;
            }
        }

        // Is this not the tail block?
        // If so this can always be followed by a 256 or 512 block.
        if(slabIndex.Parts.SubBlock != 6)
        {
            const SlabIndex tail {
                .Parts {
                    .SubBlock = static_cast<u8>(slabIndex.Parts.SubBlock + 1),
                    .Block2048 = slabIndex.Parts.Block2048,
                    .Valid = true,
                    .Pad = 0
                }
            };

            if(m_256Blocks[slabIndex.Parts.Block2048][slabIndex.Parts.SubBlock + 1].Available)
            {
                Remove256(tail);
                Free768(registerBase);
                return;
            }

            if(m_512Blocks[slabIndex.Parts.Block2048][slabIndex.Parts.SubBlock - 2].Available)
            {
                Remove512(tail);
                Free1024(registerBase);
                return;
            }
        }

        Insert512(slabIndex);
    }

    void Free256(const u16 registerBase) noexcept
    {
        const SlabIndex slabIndex = GetSlabFromRegister(registerBase);

        // Is this the first block?
        // If so it could be followed by a 1536 and a 256 block.
        if(slabIndex.Parts.SubBlock == 0)
        {
            if(m_1536Blocks[slabIndex.Parts.Block2048][1].Available && m_256Blocks[slabIndex.Parts.Block2048][7].Available)
            {
                const SlabIndex tail1536 {
                    .Parts {
                        .SubBlock = 1,
                        .Block2048 = slabIndex.Parts.Block2048,
                        .Valid = true,
                        .Pad = 0
                    }
                };
                const SlabIndex tail256 {
                    .Parts {
                        .SubBlock = 7,
                        .Block2048 = slabIndex.Parts.Block2048,
                        .Valid = true,
                        .Pad = 0
                    }
                };

                Remove1536(tail1536);
                Remove256(tail256);
                Insert2048(slabIndex);
                return;
            }
        }
        // Is this the last block?
        // If so it could be preceded by a 1536 and a 256 block.
        else if(slabIndex.Parts.SubBlock == 7)
        {
            if(m_1536Blocks[slabIndex.Parts.Block2048][1].Available && m_256Blocks[slabIndex.Parts.Block2048][0].Available)
            {
                const SlabIndex head1536 {
                    .Parts {
                        .SubBlock = 1,
                        .Block2048 = slabIndex.Parts.Block2048,
                        .Valid = true,
                        .Pad = 0
                    }
                };
                const SlabIndex head256 {
                    .Parts {
                        .SubBlock = 0,
                        .Block2048 = slabIndex.Parts.Block2048,
                        .Valid = true,
                        .Pad = 0
                    }
                };

                Remove1536(head1536);
                Remove256(head256);
                Insert2048(head256);
                return;
            }
        }

        // Can this block be preceded by a 1024 block and a 256 block?
        if(slabIndex.Parts.SubBlock > 5)
        {
            if(m_1024Blocks[slabIndex.Parts.Block2048][slabIndex.Parts.SubBlock - 4].Available && m_256Blocks[slabIndex.Parts.Block2048][slabIndex.Parts.SubBlock - 5].Available)
            {
                const SlabIndex head1024 {
                    .Parts {
                        .SubBlock = static_cast<u8>(slabIndex.Parts.SubBlock - 4),
                        .Block2048 = slabIndex.Parts.Block2048,
                        .Valid = true,
                        .Pad = 0
                    }
                };
                const SlabIndex head256 {
                    .Parts {
                        .SubBlock = static_cast<u8>(slabIndex.Parts.SubBlock - 5),
                        .Block2048 = slabIndex.Parts.Block2048,
                        .Valid = true,
                        .Pad = 0
                    }
                };

                Remove1024(head1024);
                Remove256(head256);
                Free1536(registerBase - 1536);
                return;
            }
        }
        // Can this block be followed by a 1024 block and a 256 block?
        else if(slabIndex.Parts.SubBlock < 2)
        {
            if(m_1024Blocks[slabIndex.Parts.Block2048][slabIndex.Parts.SubBlock + 4].Available && m_256Blocks[slabIndex.Parts.Block2048][slabIndex.Parts.SubBlock + 5].Available)
            {
                const SlabIndex tail1024 {
                    .Parts {
                        .SubBlock = static_cast<u8>(slabIndex.Parts.SubBlock + 4),
                        .Block2048 = slabIndex.Parts.Block2048,
                        .Valid = true,
                        .Pad = 0
                    }
                };
                const SlabIndex tail256 {
                    .Parts {
                        .SubBlock = static_cast<u8>(slabIndex.Parts.SubBlock + 5),
                        .Block2048 = slabIndex.Parts.Block2048,
                        .Valid = true,
                        .Pad = 0
                    }
                };

                Remove1024(tail1024);
                Remove256(tail256);
                Free1536(registerBase);
                return;
            }
        }

        // Is this not the first, second, or third block?
        // If so it can always be preceded by a 768 block.
        if(slabIndex.Parts.SubBlock > 2)
        {
            if(m_768Blocks[slabIndex.Parts.Block2048][slabIndex.Parts.SubBlock - 3].Available)
            {
                const SlabIndex head {
                    .Parts {
                        .SubBlock = static_cast<u8>(slabIndex.Parts.SubBlock - 3),
                        .Block2048 = slabIndex.Parts.Block2048,
                        .Valid = true,
                        .Pad = 0
                    }
                };

                Remove768(head);
                Free1024(registerBase - 768);
                return;
            }
        }

        // Is this not the last, second to last, or third to last block?
        // If so it can always be followed by a 768 block.
        if(slabIndex.Parts.SubBlock < 5)
        {
            if(m_768Blocks[slabIndex.Parts.SubBlock][slabIndex.Parts.SubBlock + 1].Available)
            {
                const SlabIndex tail {
                    .Parts {
                        .SubBlock = static_cast<u8>(slabIndex.Parts.SubBlock + 1),
                        .Block2048 = slabIndex.Parts.Block2048,
                        .Valid = true,
                        .Pad = 0
                    }
                };

                Remove768(tail);
                Free1024(registerBase);
                return;
            }
        }

        // Is this not the first or second block?
        // If so it can always be preceded by a 512 block.
        if(slabIndex.Parts.SubBlock > 1)
        {
            if(m_512Blocks[slabIndex.Parts.Block2048][slabIndex.Parts.SubBlock - 2].Available)
            {
                const SlabIndex head {
                    .Parts {
                        .SubBlock = static_cast<u8>(slabIndex.Parts.SubBlock - 2),
                        .Block2048 = slabIndex.Parts.Block2048,
                        .Valid = true,
                        .Pad = 0
                    }
                };

                Remove512(head);
                Free768(registerBase - 512);
                return;
            }
        }

        // Is this not the last or second to last block?
        // If so it can always be followed by a 512 block.
        if(slabIndex.Parts.SubBlock < 6)
        {
            if(m_512Blocks[slabIndex.Parts.SubBlock][slabIndex.Parts.SubBlock + 2].Available)
            {
                const SlabIndex tail {
                    .Parts {
                        .SubBlock = static_cast<u8>(slabIndex.Parts.SubBlock + 2),
                        .Block2048 = slabIndex.Parts.Block2048,
                        .Valid = true,
                        .Pad = 0
                    }
                };

                Remove512(tail);
                Free768(registerBase);
                return;
            }
        }

        // Is this not the first block?
        // If so it can always be preceded by a 256 block.
        if(slabIndex.Parts.SubBlock != 0)
        {
            if(m_256Blocks[slabIndex.Parts.SubBlock][slabIndex.Parts.SubBlock - 1].Available)
            {
                const SlabIndex head {
                    .Parts {
                        .SubBlock = static_cast<u8>(slabIndex.Parts.SubBlock - 1),
                        .Block2048 = slabIndex.Parts.Block2048,
                        .Valid = true,
                        .Pad = 0
                    }
                };

                Remove256(head);
                Free512(registerBase - 256);
                return;
            }
        }

        // Is this not the last block?
        // If so it can always be followed by a 256 block.
        if(slabIndex.Parts.SubBlock != 7)
        {
            if(m_256Blocks[slabIndex.Parts.SubBlock][slabIndex.Parts.SubBlock + 1].Available)
            {
                const SlabIndex tail {
                    .Parts {
                        .SubBlock = static_cast<u8>(slabIndex.Parts.SubBlock + 1),
                        .Block2048 = slabIndex.Parts.Block2048,
                        .Valid = true,
                        .Pad = 0
                    }
                };

                Remove256(tail);
                Free512(registerBase);
                return;
            }
        }

        Insert256(slabIndex);
    }
private:
    RegisterBlockPair m_2048Blocks[2][1];
    RegisterBlockPair m_1536Blocks[2][3];
    RegisterBlockPair m_1024Blocks[2][5];
    RegisterBlockPair m_768Blocks[2][6];
    RegisterBlockPair m_512Blocks[2][7];
    RegisterBlockPair m_256Blocks[2][8];

    SlabIndex m_2048Head;
    SlabIndex m_1536Head;
    SlabIndex m_1024Head;
    SlabIndex m_768Head;
    SlabIndex m_512Head;
    SlabIndex m_256Head;
};
