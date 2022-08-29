#include <ConPrinter.hpp>

#include <RegisterAllocator.hpp>

template<u16 RegisterCount>
static void TestAllocSingle() noexcept;

static void TestStochasticInOrderAlloc() noexcept;
static void TestStochasticMixedOrderAlloc() noexcept;

namespace tau::test {

void RunTests() noexcept
{
    TestAllocSingle<2048>();
    TestAllocSingle<2047>();
    TestAllocSingle<1537>();
    TestAllocSingle<1536>();
    TestAllocSingle<1535>();
    TestAllocSingle<1025>();
    TestAllocSingle<1024>();
    TestAllocSingle<1023>();
    TestAllocSingle<769>();
    TestAllocSingle<768>();
    TestAllocSingle<767>();
    TestAllocSingle<513>();
    TestAllocSingle<512>();
    TestAllocSingle<511>();
    TestAllocSingle<257>();
    TestAllocSingle<256>();
    TestAllocSingle<255>();

    TestStochasticInOrderAlloc();
    TestStochasticMixedOrderAlloc();
}

}

template<u16 RegisterCount>
static void TestAllocSingle() noexcept
{
    RegisterAllocator allocator;

    const u16 baseRegister = allocator.AllocateRegisterBlock(RegisterCount - 1);

    if(baseRegister == 0xFFFF)
    {
        ConPrinter::PrintLn("Failed to allocate {} block.", RegisterCount);
    }

    allocator.FreeRegisterBlock(baseRegister, RegisterCount - 1);

    if(!allocator.CheckFree())
    {
        ConPrinter::PrintLn("Failed to free single {} block.", RegisterCount);
    }
    else
    {
        ConPrinter::PrintLn("Successfully allocated and freed single {} block.", RegisterCount);
    }
}

static void TestStochasticInOrderAlloc() noexcept
{
    RegisterAllocator allocator;

    const u16 block0 = allocator.AllocateRegisterBlock(22);
    const u16 block1 = allocator.AllocateRegisterBlock(699);
    const u16 block2 = allocator.AllocateRegisterBlock(259);
    const u16 block3 = allocator.AllocateRegisterBlock(999);
    const u16 block4 = allocator.AllocateRegisterBlock(29);

    if(block0 == 0xFFFF)
    {
        ConPrinter::PrintLn("Failed to allocate 23 registers for in-order block 0.");
    }

    if(block1 == 0xFFFF)
    {
        ConPrinter::PrintLn("Failed to allocate 700 registers for in-order block 1.");
    }

    if(block2 == 0xFFFF)
    {
        ConPrinter::PrintLn("Failed to allocate 260 registers for in-order block 2.");
    }

    if(block3 == 0xFFFF)
    {
        ConPrinter::PrintLn("Failed to allocate 1000 registers for in-order block 3.");
    }

    if(block4 == 0xFFFF)
    {
        ConPrinter::PrintLn("Failed to allocate 30 registers for in-order block 4.");
    }

    if(block0 == block1 || block0 == block2 || block0 == block3 || block0 == block4)
    {
        ConPrinter::PrintLn("Overlapping block on block 0 detected.");
    }

    if(block1 == block2 || block1 == block3 || block1 == block4)
    {
        ConPrinter::PrintLn("Overlapping block on block 1 detected.");
    }

    if(block2 == block3 || block2 == block4)
    {
        ConPrinter::PrintLn("Overlapping block on block 2 detected.");
    }

    if(block3 == block4)
    {
        ConPrinter::PrintLn("Overlapping block on block 3 detected.");
    }

    allocator.FreeRegisterBlock(block0, 22);
    allocator.FreeRegisterBlock(block1, 699);
    allocator.FreeRegisterBlock(block2, 259);
    allocator.FreeRegisterBlock(block3, 999);
    allocator.FreeRegisterBlock(block4, 29);

    if(!allocator.CheckFree())
    {
        ConPrinter::PrintLn("Failed to free stochastic, in-order, blocks.");
    }
    else
    {
        ConPrinter::PrintLn("Successfully allocated and freed stochastic, in-order, blocks.");
    }
}

static void TestStochasticMixedOrderAlloc() noexcept
{
    RegisterAllocator allocator;

    const u16 block0 = allocator.AllocateRegisterBlock(22);
    const u16 block1 = allocator.AllocateRegisterBlock(699);
    const u16 block2 = allocator.AllocateRegisterBlock(259);

    if(block0 == 0xFFFF)
    {
        ConPrinter::PrintLn("Failed to allocate 23 registers for mixed-order block 0.");
    }

    if(block1 == 0xFFFF)
    {
        ConPrinter::PrintLn("Failed to allocate 700 registers for mixed-order block 1.");
    }

    if(block2 == 0xFFFF)
    {
        ConPrinter::PrintLn("Failed to allocate 260 registers for mixed-order block 2.");
    }

    allocator.FreeRegisterBlock(block1, 699);

    const u16 block3 = allocator.AllocateRegisterBlock(999);

    if(block3 == 0xFFFF)
    {
        ConPrinter::PrintLn("Failed to allocate 1000 registers for mixed-order block 3.");
    }

    allocator.FreeRegisterBlock(block0, 22);

    const u16 block4 = allocator.AllocateRegisterBlock(29);

    if(block4 == 0xFFFF)
    {
        ConPrinter::PrintLn("Failed to allocate 30 registers for mixed-order block 4.");
    }

    allocator.FreeRegisterBlock(block2, 259);
    allocator.FreeRegisterBlock(block3, 999);
    allocator.FreeRegisterBlock(block4, 29);

    if(!allocator.CheckFree())
    {
        ConPrinter::PrintLn("Failed to free stochastic, mixed-order, blocks.");
    }
    else
    {
        ConPrinter::PrintLn("Successfully allocated and freed stochastic, mixed-order, blocks.");
    }
}
