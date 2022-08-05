// #pragma once
//
// #include <Objects.hpp>
// #include <NumTypes.hpp>
// #include <ConPrinter.hpp>
//
// #include "RegisterFile.hpp"
//
// class RegisterAllocator final
// {
//     DEFAULT_DESTRUCT(RegisterAllocator);
//     DELETE_CM(RegisterAllocator);
// private:
//     struct RegisterBlockPair final
//     {
//         // u32 BaseRegister : 12;
//         u32 PrevFree : 5;
//         u32 NextFree : 5;
//         u32 Available : 1;
//         u32 Pad : 5;
//     };
//
//     enum class SlabSize : u8
//     {
//         S2048,
//         S1536,
//         S1024,
//         S768,
//         S512,
//         S256,
//     };
// public:
//     RegisterAllocator() noexcept
//         : m_2048Blocks{ }
//         , m_1536Blocks{ }
//         , m_1024Blocks{ }
//         , m_768Blocks{ }
//         , m_512Blocks{ }
//         , m_256Blocks{ }
//         , m_2048Head(0)
//         , m_1536Head(0x8000)
//         , m_1024Head(0x8000)
//         , m_768Head(0x8000)
//         , m_512Head(0x8000)
//         , m_256Head(0x8000)
//     {
//         m_2048Blocks[0].NextFree = 1;
//         m_2048Blocks[0].PrevFree = 0x10;
//         m_2048Blocks[0].Available = true;
//         m_2048Blocks[1].NextFree = 0x10;
//         m_2048Blocks[1].PrevFree = 0;
//         m_2048Blocks[1].Available = true;
//     }
//
//     // Register Count uses 1 based indexing.
//     [[nodiscard]] u16 AllocateRegisterBlock(const u16 registerCount) noexcept
//     {
//         if(registerCount < 256)
//         {
//             if(m_256Head != 0x8000)
//             {
//                 const u16 ret = GetRegisterFromSlab(SlabSize::S512, m_256Head);
//                 RedistributeSlabs(SlabSize::S256, m_256Head, ComputeRegisterCount(registerCount));
//
//                 return ret;
//             }
//         }
//
//         if(registerCount < 512)
//         {
//             if(m_512Head != 0x8000)
//             {
//                 const u16 ret = GetRegisterFromSlab(SlabSize::S512, m_512Head);
//                 RedistributeSlabs(SlabSize::S512, m_512Head, ComputeRegisterCount(registerCount));
//
//                 return ret;
//             }
//         }
//
//         if(registerCount < 768)
//         {
//             if(m_768Head != 0x8000)
//             {
//                 const u16 ret = GetRegisterFromSlab(SlabSize::S768, m_768Head);
//                 RedistributeSlabs(SlabSize::S768, m_768Head, ComputeRegisterCount(registerCount));
//
//                 return ret;
//             }
//         }
//
//         if(registerCount < 1024)
//         {
//             if(m_1024Head != 0x8000)
//             {
//                 const u16 ret = GetRegisterFromSlab(SlabSize::S1024, m_1024Head);
//                 RedistributeSlabs(SlabSize::S1024, m_1024Head, ComputeRegisterCount(registerCount));
//
//                 return ret;
//             }
//         }
//
//         if(registerCount < 1536)
//         {
//             if(m_1536Head != 0x8000)
//             {
//                 const u16 ret = GetRegisterFromSlab(SlabSize::S1536, m_1536Head);
//                 RedistributeSlabs(SlabSize::S1536, m_1536Head, ComputeRegisterCount(registerCount));
//
//                 return ret;
//             }
//         }
//         
//         if(m_2048Head != 0x8000)
//         {
//             const u16 ret = GetRegisterFromSlab(SlabSize::S2048, m_2048Head);
//             RedistributeSlabs(SlabSize::S2048, m_2048Head, ComputeRegisterCount(registerCount));
//
//             return ret;
//         }
//
//         return 0xFFFF;
//     }
//
//     // Register count uses 1 based indexing.
//     void FreeRegisterBlock(const u16 registerBase, const u8 registerCount) noexcept
//     {
//         const SlabSize slabSize = ComputeRegisterCount(registerCount);
//
//         switch(slabSize)
//         {
//             case SlabSize::S2048:
//             {
//                 const u16 slabIndex = GetSlabFromRegister(registerBase);
//
//                 m_2048Blocks[slabIndex].NextFree = m_2048Head;
//                 // m_2048Blocks[slabIndex].PrevFree = 0x10;
//                 m_2048Blocks[m_2048Head].PrevFree = slabIndex;
//                 m_2048Head = slabIndex;
//                 break;
//             }
//             case SlabSize::S1536:
//             {
//                 Free1536(registerBase);
//                 break;
//             }
//             case SlabSize::S1024:
//             {
//                 const u16 slabIndex = GetSlabFromRegister(registerBase);
//
//                 const u32 offset = slabIndex > 3 ? 7 : 0;
//
//                 // Is this the head of the block?
//                 if(slabIndex - offset == 0)
//                 {
//                     const u16 slab512 = GetSlabFromRegister(registerBase + 1536);
//
//                     if(m_512Blocks[slab512].Available)
//                     {
//                         m_512Blocks[slab512].Available = false;
//                         const u16 prevFree = m_512Blocks[slab512].PrevFree;
//                         const u16 nextFree = m_512Blocks[slab512].NextFree;
//
//                         if(slab512 == m_512Head)
//                         {
//                             m_512Head = nextFree;
//                         }
//                         else if(prevFree != 0x10)
//                         {
//                             m_512Blocks[prevFree].NextFree = nextFree;
//                         }
//
//                         if(nextFree != 0x10)
//                         {
//                             m_512Blocks[nextFree].PrevFree = prevFree;
//                         }
//
//                         m_2048Blocks[slabIndex].NextFree = m_2048Head;
//                         m_2048Blocks[slabIndex].PrevFree = 0x10;
//                         m_2048Blocks[m_2048Head].PrevFree = slabIndex;
//                         m_2048Head = slabIndex;
//                         break;
//                     }
//                 }
//                 // Is this the tail of the block?
//                 else if(slabIndex - offset == 2)
//                 {
//                     const u16 slab512 = GetSlabFromRegister(registerBase - 512);
//
//                     if(m_512Blocks[slab512].Available)
//                     {
//                         m_512Blocks[slab512].Available = false;
//                         const u16 prevFree = m_512Blocks[slab512].PrevFree;
//                         const u16 nextFree = m_512Blocks[slab512].NextFree;
//
//                         if(slab512 == m_512Head)
//                         {
//                             m_512Head = nextFree;
//                         }
//                         else if(prevFree != 0x10)
//                         {
//                             m_512Blocks[prevFree].NextFree = nextFree;
//                         }
//
//                         if(nextFree != 0x10)
//                         {
//                             m_512Blocks[nextFree].PrevFree = prevFree;
//                         }
//
//                         m_2048Blocks[slabIndex].NextFree = m_2048Head;
//                         m_2048Blocks[slabIndex].PrevFree = 0x10;
//                         m_2048Blocks[m_2048Head].PrevFree = slabIndex;
//                         m_2048Head = slabIndex;
//                         break;
//                     }
//                 }
//
//                 // If we can't find a merge target, or we're in the middle of the block, just store into the 1536 list.
//
//                 m_1536Blocks[slabIndex].NextFree = m_1536Head;
//                 // m_1536Blocks[slabIndex].PrevFree = 0x10;
//                 m_1536Blocks[m_1536Head].PrevFree = slabIndex;
//                 m_1536Head = slabIndex;
//                 break;
//             }
//             default: break;
//         }
//
//         if(realRegisterCount == 256)
//         {
//             if(m_256Tail == 0x8000)
//             {
//                 m_256Tail = 0;
//             }
//             else
//             {
//                 ++m_256Tail;
//             }
//
//             m_256Blocks[m_256Tail] = registerBase;
//
//             return;
//         }
//
//         if(realRegisterCount == 192)
//         {
//             Free192(registerBase);
//         }
//         else if(realRegisterCount == 128)
//         {
//             Free128(registerBase);
//         }
//         else if(realRegisterCount == 96)
//         {
//             Free96(registerBase);
//         }
//         else if(realRegisterCount == 64)
//         {
//             Free64(registerBase);
//         }
//     }
// private:
//     [[nodiscard]] SlabSize ComputeRegisterCount(const u16 targetRegisterCount) noexcept
//     {
//         if(targetRegisterCount >= 1536)
//         {
//             return SlabSize::S2048;
//         }
//         else if(targetRegisterCount >= 1024)
//         {
//             return SlabSize::S1536;
//         }
//         else if(targetRegisterCount >= 768)
//         {
//             return SlabSize::S1024;
//         }
//         else if(targetRegisterCount >= 512)
//         {
//             return SlabSize::S768;
//         }
//         else if(targetRegisterCount >= 256)
//         {
//             return SlabSize::S512;
//         }
//         else
//         {
//             return SlabSize::S256;
//         }
//     }
//
//     void RedistributeSlabs(const SlabSize slab, const u32 slabIndex, const u32 allocateSize) noexcept
//     {
//         {
//             const RegisterBlockPair block = GetRegisterBlockPair(slab, slabIndex);
//
//             switch(slab)
//             {
//                 case SlabSize::S2048:
//                     m_2048Head = static_cast<u16>(block.NextFree);
//                 break;
//                 case SlabSize::S1536:
//                     m_1536Head = static_cast<u16>(block.NextFree);
//                 break;
//                 case SlabSize::S1024:
//                     m_1024Head = static_cast<u16>(block.NextFree);
//                 break;
//                 case SlabSize::S768:
//                     m_768Head = static_cast<u16>(block.NextFree);
//                 break;
//                 case SlabSize::S512:
//                     m_512Head = static_cast<u16>(block.NextFree);
//                 break;
//                 case SlabSize::S256:
//                     m_256Head = static_cast<u16>(block.NextFree);
//                 break;
//                 default: break;
//             }
//         }
//
//         u32 diffSize = GetSlabSize(slab) - allocateSize;
//         u32 newBlockHead = GetRegisterFromSlab(slab, slabIndex) + allocateSize;
//
//         if(diffSize >= 1536)
//         {
//             const u32 blockIndex = newBlockHead / 1536;
//
//             m_1536Blocks[blockIndex].NextFree = m_1536Head;
//             // m_1536Blocks[blockIndex].PrevFree = 0x10;
//             m_1536Blocks[m_1536Head].PrevFree = blockIndex;
//             m_1536Head = static_cast<u16>(blockIndex);
//
//             diffSize -= 1536;
//             newBlockHead += 1536;
//         }
//
//         if(diffSize >= 1024)
//         {
//             const u32 blockIndex = newBlockHead / 1024;
//
//             m_1024Blocks[blockIndex].NextFree = m_1024Head;
//             // m_1024Blocks[blockIndex].PrevFree = 0x10;
//             m_1024Blocks[m_1024Head].PrevFree = blockIndex;
//             m_1024Head = static_cast<u16>(blockIndex);
//
//             diffSize -= 1024;
//             newBlockHead += 1024;
//         }
//
//         if(diffSize >= 768)
//         {
//             const u32 blockIndex = newBlockHead / 768;
//
//             m_768Blocks[blockIndex].NextFree = m_768Head;
//             // m_768Blocks[blockIndex].PrevFree = 0x10;
//             m_768Blocks[m_768Head].PrevFree = blockIndex;
//             m_768Head = static_cast<u16>(blockIndex);
//
//             diffSize -= 768;
//             newBlockHead += 768;
//         }
//
//         if(diffSize >= 512)
//         {
//             const u32 blockIndex = newBlockHead / 512;
//
//             m_512Blocks[blockIndex].NextFree = m_512Head;
//             // m_512Blocks[blockIndex].PrevFree = 0x10;
//             m_512Blocks[m_512Head].PrevFree = blockIndex;
//             m_512Head = static_cast<u16>(blockIndex);
//
//             diffSize -= 512;
//             newBlockHead += 512;
//         }
//
//         if(diffSize >= 256)
//         {
//             const u32 blockIndex = newBlockHead / 256;
//
//             m_256Blocks[blockIndex].NextFree = m_256Head;
//             // m_256Blocks[blockIndex].PrevFree = 0x10;
//             m_256Blocks[m_256Head].PrevFree = blockIndex;
//             m_256Head = static_cast<u16>(blockIndex);
//
//             diffSize -= 256;
//             newBlockHead += 256;
//         }
//
//         if(diffSize != 0)
//         {
//             ConPrinter::Print("Unable to correctly distribute register blocks. Remaining registers: {}\n", diffSize);
//             assert(diffSize != 0);
//         }
//     }
//
//     [[nodiscard]] u16 GetRegisterFromSlab(const SlabSize slab, const u32 index) noexcept
//     {
//         if(slab == SlabSize::S2048)
//         {
//             return index << 11;
//         }
//
//         u32 offset = 0;
//
//         switch(slab)
//         {
//             case SlabSize::S1536:
//                 if(index > 3)
//                 {
//                     offset = 3;
//                 }
//                 break;
//             case SlabSize::S1024:
//                 if(index > 5)
//                 {
//                     offset = 5;
//                 }
//                 break;
//             case SlabSize::S768:
//                 if(index > 6)
//                 {
//                     offset = 6;
//                 }
//                 break;
//             case SlabSize::S512:
//                 if(index > 7)
//                 {
//                     offset = 7;
//                 }
//                 break;
//             default: break;
//         }
//
//         if(offset != 0)
//         {
//             return ((index - offset) << 8) + 2048;
//         }
//
//         return index * 256;
//     }
//
//     [[nodiscard]] u16 GetSlabFromRegister(const u16 registerBase) noexcept
//     {
//         if(registerBase > 2048)
//         {
//             return ((registerBase & 0x7FF) >> 8) + (registerBase >> 11);
//         }
//
//         return registerBase >> 8;
//     }
//
//     [[nodiscard]] RegisterBlockPair GetRegisterBlockPair(const SlabSize slab, const u32 index) noexcept
//     {
//         switch(slab)
//         {
//             case SlabSize::S2048: return m_2048Blocks[index];
//             case SlabSize::S1536: return m_1536Blocks[index];
//             case SlabSize::S1024: return m_1024Blocks[index];
//             case SlabSize::S768:  return m_768Blocks[index];
//             case SlabSize::S512:  return m_512Blocks[index];
//             case SlabSize::S256:  return m_256Blocks[index];
//             default: return {};
//         }
//     }
//
//     [[nodiscard]] u32 GetSlabSize(const SlabSize slab) noexcept
//     {
//         switch(slab)
//         {
//             case SlabSize::S2048: return 2048;
//             case SlabSize::S1536: return 1536;
//             case SlabSize::S1024: return 1024;
//             case SlabSize::S768:  return 768;
//             case SlabSize::S512:  return 512;
//             case SlabSize::S256:  return 256;
//             default: return 0;
//         }
//     }
//
//     void Free1536(const u16 registerBase)
//     {
//         const u16 slabIndex = GetSlabFromRegister(registerBase);
//
//         const u16 offset = slabIndex > 3 ? 3 : 0;
//
//         // Is this the head of the block?
//         if(slabIndex - offset == 0)
//         {
//             const u16 slab512 = GetSlabFromRegister(registerBase + 1536);
//
//             if(m_512Blocks[slab512].Available)
//             {
//                 m_512Blocks[slab512].Available = false;
//                 const u16 prevFree = m_512Blocks[slab512].PrevFree;
//                 const u16 nextFree = m_512Blocks[slab512].NextFree;
//
//                 if(slab512 == m_512Head)
//                 {
//                     m_512Head = nextFree;
//                 }
//                 else if(prevFree != 0x10)
//                 {
//                     m_512Blocks[prevFree].NextFree = nextFree;
//                 }
//
//                 if(nextFree != 0x10)
//                 {
//                     m_512Blocks[nextFree].PrevFree = prevFree;
//                 }
//
//                 m_2048Blocks[slabIndex].NextFree = m_2048Head;
//                 m_2048Blocks[slabIndex].PrevFree = 0x10;
//                 m_2048Blocks[m_2048Head].PrevFree = slabIndex;
//                 m_2048Head = slabIndex;
//                 return;
//             }
//         }
//         // Is this the tail of the block?
//         else if(slabIndex - offset == 2)
//         {
//             const u16 slab512 = GetSlabFromRegister(registerBase - 512);
//
//             if(m_512Blocks[slab512].Available)
//             {
//                 m_512Blocks[slab512].Available = false;
//                 const u16 prevFree = m_512Blocks[slab512].PrevFree;
//                 const u16 nextFree = m_512Blocks[slab512].NextFree;
//
//                 if(slab512 == m_512Head)
//                 {
//                     m_512Head = nextFree;
//                 }
//                 else if(prevFree != 0x10)
//                 {
//                     m_512Blocks[prevFree].NextFree = nextFree;
//                 }
//
//                 if(nextFree != 0x10)
//                 {
//                     m_512Blocks[nextFree].PrevFree = prevFree;
//                 }
//
//                 m_2048Blocks[slab512].NextFree = m_2048Head;
//                 m_2048Blocks[slab512].PrevFree = 0x10;
//                 m_2048Blocks[m_2048Head].PrevFree = slab512;
//                 m_2048Head = slab512;
//                 return;
//             }
//         }
//
//         // If we can't find a merge target, or we're in the middle of the block, just store into the 1536 list.
//
//         m_1536Blocks[slabIndex].NextFree = m_1536Head;
//         // m_1536Blocks[slabIndex].PrevFree = 0x10;
//         m_1536Blocks[m_1536Head].PrevFree = slabIndex;
//         m_1536Head = slabIndex;
//     }
//
//     void Free1024(const u16 registerBase)
//     {
//         const u16 slabIndex = GetSlabFromRegister(registerBase);
//
//         const u16 offset = slabIndex > 5 ? 5 : 0;
//
//         // Is this the head of the block?
//         if(slabIndex - offset == 0)
//         {
//             if(m_1024Blocks[slabIndex + 1].Available)
//             {
//                 m_1024Blocks[slabIndex + 1].Available = false;
//                 const u16 prevFree = m_1024Blocks[slabIndex + 1].PrevFree;
//                 const u16 nextFree = m_1024Blocks[slabIndex + 1].NextFree;
//
//                 if(slabIndex + 1 == m_1024Head)
//                 {
//                     m_1024Head = nextFree;
//                 }
//                 else if(prevFree != 0x10)
//                 {
//                     m_1024Blocks[prevFree].NextFree = nextFree;
//                 }
//
//                 if(nextFree != 0x10)
//                 {
//                     m_1024Blocks[nextFree].PrevFree = prevFree;
//                 }
//
//                 m_2048Blocks[slabIndex].NextFree = m_2048Head;
//                 m_2048Blocks[slabIndex].PrevFree = 0x10;
//                 m_2048Blocks[m_2048Head].PrevFree = slabIndex;
//                 m_2048Head = slabIndex;
//                 return;
//             }
//
//             const u16 slab512 = GetSlabFromRegister(registerBase + 1536);
//
//             if(m_512Blocks[slab512].Available)
//             {
//                 m_512Blocks[slab512].Available = false;
//                 const u16 prevFree = m_512Blocks[slab512].PrevFree;
//                 const u16 nextFree = m_512Blocks[slab512].NextFree;
//
//                 if(slab512 == m_512Head)
//                 {
//                     m_512Head = nextFree;
//                 }
//                 else if(prevFree != 0x10)
//                 {
//                     m_512Blocks[prevFree].NextFree = nextFree;
//                 }
//
//                 if(nextFree != 0x10)
//                 {
//                     m_512Blocks[nextFree].PrevFree = prevFree;
//                 }
//
//                 m_1536Blocks[slabIndex].NextFree = m_1536Head;
//                 m_1536Blocks[slabIndex].PrevFree = 0x10;
//                 m_1536Blocks[m_1536Head].PrevFree = slabIndex;
//                 m_1536Head = slabIndex;
//                 return;
//             }
//         }
//         // Is this the tail of the block?
//         else if(slabIndex - offset == 4)
//         {
//             if(m_1024Blocks[offset].Available)
//             {
//                 m_1024Blocks[offset].Available = false;
//                 const u16 prevFree = m_1024Blocks[offset].PrevFree;
//                 const u16 nextFree = m_1024Blocks[offset].NextFree;
//
//                 if(offset == m_1024Head)
//                 {
//                     m_1024Head = nextFree;
//                 }
//                 else if(prevFree != 0x10)
//                 {
//                     m_1024Blocks[prevFree].NextFree = nextFree;
//                 }
//
//                 if(nextFree != 0x10)
//                 {
//                     m_1024Blocks[nextFree].PrevFree = prevFree;
//                 }
//
//                 m_2048Blocks[offset].NextFree = m_2048Head;
//                 m_2048Blocks[offset].PrevFree = 0x10;
//                 m_2048Blocks[m_2048Head].PrevFree = offset;
//                 m_2048Head = offset;
//                 return;
//             }
//
//             const u16 slab512 = GetSlabFromRegister(registerBase - 512);
//
//             if(m_512Blocks[slab512].Available)
//             {
//                 m_512Blocks[slab512].Available = false;
//                 const u16 prevFree = m_512Blocks[slab512].PrevFree;
//                 const u16 nextFree = m_512Blocks[slab512].NextFree;
//
//                 if(slab512 == m_512Head)
//                 {
//                     m_512Head = nextFree;
//                 }
//                 else if(prevFree != 0x10)
//                 {
//                     m_512Blocks[prevFree].NextFree = nextFree;
//                 }
//
//                 if(nextFree != 0x10)
//                 {
//                     m_512Blocks[nextFree].PrevFree = prevFree;
//                 }
//
//                 m_2048Blocks[slab512].NextFree = m_2048Head;
//                 m_2048Blocks[slab512].PrevFree = 0x10;
//                 m_2048Blocks[m_2048Head].PrevFree = slab512;
//                 m_2048Head = slab512;
//                 return;
//             }
//         }
//
//         // If we can't find a merge target, or we're in the middle of the block, just store into the 1536 list.
//
//         m_1536Blocks[slabIndex].NextFree = m_1536Head;
//         // m_1536Blocks[slabIndex].PrevFree = 0x10;
//         m_1536Blocks[m_1536Head].PrevFree = slabIndex;
//         m_1536Head = slabIndex;
//     }
//
//     void Free192(const u16 registerBase) noexcept
//     {
//         // Is this the head of a 256 block, or the tail of a 256 block.
//         if((registerBase & 0xFF) == 0)
//         {
//             const u32 targetBase = registerBase + 192;
//
//             // We only need to check 64, as we don't have a slab list for 224.
//             if(m_64Tail != 0x8000)
//             {
//                 for(u32 i = 0; i <= m_64Tail; ++i)
//                 {
//                     // Is this the block we're looking for?
//                     if(m_64Blocks[i] != targetBase)
//                     {
//                         continue;
//                     }
//
//                     // Are we at the tail? If so we can just decrement the tail pointer.
//                     if(i == m_64Tail)
//                     {
//                         --m_64Tail;
//                     }
//                     else
//                     {
//                         // If the tail is 0, then we just set the pointer to its null value.
//                         if(m_64Tail == 0)
//                         {
//                             m_64Tail = 0x8000;
//                         }
//                         else
//                         {
//                             // Copy the tail to the block being removed.
//                             m_64Blocks[i] = m_64Blocks[m_64Tail];
//                             --m_64Tail;
//                         }
//                     }
//
//                     // Is the 256 tail null?
//                     if(m_256Tail == 0x8000)
//                     {
//                         // We'll store at block 0.
//                         m_256Tail = 0;
//                     }
//                     else
//                     {
//                         // Otherwise increment the tail.
//                         ++m_256Tail;
//                     }
//
//                     // Store the head of the block at the tail.
//                     m_256Blocks[m_256Tail] = registerBase;
//
//                     return;
//                 }
//             }
//         }
//         else
//         {
//             const u32 targetBase = registerBase - 64;
//
//             // We only need to check 64, as we don't have a slab list for 224.
//             if(m_64Tail != 0x8000)
//             {
//                 for(u32 i = 0; i <= m_64Tail; ++i)
//                 {
//                     // Is this the block we're looking for?
//                     if(m_64Blocks[i] != targetBase)
//                     {
//                         continue;
//                     }
//
//                     // Are we at the tail? If so we can just decrement the tail pointer.
//                     if(i == m_64Tail)
//                     {
//                         --m_64Tail;
//                     }
//                     else
//                     {
//                         // If the tail is 0, then we just set the pointer to its null value.
//                         if(m_64Tail == 0)
//                         {
//                             m_64Tail = 0x8000;
//                         }
//                         else
//                         {
//                             // Copy the tail to the block being removed.
//                             m_64Blocks[i] = m_64Blocks[m_64Tail];
//                             --m_64Tail;
//                         }
//                     }
//
//                     // Is the 256 tail null?
//                     if(m_256Tail == 0x8000)
//                     {
//                         // We'll store at block 0.
//                         m_256Tail = 0;
//                     }
//                     else
//                     {
//                         // Otherwise increment the tail.
//                         ++m_256Tail;
//                     }
//
//                     // Store the head of the block at the tail.
//                     m_256Blocks[m_256Tail] = static_cast<u16>(targetBase);
//
//                     return;
//                 }
//             }
//         }
//
//         // If we couldn't find an expansion slot, just store into the exact size slab.
//         if(m_192Tail == 0x8000)
//         {
//             // We'll store at block 0.
//             m_192Tail = 0;
//         }
//         else
//         {
//             // Otherwise increment the tail.
//             ++m_192Tail;
//         }
//
//         // Store the head of the block at the tail.
//         m_192Blocks[m_192Tail] = registerBase;
//     }
//
//     void Free128(const u16 registerBase) noexcept
//     {
//         // Is this the head of a 256 block?
//         if((registerBase & 0xFF) == 0)
//         {
//             const u32 targetBase = registerBase + 128;
//
//             if(m_128Tail != 0x8000)
//             {
//                 for(u32 i = 0; i <= m_128Tail; ++i)
//                 {
//                     // Is this the block we're looking for?
//                     if(m_128Blocks[i] != targetBase)
//                     {
//                         continue;
//                     }
//
//                     // Are we at the tail? If so we can just decrement the tail pointer.
//                     if(i == m_128Tail)
//                     {
//                         --m_128Tail;
//                     }
//                     else
//                     {
//                         // If the tail is 0, then we just set the pointer to its null value.
//                         if(m_128Tail == 0)
//                         {
//                             m_128Tail = 0x8000;
//                         }
//                         else
//                         {
//                             // Copy the tail to the block being removed.
//                             m_128Blocks[i] = m_128Blocks[m_128Tail];
//                             --m_128Tail;
//                         }
//                     }
//
//                     // Is the 256 tail null?
//                     if(m_256Tail == 0x8000)
//                     {
//                         // We'll store at block 0.
//                         m_256Tail = 0;
//                     }
//                     else
//                     {
//                         // Otherwise increment the tail.
//                         ++m_256Tail;
//                     }
//
//                     // Store the head of the block at the tail.
//                     m_256Blocks[m_256Tail] = registerBase;
//
//                     return;
//                 }
//             }
//
//             if(m_64Tail != 0x8000)
//             {
//                 for(u32 i = 0; i <= m_64Tail; ++i)
//                 {
//                     // Is this the block we're looking for?
//                     if(m_64Blocks[i] != targetBase)
//                     {
//                         continue;
//                     }
//
//                     // Are we at the tail? If so we can just decrement the tail pointer.
//                     if(i == m_64Tail)
//                     {
//                         --m_64Tail;
//                     }
//                     else
//                     {
//                         // If the tail is 0, then we just set the pointer to its null value.
//                         if(m_64Tail == 0)
//                         {
//                             m_64Tail = 0x8000;
//                         }
//                         else
//                         {
//                             // Copy the tail to the block being removed.
//                             m_64Blocks[i] = m_64Blocks[m_64Tail];
//                             --m_64Tail;
//                         }
//                     }
//
//                     // Is the 256 tail null?
//                     if(m_192Tail == 0x8000)
//                     {
//                         // We'll store at block 0.
//                         m_192Tail = 0;
//                     }
//                     else
//                     {
//                         // Otherwise increment the tail.
//                         ++m_192Tail;
//                     }
//
//                     // Store the head of the block at the tail.
//                     m_192Blocks[m_192Tail] = registerBase;
//
//                     return;
//                 }
//             }
//         }
//         // Is this the tail of a 256 block?
//         else if((registerBase & 0xFF) == 128)
//         {
//             if(m_128Tail != 0x8000)
//             {
//                 const u32 targetBase = registerBase - 128;
//
//                 for(u32 i = 0; i <= m_128Tail; ++i)
//                 {
//                     // Is this the block we're looking for?
//                     if(m_128Blocks[i] != targetBase)
//                     {
//                         continue;
//                     }
//
//                     // Are we at the tail? If so we can just decrement the tail pointer.
//                     if(i == m_128Tail)
//                     {
//                         --m_128Tail;
//                     }
//                     else
//                     {
//                         // If the tail is 0, then we just set the pointer to its null value.
//                         if(m_128Tail == 0)
//                         {
//                             m_128Tail = 0x8000;
//                         }
//                         else
//                         {
//                             // Copy the tail to the block being removed.
//                             m_128Blocks[i] = m_128Blocks[m_128Tail];
//                             --m_128Tail;
//                         }
//                     }
//
//                     // Is the 256 tail null?
//                     if(m_256Tail == 0x8000)
//                     {
//                         // We'll store at block 0.
//                         m_256Tail = 0;
//                     }
//                     else
//                     {
//                         // Otherwise increment the tail.
//                         ++m_256Tail;
//                     }
//
//                     // Store the head of the block at the tail.
//                     m_256Blocks[m_256Tail] = static_cast<u16>(targetBase);
//
//                     return;
//                 }
//             }
//
//             if(m_64Tail != 0x8000)
//             {
//                 const u32 targetBase = registerBase - 64;
//
//                 for(u32 i = 0; i <= m_64Tail; ++i)
//                 {
//                     // Is this the block we're looking for?
//                     if(m_64Blocks[i] != targetBase)
//                     {
//                         continue;
//                     }
//
//                     // Are we at the tail? If so we can just decrement the tail pointer.
//                     if(i == m_64Tail)
//                     {
//                         --m_64Tail;
//                     }
//                     else
//                     {
//                         // If the tail is 0, then we just set the pointer to its null value.
//                         if(m_64Tail == 0)
//                         {
//                             m_64Tail = 0x8000;
//                         }
//                         else
//                         {
//                             // Copy the tail to the block being removed.
//                             m_64Blocks[i] = m_64Blocks[m_64Tail];
//                             --m_64Tail;
//                         }
//                     }
//
//                     // Is the 256 tail null?
//                     if(m_192Tail == 0x8000)
//                     {
//                         // We'll store at block 0.
//                         m_192Tail = 0;
//                     }
//                     else
//                     {
//                         // Otherwise increment the tail.
//                         ++m_192Tail;
//                     }
//
//                     // Store the head of the block at the tail.
//                     m_256Blocks[m_192Tail] = static_cast<u16>(targetBase);
//
//                     return;
//                 }
//             }
//         }
//         // It must be in the middle of the 256 block.
//         else
//         {
//             const u32 targetBaseHead = registerBase - 64;
//             const u32 targetBaseTail = registerBase + 192;
//
//             if(m_64Tail != 0x8000)
//             {
//                 enum class FoundState : u8
//                 {
//                     FoundNone,
//                     FoundHead,
//                     FoundTail
//                 };
//
//                 FoundState found = FoundState::FoundNone;
//
//                 for(u32 i = 0; i <= m_64Tail; ++i)
//                 {
//                     const u16 block = m_64Blocks[i];
//
//                     // Is this the block we're looking for?
//                     if(block != targetBaseHead && block != targetBaseTail)
//                     {
//                         continue;
//                     }
//
//                     // Are we at the tail? If so we can just decrement the tail pointer.
//                     if(i == m_64Tail)
//                     {
//                         --m_64Tail;
//                     }
//                     else
//                     {
//                         // If the tail is 0, then we just set the pointer to its null value.
//                         if(m_64Tail == 0)
//                         {
//                             m_64Tail = 0x8000;
//                         }
//                         else
//                         {
//                             // Copy the tail to the block being removed.
//                             m_64Blocks[i] = m_64Blocks[m_64Tail];
//                             --m_64Tail;
//                         }
//                     }
//
//                     // If we've already found a tail or head, and we just found another tail/head,
//                     // we know that it is a full 256 block and we can be done.
//                     if(found != FoundState::FoundNone)
//                     {
//                         // Is the 256 tail null?
//                         if(m_256Tail == 0x8000)
//                         {
//                             // We'll store at block 0.
//                             m_256Tail = 0;
//                         }
//                         else
//                         {
//                             // Otherwise increment the tail.
//                             ++m_256Tail;
//                         }
//
//                         // Store the head of the block at the tail.
//                         m_256Blocks[m_256Tail] = static_cast<u16>(targetBaseHead);
//
//                         return;
//                     }
//
//                     // Is this the head block?
//                     if(block == targetBaseHead)
//                     {
//                         found = FoundState::FoundHead;
//                     }
//                     // It must be the tail block.
//                     else
//                     {
//                         found = FoundState::FoundTail;
//                     }
//                 }
//
//                 if(found == FoundState::FoundHead)
//                 {
//                     // Is the 192 tail null?
//                     if(m_192Tail == 0x8000)
//                     {
//                         // We'll store at block 0.
//                         m_192Tail = 0;
//                     }
//                     else
//                     {
//                         // Otherwise increment the tail.
//                         ++m_192Tail;
//                     }
//
//                     // Store the head of the block at the tail.
//                     m_192Blocks[m_192Tail] = static_cast<u16>(targetBaseHead);
//
//                     return;
//                 }
//                 else if(found == FoundState::FoundTail)
//                 {
//                     // Is the 192 tail null?
//                     if(m_192Tail == 0x8000)
//                     {
//                         // We'll store at block 0.
//                         m_192Tail = 0;
//                     }
//                     else
//                     {
//                         // Otherwise increment the tail.
//                         ++m_192Tail;
//                     }
//
//                     // Store the head of the block at the tail.
//                     m_192Blocks[m_192Tail] = registerBase;
//
//                     return;
//                 }
//             }
//         }
//
//         // If we couldn't find an expansion slot, just store into the exact size slab.
//         if(m_128Tail == 0x8000)
//         {
//             // We'll store at block 0.
//             m_128Tail = 0;
//         }
//         else
//         {
//             // Otherwise increment the tail.
//             ++m_128Tail;
//         }
//
//         // Store the head of the block at the tail.
//         m_128Blocks[m_128Tail] = registerBase;
//     }
//
//     void Free128Pure(const u16 registerBase) noexcept
//     {
//         // Is this the head of a 256 block?
//         if((registerBase & 0xFF) == 0)
//         {
//             const u32 targetBase = registerBase + 128;
//
//             if(m_128Tail != 0x8000)
//             {
//                 for(u32 i = 0; i <= m_128Tail; ++i)
//                 {
//                     // Is this the block we're looking for?
//                     if(m_128Blocks[i] != targetBase)
//                     {
//                         continue;
//                     }
//
//                     // Are we at the tail? If so we can just decrement the tail pointer.
//                     if(i == m_128Tail)
//                     {
//                         --m_128Tail;
//                     }
//                     else
//                     {
//                         // If the tail is 0, then we just set the pointer to its null value.
//                         if(m_128Tail == 0)
//                         {
//                             m_128Tail = 0x8000;
//                         }
//                         else
//                         {
//                             // Copy the tail to the block being removed.
//                             m_128Blocks[i] = m_128Blocks[m_128Tail];
//                             --m_128Tail;
//                         }
//                     }
//
//                     // Is the 256 tail null?
//                     if(m_256Tail == 0x8000)
//                     {
//                         // We'll store at block 0.
//                         m_256Tail = 0;
//                     }
//                     else
//                     {
//                         // Otherwise increment the tail.
//                         ++m_256Tail;
//                     }
//
//                     // Store the head of the block at the tail.
//                     m_256Blocks[m_256Tail] = registerBase;
//
//                     return;
//                 }
//             }
//         }
//         // Is this the tail of a 256 block?
//         else if((registerBase & 0xFF) == 128)
//         {
//             const u32 targetBase = registerBase - 128;
//
//             if(m_128Tail != 0x8000)
//             {
//                 for(u32 i = 0; i <= m_128Tail; ++i)
//                 {
//                     // Is this the block we're looking for?
//                     if(m_128Blocks[i] != targetBase)
//                     {
//                         continue;
//                     }
//
//                     // Are we at the tail? If so we can just decrement the tail pointer.
//                     if(i == m_128Tail)
//                     {
//                         --m_128Tail;
//                     }
//                     else
//                     {
//                         // If the tail is 0, then we just set the pointer to its null value.
//                         if(m_128Tail == 0)
//                         {
//                             m_128Tail = 0x8000;
//                         }
//                         else
//                         {
//                             // Copy the tail to the block being removed.
//                             m_128Blocks[i] = m_128Blocks[m_128Tail];
//                             --m_128Tail;
//                         }
//                     }
//
//                     // Is the 256 tail null?
//                     if(m_256Tail == 0x8000)
//                     {
//                         // We'll store at block 0.
//                         m_256Tail = 0;
//                     }
//                     else
//                     {
//                         // Otherwise increment the tail.
//                         ++m_256Tail;
//                     }
//
//                     // Store the head of the block at the tail.
//                     m_256Blocks[m_256Tail] = static_cast<u16>(targetBase);
//
//                     return;
//                 }
//             }
//         }
//         // It must be in the middle of the 256 block.
//         else
//         {
//             const u32 targetBaseHead = registerBase - 64;
//             const u32 targetBaseTail = registerBase + 192;
//
//             if(m_64Tail != 0x8000)
//             {
//                 enum class FoundState : u8
//                 {
//                     FoundNone,
//                     FoundHead,
//                     FoundTail
//                 };
//
//                 FoundState found = FoundState::FoundNone;
//
//                 for(u32 i = 0; i <= m_64Tail; ++i)
//                 {
//                     const u16 block = m_64Blocks[i];
//
//                     // Is this the block we're looking for?
//                     if(block != targetBaseHead && block != targetBaseTail)
//                     {
//                         continue;
//                     }
//
//                     // Are we at the tail? If so we can just decrement the tail pointer.
//                     if(i == m_64Tail)
//                     {
//                         --m_64Tail;
//                     }
//                     else
//                     {
//                         // If the tail is 0, then we just set the pointer to its null value.
//                         if(m_64Tail == 0)
//                         {
//                             m_64Tail = 0x8000;
//                         }
//                         else
//                         {
//                             // Copy the tail to the block being removed.
//                             m_64Blocks[i] = m_64Blocks[m_64Tail];
//                             --m_64Tail;
//                         }
//                     }
//
//                     // If we've already found a tail or head, and we just found another tail/head,
//                     // we know that it is a full 256 block and we can be done.
//                     if(found != FoundState::FoundNone)
//                     {
//                         // Is the 256 tail null?
//                         if(m_256Tail == 0x8000)
//                         {
//                             // We'll store at block 0.
//                             m_256Tail = 0;
//                         }
//                         else
//                         {
//                             // Otherwise increment the tail.
//                             ++m_256Tail;
//                         }
//
//                         // Store the head of the block at the tail.
//                         m_256Blocks[m_256Tail] = static_cast<u16>(targetBaseHead);
//
//                         return;
//                     }
//
//                     // Is this the head block?
//                     if(block == targetBaseHead)
//                     {
//                         found = FoundState::FoundHead;
//                     }
//                     // It must be the tail block.
//                     else
//                     {
//                         found = FoundState::FoundTail;
//                     }
//                 }
//
//                 if(found == FoundState::FoundHead)
//                 {
//                     // Is the 192 tail null?
//                     if(m_192Tail == 0x8000)
//                     {
//                         // We'll store at block 0.
//                         m_192Tail = 0;
//                     }
//                     else
//                     {
//                         // Otherwise increment the tail.
//                         ++m_192Tail;
//                     }
//
//                     // Store the head of the block at the tail.
//                     m_192Blocks[m_192Tail] = static_cast<u16>(targetBaseHead);
//
//                     return;
//                 }
//                 else if(found == FoundState::FoundTail)
//                 {
//                     // Is the 192 tail null?
//                     if(m_192Tail == 0x8000)
//                     {
//                         // We'll store at block 0.
//                         m_192Tail = 0;
//                     }
//                     else
//                     {
//                         // Otherwise increment the tail.
//                         ++m_192Tail;
//                     }
//
//                     // Store the head of the block at the tail.
//                     m_192Blocks[m_192Tail] = registerBase;
//
//                     return;
//                 }
//             }
//         }
//
//         // If we couldn't find an expansion slot, just store into the exact size slab.
//         if(m_128Tail == 0x8000)
//         {
//             // We'll store at block 0.
//             m_128Tail = 0;
//         }
//         else
//         {
//             // Otherwise increment the tail.
//             ++m_128Tail;
//         }
//
//         // Store the head of the block at the tail.
//         m_128Blocks[m_128Tail] = registerBase;
//     }
//
//     void Free96(const u16 registerBase) noexcept
//     {
//         // Is this the head of a 256 block, or the tail of a 256 block.
//         if((registerBase & 0xFF) == 0)
//         {
//             const u32 targetBase = registerBase + 96;
//
//             // We only need to check 64, as we don't have a slab list for 224.
//             if(m_32Tail != 0x8000)
//             {
//                 for(u32 i = 0; i <= m_32Tail; ++i)
//                 {
//                     // Is this the block we're looking for?
//                     if(m_32Blocks[i] != targetBase)
//                     {
//                         continue;
//                     }
//
//                     // Are we at the tail? If so we can just decrement the tail pointer.
//                     if(i == m_32Tail)
//                     {
//                         --m_32Tail;
//                     }
//                     else
//                     {
//                         // If the tail is 0, then we just set the pointer to its null value.
//                         if(m_32Tail == 0)
//                         {
//                             m_32Tail = 0x8000;
//                         }
//                         else
//                         {
//                             // Copy the tail to the block being removed.
//                             m_32Blocks[i] = m_32Blocks[m_32Tail];
//                             --m_32Tail;
//                         }
//                     }
//
//                     Free128(registerBase);
//                     return;
//                 }
//             }
//         }
//         // Is this the tail of 256 block?
//         else if((registerBase & 0xFF) == 160)
//         {
//             const u32 targetBase = registerBase - 32;
//
//             // We only need to check 64, as we don't have a slab list for 224.
//             if(m_32Tail != 0x8000)
//             {
//                 for(u32 i = 0; i <= m_32Tail; ++i)
//                 {
//                     // Is this the block we're looking for?
//                     if(m_32Blocks[i] != targetBase)
//                     {
//                         continue;
//                     }
//
//                     // Are we at the tail? If so we can just decrement the tail pointer.
//                     if(i == m_32Tail)
//                     {
//                         --m_32Tail;
//                     }
//                     else
//                     {
//                         // If the tail is 0, then we just set the pointer to its null value.
//                         if(m_32Tail == 0)
//                         {
//                             m_32Tail = 0x8000;
//                         }
//                         else
//                         {
//                             // Copy the tail to the block being removed.
//                             m_32Blocks[i] = m_32Blocks[m_32Tail];
//                             --m_32Tail;
//                         }
//                     }
//
//                     Free128(static_cast<u16>(targetBase));
//                     return;
//                 }
//             }
//         }
//         else
//         {
//             const u32 targetBaseHead = registerBase + 96;
//             const u32 targetBaseTail = registerBase - 32;
//
//             // We only need to check 64, as we don't have a slab list for 224.
//             if(m_32Tail != 0x8000)
//             {
//                 for(u32 i = 0; i <= m_32Tail; ++i)
//                 {
//                     const u16 block = m_32Blocks[i];
//
//                     // Is this the block we're looking for?
//                     if(block != targetBaseHead && block != targetBaseTail)
//                     {
//                         continue;
//                     }
//
//                     // Are we at the tail? If so we can just decrement the tail pointer.
//                     if(i == m_32Tail)
//                     {
//                         --m_32Tail;
//                     }
//                     else
//                     {
//                         // If the tail is 0, then we just set the pointer to its null value.
//                         if(m_32Tail == 0)
//                         {
//                             m_32Tail = 0x8000;
//                         }
//                         else
//                         {
//                             // Copy the tail to the block being removed.
//                             m_32Blocks[i] = m_32Blocks[m_32Tail];
//                             --m_32Tail;
//                         }
//                     }
//
//                     if(block == targetBaseTail)
//                     {
//                         Free128(static_cast<u16>(targetBaseTail));
//                     }
//                     else
//                     {
//                         Free128(registerBase);
//                     }
//
//                     return;
//                 }
//             }
//         }
//
//         // If we couldn't find an expansion slot, just store into the exact size slab.
//         if(m_96Tail == 0x8000)
//         {
//             // We'll store at block 0.
//             m_96Tail = 0;
//         }
//         else
//         {
//             // Otherwise increment the tail.
//             ++m_96Tail;
//         }
//
//         // Store the head of the block at the tail.
//         m_96Blocks[m_96Tail] = registerBase;
//     }
//
//     void Free64(const u16 registerBase) noexcept
//     {
//         // Is this the head of a 256 block, or the tail of a 256 block.
//         if((registerBase & 0xFF) == 0)
//         {
//             const u32 targetBase = registerBase + 64;
//
//             if(m_192Tail != 0x8000)
//             {
//                 for(u32 i = 0; i <= m_192Tail; ++i)
//                 {
//                     // Is this the block we're looking for?
//                     if(m_192Blocks[i] != targetBase)
//                     {
//                         continue;
//                     }
//
//                     // Are we at the tail? If so we can just decrement the tail pointer.
//                     if(i == m_192Tail)
//                     {
//                         --m_192Tail;
//                     }
//                     else
//                     {
//                         // If the tail is 0, then we just set the pointer to its null value.
//                         if(m_192Tail == 0)
//                         {
//                             m_192Tail = 0x8000;
//                         }
//                         else
//                         {
//                             // Copy the tail to the block being removed.
//                             m_192Blocks[i] = m_192Blocks[m_192Tail];
//                             --m_192Tail;
//                         }
//                     }
//                     
//                     if(m_256Tail == 0x8000)
//                     {
//                         // We'll store at block 0.
//                         m_256Tail = 0;
//                     }
//                     else
//                     {
//                         // Otherwise increment the tail.
//                         ++m_256Tail;
//                     }
//
//                     // Store the head of the block at the tail.
//                     m_256Blocks[m_256Tail] = registerBase;
//
//                     return;
//                 }
//             }
//
//             if(m_128Tail != 0x8000)
//             {
//                 for(u32 i = 0; i <= m_128Tail; ++i)
//                 {
//                     // Is this the block we're looking for?
//                     if(m_128Blocks[i] != targetBase)
//                     {
//                         continue;
//                     }
//
//                     // Are we at the tail? If so we can just decrement the tail pointer.
//                     if(i == m_128Tail)
//                     {
//                         --m_128Tail;
//                     }
//                     else
//                     {
//                         // If the tail is 0, then we just set the pointer to its null value.
//                         if(m_128Tail == 0)
//                         {
//                             m_128Tail = 0x8000;
//                         }
//                         else
//                         {
//                             // Copy the tail to the block being removed.
//                             m_128Blocks[i] = m_128Blocks[m_128Tail];
//                             --m_128Tail;
//                         }
//                     }
//
//                     // We'll simply store the 192. If there was another 64 block after the 128, then it would have already been merged into a 192 block.
//                     if(m_192Tail == 0x8000)
//                     {
//                         // We'll store at block 0.
//                         m_192Tail = 0;
//                     }
//                     else
//                     {
//                         // Otherwise increment the tail.
//                         ++m_192Tail;
//                     }
//
//                     // Store the head of the block at the tail.
//                     m_192Blocks[m_192Tail] = registerBase;
//
//                     return;
//                 }
//             }
//             
//             if(m_64Tail != 0x8000)
//             {
//                 for(u32 i = 0; i <= m_64Tail; ++i)
//                 {
//                     // Is this the block we're looking for?
//                     if(m_64Blocks[i] != targetBase)
//                     {
//                         continue;
//                     }
//
//                     // Are we at the tail? If so we can just decrement the tail pointer.
//                     if(i == m_64Tail)
//                     {
//                         --m_64Tail;
//                     }
//                     else
//                     {
//                         // If the tail is 0, then we just set the pointer to its null value.
//                         if(m_64Tail == 0)
//                         {
//                             m_64Tail = 0x8000;
//                         }
//                         else
//                         {
//                             // Copy the tail to the block being removed.
//                             m_64Blocks[i] = m_64Blocks[m_64Tail];
//                             --m_64Tail;
//                         }
//                     }
//
//                     // The pure free will only check for a 128 block. If there was another 64 block it would have already been merged into a 128 block.
//                     Free128Pure(registerBase);
//                     return;
//                 }
//             }
//             
//             if(m_32Tail != 0x8000)
//             {
//                 for(u32 i = 0; i <= m_32Tail; ++i)
//                 {
//                     // Is this the block we're looking for?
//                     if(m_32Blocks[i] != targetBase)
//                     {
//                         continue;
//                     }
//
//                     // Are we at the tail? If so we can just decrement the tail pointer.
//                     if(i == m_32Tail)
//                     {
//                         --m_32Tail;
//                     }
//                     else
//                     {
//                         // If the tail is 0, then we just set the pointer to its null value.
//                         if(m_32Tail == 0)
//                         {
//                             m_32Tail = 0x8000;
//                         }
//                         else
//                         {
//                             // Copy the tail to the block being removed.
//                             m_32Blocks[i] = m_32Blocks[m_32Tail];
//                             --m_32Tail;
//                         }
//                     }
//
//                     // We'll simply store the 96. If there was another 32 block after the 96, then it would have already been merged into a 128 block.
//                     if(m_96Tail == 0x8000)
//                     {
//                         // We'll store at block 0.
//                         m_96Tail = 0;
//                     }
//                     else
//                     {
//                         // Otherwise increment the tail.
//                         ++m_96Tail;
//                     }
//
//                     // Store the head of the block at the tail.
//                     m_96Blocks[m_96Tail] = registerBase;
//
//                     return;
//                 }
//             }
//         }
//         // Is this the tail of 256 block?
//         else if((registerBase & 0xFF) == 192)
//         {
//             if(m_192Tail != 0x8000)
//             {
//                 const u32 targetBase = registerBase - 192;
//
//                 for(u32 i = 0; i <= m_192Tail; ++i)
//                 {
//                     // Is this the block we're looking for?
//                     if(m_192Blocks[i] != targetBase)
//                     {
//                         continue;
//                     }
//
//                     // Are we at the tail? If so we can just decrement the tail pointer.
//                     if(i == m_192Tail)
//                     {
//                         --m_192Tail;
//                     }
//                     else
//                     {
//                         // If the tail is 0, then we just set the pointer to its null value.
//                         if(m_192Tail == 0)
//                         {
//                             m_192Tail = 0x8000;
//                         }
//                         else
//                         {
//                             // Copy the tail to the block being removed.
//                             m_192Blocks[i] = m_192Blocks[m_192Tail];
//                             --m_192Tail;
//                         }
//                     }
//
//                     if(m_256Tail == 0x8000)
//                     {
//                         // We'll store at block 0.
//                         m_256Tail = 0;
//                     }
//                     else
//                     {
//                         // Otherwise increment the tail.
//                         ++m_256Tail;
//                     }
//
//                     // Store the head of the block at the tail.
//                     m_256Blocks[m_256Tail] = static_cast<u16>(targetBase);
//
//                     return;
//                 }
//             }
//
//             if(m_128Tail != 0x8000)
//             {
//                 const u32 targetBase = registerBase - 128;
//
//                 for(u32 i = 0; i <= m_128Tail; ++i)
//                 {
//                     // Is this the block we're looking for?
//                     if(m_128Blocks[i] != targetBase)
//                     {
//                         continue;
//                     }
//
//                     // Are we at the tail? If so we can just decrement the tail pointer.
//                     if(i == m_128Tail)
//                     {
//                         --m_128Tail;
//                     }
//                     else
//                     {
//                         // If the tail is 0, then we just set the pointer to its null value.
//                         if(m_128Tail == 0)
//                         {
//                             m_128Tail = 0x8000;
//                         }
//                         else
//                         {
//                             // Copy the tail to the block being removed.
//                             m_128Blocks[i] = m_128Blocks[m_128Tail];
//                             --m_128Tail;
//                         }
//                     }
//
//                     // We'll simply store the 192. If there was another 64 block before the 128, then it would have already been merged into a 192 block.
//                     if(m_192Tail == 0x8000)
//                     {
//                         // We'll store at block 0.
//                         m_192Tail = 0;
//                     }
//                     else
//                     {
//                         // Otherwise increment the tail.
//                         ++m_192Tail;
//                     }
//
//                     // Store the head of the block at the tail.
//                     m_192Blocks[m_192Tail] = static_cast<u16>(targetBase);
//
//                     return;
//                 }
//             }
//
//             if(m_64Tail != 0x8000)
//             {
//                 const u32 targetBase = registerBase - 64;
//
//                 for(u32 i = 0; i <= m_64Tail; ++i)
//                 {
//                     // Is this the block we're looking for?
//                     if(m_64Blocks[i] != targetBase)
//                     {
//                         continue;
//                     }
//
//                     // Are we at the tail? If so we can just decrement the tail pointer.
//                     if(i == m_64Tail)
//                     {
//                         --m_64Tail;
//                     }
//                     else
//                     {
//                         // If the tail is 0, then we just set the pointer to its null value.
//                         if(m_64Tail == 0)
//                         {
//                             m_64Tail = 0x8000;
//                         }
//                         else
//                         {
//                             // Copy the tail to the block being removed.
//                             m_64Blocks[i] = m_64Blocks[m_64Tail];
//                             --m_64Tail;
//                         }
//                     }
//
//                     // The pure free will only check for a 128 block. If there was another 64 block it would have already been merged into a 128 block.
//                     Free128Pure(static_cast<u16>(targetBase));
//                     return;
//                 }
//             }
//
//             if(m_32Tail != 0x8000)
//             {
//                 const u32 targetBase = registerBase - 32;
//
//                 for(u32 i = 0; i <= m_32Tail; ++i)
//                 {
//                     // Is this the block we're looking for?
//                     if(m_32Blocks[i] != targetBase)
//                     {
//                         continue;
//                     }
//
//                     // Are we at the tail? If so we can just decrement the tail pointer.
//                     if(i == m_32Tail)
//                     {
//                         --m_32Tail;
//                     }
//                     else
//                     {
//                         // If the tail is 0, then we just set the pointer to its null value.
//                         if(m_32Tail == 0)
//                         {
//                             m_32Tail = 0x8000;
//                         }
//                         else
//                         {
//                             // Copy the tail to the block being removed.
//                             m_32Blocks[i] = m_32Blocks[m_32Tail];
//                             --m_32Tail;
//                         }
//                     }
//
//                     // We'll simply store the 96. If there was another 32 block before the 96, then it would have already been merged into a 128 block.
//                     if(m_96Tail == 0x8000)
//                     {
//                         // We'll store at block 0.
//                         m_96Tail = 0;
//                     }
//                     else
//                     {
//                         // Otherwise increment the tail.
//                         ++m_96Tail;
//                     }
//
//                     // Store the head of the block at the tail.
//                     m_96Blocks[m_96Tail] = static_cast<u16>(targetBase);
//
//                     return;
//                 }
//             }
//         }
//         else
//         {
//             
//         }
//
//         // If we couldn't find an expansion slot, just store into the exact size slab.
//         if(m_64Tail == 0x8000)
//         {
//             // We'll store at block 0.
//             m_64Tail = 0;
//         }
//         else
//         {
//             // Otherwise increment the tail.
//             ++m_64Tail;
//         }
//
//         // Store the head of the block at the tail.
//         m_64Blocks[m_64Tail] = registerBase;
//     }
// private:
//     RegisterBlockPair m_2048Blocks[1 * 2];
//     RegisterBlockPair m_1536Blocks[3 * 2];
//     RegisterBlockPair m_1024Blocks[5 * 2];
//     RegisterBlockPair m_768Blocks[6 * 2];
//     RegisterBlockPair m_512Blocks[7 * 2];
//     RegisterBlockPair m_256Blocks[8 * 2];
//     // u16 m_192Blocks[REGISTER_FILE_REGISTER_COUNT / 192];
//     // u16 m_128Blocks[REGISTER_FILE_REGISTER_COUNT / 128];
//     // u16 m_96Blocks[REGISTER_FILE_REGISTER_COUNT / 96];
//     // u16 m_64Blocks[REGISTER_FILE_REGISTER_COUNT / 64];
//     // u16 m_32Blocks[REGISTER_FILE_REGISTER_COUNT / 32];
//
//     u16 m_2048Head;
//     u16 m_1536Head;
//     u16 m_1024Head;
//     u16 m_768Head;
//     u16 m_512Head;
//     u16 m_256Head;
//     // u16 m_192Tail;
//     // u16 m_128Tail;
//     // u16 m_96Tail;
//     // u16 m_64Tail;
//     // u16 m_32Tail;
// };
