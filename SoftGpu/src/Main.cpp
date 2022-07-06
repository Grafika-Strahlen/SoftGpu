#include "Processor.hpp"
#include <ConPrinter.hpp>

static Processor processor;

static void TestMove() noexcept;
static void TestAdd1F() noexcept;
static void TestAdd2F() noexcept;
static void TestAdd2FReplicated() noexcept;
static void TestMul2FReplicated() noexcept;
static void TestMul2FReplicatedDualDispatch() noexcept;

int main(int argCount, char* args[])
{
    Console::Init();

    // TestMove();
    // TestAdd1F();
    // TestAdd2F();
    // TestAdd2FReplicated();
    // TestMul2FReplicated();
    TestMul2FReplicatedDualDispatch();

    return 0;
}

static void TestMove() noexcept
{
    u32 initialValue = 42;
    u32 storageLocation = 0xCDCDCDCD;

    const uintptr_t initialValuePtr = reinterpret_cast<uintptr_t>(&initialValue) >> 2;
    u8 initialValuePtrBytes[8];
    (void) ::std::memcpy(initialValuePtrBytes, &initialValuePtr, sizeof(initialValuePtr));

    const uintptr_t storageLocationPtr = reinterpret_cast<uintptr_t>(&storageLocation) >> 2;
    u8 storageLocationPtrBytes[8];
    (void) ::std::memcpy(storageLocationPtrBytes, &storageLocationPtr, sizeof(storageLocationPtr));

    u8 program[] =
    {
        static_cast<u8>(EInstruction::LoadImmediate), 0, initialValuePtrBytes[0], initialValuePtrBytes[1], initialValuePtrBytes[2], initialValuePtrBytes[3],
        static_cast<u8>(EInstruction::LoadImmediate), 1, initialValuePtrBytes[4], initialValuePtrBytes[5], initialValuePtrBytes[6], initialValuePtrBytes[7],
        static_cast<u8>(EInstruction::LoadImmediate), 2, storageLocationPtrBytes[0], storageLocationPtrBytes[1], storageLocationPtrBytes[2], storageLocationPtrBytes[3],
        static_cast<u8>(EInstruction::LoadImmediate), 3, storageLocationPtrBytes[4], storageLocationPtrBytes[5], storageLocationPtrBytes[6], storageLocationPtrBytes[7],
        static_cast<u8>(EInstruction::LoadStore), 0b00111000, 0, 4, 0, 0,
        static_cast<u8>(EInstruction::LoadStore), 0b01111000, 2, 4, 0, 0,
        static_cast<u8>(EInstruction::FlushCache),
        static_cast<u8>(EInstruction::Hlt)
    };

    ConPrinter::Print("Value to copy: {}\n", initialValue);
    ConPrinter::Print("Value at storage location: {}\n", storageLocation);

    processor.TestLoadProgram(0, 0, 0x1, program);

    for(int i = 0; i < 20; ++i)
    {
        processor.Clock();
    }

    ConPrinter::Print("Value to copy: {}\n", initialValue);
    ConPrinter::Print("Value at storage location: {}\n", storageLocation);

}

static void TestAdd1F() noexcept
{
    f32 valueA = 1.5f;
    const f32 valueB = 2.5f;
    f32 storageValue = ::std::numeric_limits<f32>::quiet_NaN();

    const uintptr_t valueAPtr = reinterpret_cast<uintptr_t>(&valueA) >> 2;
    u8 valueAPtrBytes[8];
    (void) ::std::memcpy(valueAPtrBytes, &valueAPtr, sizeof(valueAPtr));
    
    u8 valueBBytes[8];
    (void) ::std::memcpy(valueBBytes, &valueB, sizeof(valueB));

    const uintptr_t storageLocationPtr = reinterpret_cast<uintptr_t>(&storageValue) >> 2;
    u8 storageLocationPtrBytes[8];
    (void) ::std::memcpy(storageLocationPtrBytes, &storageLocationPtr, sizeof(storageLocationPtr));

    u8 program[] =
    {
        static_cast<u8>(EInstruction::LoadImmediate), 0, valueAPtrBytes[0], valueAPtrBytes[1], valueAPtrBytes[2], valueAPtrBytes[3],
        static_cast<u8>(EInstruction::LoadImmediate), 1, valueAPtrBytes[4], valueAPtrBytes[5], valueAPtrBytes[6], valueAPtrBytes[7],
        static_cast<u8>(EInstruction::LoadImmediate), 2, storageLocationPtrBytes[0], storageLocationPtrBytes[1], storageLocationPtrBytes[2], storageLocationPtrBytes[3],
        static_cast<u8>(EInstruction::LoadImmediate), 3, storageLocationPtrBytes[4], storageLocationPtrBytes[5], storageLocationPtrBytes[6], storageLocationPtrBytes[7],
        static_cast<u8>(EInstruction::LoadImmediate), 4, valueBBytes[0], valueBBytes[1], valueBBytes[2], valueBBytes[3],
        static_cast<u8>(EInstruction::LoadStore), 0b00111000, 0, 5, 0, 0,
        static_cast<u8>(EInstruction::AddF), 5, 4, 6,
        static_cast<u8>(EInstruction::LoadStore), 0b01111000, 2, 6, 0, 0,
        static_cast<u8>(EInstruction::FlushCache),
        static_cast<u8>(EInstruction::Hlt)
    };

    ConPrinter::Print("{} + {} = {}\n", valueA, valueB, storageValue);

    processor.TestLoadProgram(0, 0, 0x1, program);

    for(int i = 0; i < 20; ++i)
    {
        processor.Clock();
    }

    ConPrinter::Print("{} + {} = {}\n", valueA, valueB, storageValue);
}

static void TestAdd2F() noexcept
{
    f32 valueA[2] = { 1.5f, 1.0f };
    const f32 valueB[2] = { 2.5f, -2.5f };
    f32 storageValue[2] = { ::std::numeric_limits<f32>::quiet_NaN(), ::std::numeric_limits<f32>::quiet_NaN() };

    const uintptr_t valueAPtr = reinterpret_cast<uintptr_t>(&valueA) >> 2;
    u8 valueAPtrBytes[8];
    (void) ::std::memcpy(valueAPtrBytes, &valueAPtr, sizeof(valueAPtr));

    const uintptr_t valueBPtr = reinterpret_cast<uintptr_t>(&valueB) >> 2;
    u8 valueBPtrBytes[8];
    (void) ::std::memcpy(valueBPtrBytes, &valueBPtr, sizeof(valueBPtr));
    
    const uintptr_t storageLocationPtr = reinterpret_cast<uintptr_t>(&storageValue) >> 2;
    u8 storageLocationPtrBytes[8];
    (void) ::std::memcpy(storageLocationPtrBytes, &storageLocationPtr, sizeof(storageLocationPtr));

    u8 program[] =
    {
        static_cast<u8>(EInstruction::LoadImmediate), 0, valueAPtrBytes[0], valueAPtrBytes[1], valueAPtrBytes[2], valueAPtrBytes[3],
        static_cast<u8>(EInstruction::LoadImmediate), 1, valueAPtrBytes[4], valueAPtrBytes[5], valueAPtrBytes[6], valueAPtrBytes[7],
        static_cast<u8>(EInstruction::LoadImmediate), 2, valueBPtrBytes[0], valueBPtrBytes[1], valueBPtrBytes[2], valueBPtrBytes[3],
        static_cast<u8>(EInstruction::LoadImmediate), 3, valueBPtrBytes[4], valueBPtrBytes[5], valueBPtrBytes[6], valueBPtrBytes[7],
        static_cast<u8>(EInstruction::LoadImmediate), 4, storageLocationPtrBytes[0], storageLocationPtrBytes[1], storageLocationPtrBytes[2], storageLocationPtrBytes[3],
        static_cast<u8>(EInstruction::LoadImmediate), 5, storageLocationPtrBytes[4], storageLocationPtrBytes[5], storageLocationPtrBytes[6], storageLocationPtrBytes[7],
        static_cast<u8>(EInstruction::LoadStore), 0b00111001, 0, 6, 0, 0,
        static_cast<u8>(EInstruction::LoadStore), 0b00111001, 2, 8, 0, 0,
        static_cast<u8>(EInstruction::AddVec2F), 6, 8, 10,
        static_cast<u8>(EInstruction::LoadStore), 0b01111001, 4, 10, 0, 0,
        static_cast<u8>(EInstruction::FlushCache),
        static_cast<u8>(EInstruction::Hlt)
    };

    ConPrinter::Print("[{}, {}] + [{}, {}] = [{}, {}]\n", valueA[0], valueA[1], valueB[0], valueB[1], storageValue[0], storageValue[1]);

    processor.TestLoadProgram(0, 0, 0x1, program);

    for(int i = 0; i < 20; ++i)
    {
        processor.Clock();
    }

    ConPrinter::Print("[{}, {}] + [{}, {}] = [{}, {}]\n", valueA[0], valueA[1], valueB[0], valueB[1], storageValue[0], storageValue[1]);
}

static void TestAdd2FReplicated() noexcept
{
    struct InputData
    {
        f32 ValueA[2];
        f32 ValueB[2];
    };

    struct OutputData
    {
        f32 ValueResult[2];
    };

    InputData inData[2];
    OutputData outData[2];

    inData[0].ValueA[0] = 1.5f;
    inData[0].ValueA[1] = 1.0f;
    inData[0].ValueB[0] = 2.5f;
    inData[0].ValueB[1] = -2.5f;

    inData[1].ValueA[0] = 3.0f;
    inData[1].ValueA[1] = -1.0f;
    inData[1].ValueB[0] = 2.5f;
    inData[1].ValueB[1] = -2.5f;

    outData[0].ValueResult[0] = ::std::numeric_limits<f32>::quiet_NaN();
    outData[0].ValueResult[1] = ::std::numeric_limits<f32>::quiet_NaN();
    outData[1].ValueResult[0] = ::std::numeric_limits<f32>::quiet_NaN();
    outData[1].ValueResult[1] = ::std::numeric_limits<f32>::quiet_NaN();
    
    const uintptr_t inData0Ptr = reinterpret_cast<uintptr_t>(&inData[0]) >> 2;
    u32 inData0Words[2];
    (void) ::std::memcpy(inData0Words, &inData0Ptr, sizeof(inData0Ptr));

    const uintptr_t inData1Ptr = reinterpret_cast<uintptr_t>(&inData[1]) >> 2;
    u32 inData1Words[2];
    (void) ::std::memcpy(inData1Words, &inData1Ptr, sizeof(inData1Ptr));
    
    const uintptr_t outData0Ptr = reinterpret_cast<uintptr_t>(&outData[0]) >> 2;
    u32 outData0Words[2];
    (void) ::std::memcpy(outData0Words, &outData0Ptr, sizeof(outData0Ptr));

    const uintptr_t outData1Ptr = reinterpret_cast<uintptr_t>(&outData[1]) >> 2;
    u32 outData1Words[2];
    (void) ::std::memcpy(outData1Words, &outData1Ptr, sizeof(outData1Ptr));

    u8 program[] =
    {
        static_cast<u8>(EInstruction::LoadStore), 0b00111011, 0, 6, 0, 0, // Load inData.ValueA and inData.ValueB
        static_cast<u8>(EInstruction::AddVec2F), 6, 8, 10,
        static_cast<u8>(EInstruction::LoadStore), 0b01111001, 2, 10, 0, 0, // Store inData.ValueA + inData.ValueB in outData.ValueResult
        static_cast<u8>(EInstruction::FlushCache),
        static_cast<u8>(EInstruction::Hlt)
    };

    ConPrinter::Print("[{}, {}] + [{}, {}] = [{}, {}]\n", inData[0].ValueA[0], inData[0].ValueA[1], inData[0].ValueB[0], inData[0].ValueB[1], outData[0].ValueResult[0], outData[0].ValueResult[1]);
    ConPrinter::Print("[{}, {}] + [{}, {}] = [{}, {}]\n", inData[1].ValueA[0], inData[1].ValueA[1], inData[1].ValueB[0], inData[1].ValueB[1], outData[1].ValueResult[0], outData[1].ValueResult[1]);

    processor.TestLoadProgram(0, 0, 0x3, program);

    processor.TestLoadRegister(0, 0, 0, 0, inData0Words[0]);
    processor.TestLoadRegister(0, 0, 0, 1, inData0Words[1]);
    processor.TestLoadRegister(0, 0, 1, 0, inData1Words[0]);
    processor.TestLoadRegister(0, 0, 1, 1, inData1Words[1]);
    processor.TestLoadRegister(0, 0, 0, 2, outData0Words[0]);
    processor.TestLoadRegister(0, 0, 0, 3, outData0Words[1]);
    processor.TestLoadRegister(0, 0, 1, 2, outData1Words[0]);
    processor.TestLoadRegister(0, 0, 1, 3, outData1Words[1]);

    for(int i = 0; i < 20; ++i)
    {
        processor.Clock();
    }

    ConPrinter::Print("[{}, {}] + [{}, {}] = [{}, {}]\n", inData[0].ValueA[0], inData[0].ValueA[1], inData[0].ValueB[0], inData[0].ValueB[1], outData[0].ValueResult[0], outData[0].ValueResult[1]);
    ConPrinter::Print("[{}, {}] + [{}, {}] = [{}, {}]\n", inData[1].ValueA[0], inData[1].ValueA[1], inData[1].ValueB[0], inData[1].ValueB[1], outData[1].ValueResult[0], outData[1].ValueResult[1]);
}

static void TestMul2FReplicated() noexcept
{
    struct InputData
    {
        f32 ValueA[2];
        f32 ValueB[2];
    };

    struct OutputData
    {
        f32 ValueResult[2];
        u64 FpSaturation;
        u64 IntFpSaturation;
        u64 LdStSaturation;
        u64 FpTotals;
        u64 IntFpTotals;
        u64 LdStTotals;
    };

    InputData inData[4];
    OutputData outData[4];

    inData[0].ValueA[0] = 1.5f;
    inData[0].ValueA[1] = 1.0f;
    inData[0].ValueB[0] = 2.5f;
    inData[0].ValueB[1] = -2.5f;

    inData[1].ValueA[0] = 3.0f;
    inData[1].ValueA[1] = -1.0f;
    inData[1].ValueB[0] = 2.5f;
    inData[1].ValueB[1] = -2.5f;

    inData[2].ValueA[0] = 0.5f;
    inData[2].ValueA[1] = 2.0f;
    inData[2].ValueB[0] = 2.5f;
    inData[2].ValueB[1] = -2.5f;

    inData[3].ValueA[0] = 5.3f;
    inData[3].ValueA[1] = 0.0f;
    inData[3].ValueB[0] = 0.1f;
    inData[3].ValueB[1] = -2.5f;

    outData[0].ValueResult[0] = ::std::numeric_limits<f32>::quiet_NaN();
    outData[0].ValueResult[1] = ::std::numeric_limits<f32>::quiet_NaN();
    outData[1].ValueResult[0] = ::std::numeric_limits<f32>::quiet_NaN();
    outData[1].ValueResult[1] = ::std::numeric_limits<f32>::quiet_NaN();
    outData[2].ValueResult[0] = ::std::numeric_limits<f32>::quiet_NaN();
    outData[2].ValueResult[1] = ::std::numeric_limits<f32>::quiet_NaN();
    outData[3].ValueResult[0] = ::std::numeric_limits<f32>::quiet_NaN();
    outData[3].ValueResult[1] = ::std::numeric_limits<f32>::quiet_NaN();

    const uintptr_t inData0Ptr = reinterpret_cast<uintptr_t>(&inData[0]) >> 2;
    u32 inData0Words[2];
    (void) ::std::memcpy(inData0Words, &inData0Ptr, sizeof(inData0Ptr));

    const uintptr_t inData1Ptr = reinterpret_cast<uintptr_t>(&inData[1]) >> 2;
    u32 inData1Words[2];
    (void) ::std::memcpy(inData1Words, &inData1Ptr, sizeof(inData1Ptr));

    const uintptr_t inData2Ptr = reinterpret_cast<uintptr_t>(&inData[2]) >> 2;
    u32 inData2Words[2];
    (void) ::std::memcpy(inData2Words, &inData2Ptr, sizeof(inData2Ptr));

    const uintptr_t inData3Ptr = reinterpret_cast<uintptr_t>(&inData[3]) >> 2;
    u32 inData3Words[2];
    (void) ::std::memcpy(inData3Words, &inData3Ptr, sizeof(inData3Ptr));

    const uintptr_t outData0Ptr = reinterpret_cast<uintptr_t>(&outData[0]) >> 2;
    u32 outData0Words[2];
    (void) ::std::memcpy(outData0Words, &outData0Ptr, sizeof(outData0Ptr));

    const uintptr_t outData1Ptr = reinterpret_cast<uintptr_t>(&outData[1]) >> 2;
    u32 outData1Words[2];
    (void) ::std::memcpy(outData1Words, &outData1Ptr, sizeof(outData1Ptr));

    const uintptr_t outData2Ptr = reinterpret_cast<uintptr_t>(&outData[2]) >> 2;
    u32 outData2Words[2];
    (void) ::std::memcpy(outData2Words, &outData2Ptr, sizeof(outData2Ptr));

    const uintptr_t outData3Ptr = reinterpret_cast<uintptr_t>(&outData[3]) >> 2;
    u32 outData3Words[2];
    (void) ::std::memcpy(outData3Words, &outData3Ptr, sizeof(outData3Ptr));

    u8 program[] =
    {
        static_cast<u8>(EInstruction::LoadStore), 0b00111011, 0, 6, 0, 0, // Load inData.ValueA and inData.ValueB
        static_cast<u8>(EInstruction::MulVec2F), 6, 8, 10,
        static_cast<u8>(EInstruction::LoadStore), 0b01111001, 2, 10, 0, 0, // Store inData.ValueA + inData.ValueB in outData.ValueResult
        static_cast<u8>(EInstruction::WriteStatistics), 0, 12, 14,
        static_cast<u8>(EInstruction::WriteStatistics), 1, 16, 18,
        static_cast<u8>(EInstruction::WriteStatistics), 2, 20, 22,
        static_cast<u8>(EInstruction::LoadStore), 0b01111001, 2, 12, 2, 0,
        static_cast<u8>(EInstruction::LoadStore), 0b01111001, 2, 16, 4, 0,
        static_cast<u8>(EInstruction::LoadStore), 0b01111001, 2, 20, 6, 0,
        static_cast<u8>(EInstruction::LoadStore), 0b01111001, 2, 14, 8, 0,
        static_cast<u8>(EInstruction::LoadStore), 0b01111001, 2, 18, 10, 0,
        static_cast<u8>(EInstruction::LoadStore), 0b01111001, 2, 22, 12, 0,
        static_cast<u8>(EInstruction::FlushCache),
        static_cast<u8>(EInstruction::Hlt)
    };

    for(u32 i = 0; i < 4; ++i)
    {
        ConPrinter::Print("[{}, {}] + [{}, {}] = [{}, {}]\n", inData[i].ValueA[0], inData[i].ValueA[1], inData[i].ValueB[0], inData[i].ValueB[1], outData[i].ValueResult[0], outData[i].ValueResult[1]);
    }

    processor.TestLoadProgram(0, 0, 0xF, program);

    processor.TestLoadRegister(0, 0, 0, 0, inData0Words[0]);
    processor.TestLoadRegister(0, 0, 0, 1, inData0Words[1]);
    processor.TestLoadRegister(0, 0, 1, 0, inData1Words[0]);
    processor.TestLoadRegister(0, 0, 1, 1, inData1Words[1]);
    processor.TestLoadRegister(0, 0, 2, 0, inData2Words[0]);
    processor.TestLoadRegister(0, 0, 2, 1, inData2Words[1]);
    processor.TestLoadRegister(0, 0, 3, 0, inData3Words[0]);
    processor.TestLoadRegister(0, 0, 3, 1, inData3Words[1]);
    processor.TestLoadRegister(0, 0, 0, 2, outData0Words[0]);
    processor.TestLoadRegister(0, 0, 0, 3, outData0Words[1]);
    processor.TestLoadRegister(0, 0, 1, 2, outData1Words[0]);
    processor.TestLoadRegister(0, 0, 1, 3, outData1Words[1]);
    processor.TestLoadRegister(0, 0, 2, 2, outData2Words[0]);
    processor.TestLoadRegister(0, 0, 2, 3, outData2Words[1]);
    processor.TestLoadRegister(0, 0, 3, 2, outData3Words[0]);
    processor.TestLoadRegister(0, 0, 3, 3, outData3Words[1]);

    for(int i = 0; i < 80; ++i)
    {
        processor.Clock();
    }

    for(u32 i = 0; i < 4; ++i)
    {
        ConPrinter::Print("[{}, {}] + [{}, {}] = [{}, {}]\n", inData[i].ValueA[0], inData[i].ValueA[1], inData[i].ValueB[0], inData[i].ValueB[1], outData[i].ValueResult[0], outData[i].ValueResult[1]);
    }

    const f64 fpSaturation = static_cast<f64>(outData[0].FpSaturation) / static_cast<f64>(outData[0].FpTotals);
    const f64 intFpSaturation = static_cast<f64>(outData[0].IntFpSaturation) / static_cast<f64>(outData[0].IntFpTotals);
    const f64 ldStSaturation = static_cast<f64>(outData[0].LdStSaturation) / static_cast<f64>(outData[0].LdStTotals);

    ConPrinter::Print("FP Saturation: {}:{} ({})\n", outData[0].FpSaturation, outData[0].FpTotals, fpSaturation);
    ConPrinter::Print("Int/FP Saturation: {}:{} ({})\n", outData[0].IntFpSaturation, outData[0].IntFpTotals, intFpSaturation);
    ConPrinter::Print("LD/ST Saturation: {}:{} ({})\n", outData[0].LdStSaturation, outData[0].LdStTotals, ldStSaturation);
}

static void TestMul2FReplicatedDualDispatch() noexcept
{
    struct InputData
    {
        f32 ValueA[2];
        f32 ValueB[2];
    };

    struct OutputData
    {
        f32 ValueResult[2];
        u64 FpSaturation;
        u64 IntFpSaturation;
        u64 LdStSaturation;
        u64 FpTotals;
        u64 IntFpTotals;
        u64 LdStTotals;
    };

    InputData inData[4];
    OutputData outData[4];

    inData[0].ValueA[0] = 1.5f;
    inData[0].ValueA[1] = 1.0f;
    inData[0].ValueB[0] = 2.5f;
    inData[0].ValueB[1] = -2.5f;

    inData[1].ValueA[0] = 3.0f;
    inData[1].ValueA[1] = -1.0f;
    inData[1].ValueB[0] = 2.5f;
    inData[1].ValueB[1] = -2.5f;

    inData[2].ValueA[0] = 0.5f;
    inData[2].ValueA[1] = 2.0f;
    inData[2].ValueB[0] = 2.5f;
    inData[2].ValueB[1] = -2.5f;

    inData[3].ValueA[0] = 5.3f;
    inData[3].ValueA[1] = 0.0f;
    inData[3].ValueB[0] = 0.1f;
    inData[3].ValueB[1] = -2.5f;

    for(u32 i = 0; i < 4; ++i)
    {
        outData[i].ValueResult[0] = ::std::numeric_limits<f32>::quiet_NaN();
        outData[i].ValueResult[1] = ::std::numeric_limits<f32>::quiet_NaN();
        outData[i].FpSaturation = 0;
        outData[i].IntFpSaturation = 0;
        outData[i].LdStSaturation = 0;
        outData[i].FpTotals = 0;
        outData[i].IntFpTotals = 0;
        outData[i].LdStTotals = 0;
    }

    const uintptr_t inData0Ptr = reinterpret_cast<uintptr_t>(&inData[0]) >> 2;
    u32 inData0Words[2];
    (void) ::std::memcpy(inData0Words, &inData0Ptr, sizeof(inData0Ptr));

    const uintptr_t inData1Ptr = reinterpret_cast<uintptr_t>(&inData[1]) >> 2;
    u32 inData1Words[2];
    (void) ::std::memcpy(inData1Words, &inData1Ptr, sizeof(inData1Ptr));

    const uintptr_t inData2Ptr = reinterpret_cast<uintptr_t>(&inData[2]) >> 2;
    u32 inData2Words[2];
    (void) ::std::memcpy(inData2Words, &inData2Ptr, sizeof(inData2Ptr));

    const uintptr_t inData3Ptr = reinterpret_cast<uintptr_t>(&inData[3]) >> 2;
    u32 inData3Words[2];
    (void) ::std::memcpy(inData3Words, &inData3Ptr, sizeof(inData3Ptr));

    const uintptr_t outData0Ptr = reinterpret_cast<uintptr_t>(&outData[0]) >> 2;
    u32 outData0Words[2];
    (void) ::std::memcpy(outData0Words, &outData0Ptr, sizeof(outData0Ptr));

    const uintptr_t outData1Ptr = reinterpret_cast<uintptr_t>(&outData[1]) >> 2;
    u32 outData1Words[2];
    (void) ::std::memcpy(outData1Words, &outData1Ptr, sizeof(outData1Ptr));

    const uintptr_t outData2Ptr = reinterpret_cast<uintptr_t>(&outData[2]) >> 2;
    u32 outData2Words[2];
    (void) ::std::memcpy(outData2Words, &outData2Ptr, sizeof(outData2Ptr));

    const uintptr_t outData3Ptr = reinterpret_cast<uintptr_t>(&outData[3]) >> 2;
    u32 outData3Words[2];
    (void) ::std::memcpy(outData3Words, &outData3Ptr, sizeof(outData3Ptr));

    u8 program[] =
    {
        static_cast<u8>(EInstruction::LoadStore), 0b00111011, 0, 6, 0, 0, // Load inData.ValueA and inData.ValueB
        static_cast<u8>(EInstruction::MulVec2F), 6, 8, 10,
        static_cast<u8>(EInstruction::WriteStatistics), 0, 12, 14,
        static_cast<u8>(EInstruction::WriteStatistics), 1, 16, 18,
        static_cast<u8>(EInstruction::LoadStore), 0b01111001, 2, 10, 0, 0, // Store inData.ValueA + inData.ValueB in outData.ValueResult
        static_cast<u8>(EInstruction::WriteStatistics), 2, 20, 22,
        static_cast<u8>(EInstruction::LoadStore), 0b01111001, 2, 12, 2, 0,
        static_cast<u8>(EInstruction::LoadStore), 0b01111001, 2, 16, 4, 0,
        static_cast<u8>(EInstruction::LoadStore), 0b01111001, 2, 20, 6, 0,
        static_cast<u8>(EInstruction::LoadStore), 0b01111001, 2, 14, 8, 0,
        static_cast<u8>(EInstruction::LoadStore), 0b01111001, 2, 18, 10, 0,
        static_cast<u8>(EInstruction::LoadStore), 0b01111001, 2, 22, 12, 0,
        static_cast<u8>(EInstruction::FlushCache),
        static_cast<u8>(EInstruction::Hlt)
    };

    for(u32 i = 0; i < 4; ++i)
    {
        ConPrinter::Print("[{}, {}] + [{}, {}] = [{}, {}]\n", inData[i].ValueA[0], inData[i].ValueA[1], inData[i].ValueB[0], inData[i].ValueB[1], outData[i].ValueResult[0], outData[i].ValueResult[1]);
    }

    for(u32 i = 0; i < 2; ++i)
    {
        processor.TestLoadProgram(0, i, 0xF, program);

        processor.TestLoadRegister(0, i, 0, 0, inData0Words[0]);
        processor.TestLoadRegister(0, i, 0, 1, inData0Words[1]);
        processor.TestLoadRegister(0, i, 1, 0, inData1Words[0]);
        processor.TestLoadRegister(0, i, 1, 1, inData1Words[1]);
        processor.TestLoadRegister(0, i, 2, 0, inData2Words[0]);
        processor.TestLoadRegister(0, i, 2, 1, inData2Words[1]);
        processor.TestLoadRegister(0, i, 3, 0, inData3Words[0]);
        processor.TestLoadRegister(0, i, 3, 1, inData3Words[1]);
        processor.TestLoadRegister(0, i, 0, 2, outData0Words[0]);
        processor.TestLoadRegister(0, i, 0, 3, outData0Words[1]);
        processor.TestLoadRegister(0, i, 1, 2, outData1Words[0]);
        processor.TestLoadRegister(0, i, 1, 3, outData1Words[1]);
        processor.TestLoadRegister(0, i, 2, 2, outData2Words[0]);
        processor.TestLoadRegister(0, i, 2, 3, outData2Words[1]);
        processor.TestLoadRegister(0, i, 3, 2, outData3Words[0]);
        processor.TestLoadRegister(0, i, 3, 3, outData3Words[1]);
    }

    for(int i = 0; i < 500; ++i)
    {
        processor.Clock();
    }

    for(u32 i = 0; i < 4; ++i)
    {
        ConPrinter::Print("[{}, {}] + [{}, {}] = [{}, {}]\n", inData[i].ValueA[0], inData[i].ValueA[1], inData[i].ValueB[0], inData[i].ValueB[1], outData[i].ValueResult[0], outData[i].ValueResult[1]);
    }

    const u64 totalCoreSaturation = outData[0].FpSaturation + outData[0].IntFpSaturation;
    const f64 totalCoreTotals = static_cast<f64>(outData[0].FpTotals + outData[0].IntFpTotals) / 2.0;

    const f64 fpPercent = static_cast<f64>(outData[0].FpSaturation) / static_cast<f64>(outData[0].FpTotals);
    const f64 intFpPercent = static_cast<f64>(outData[0].IntFpSaturation) / static_cast<f64>(outData[0].IntFpTotals);
    const f64 totalCorePercent = static_cast<f64>(totalCoreSaturation) / totalCoreTotals;
    const f64 ldStPercent = static_cast<f64>(outData[0].LdStSaturation) / static_cast<f64>(outData[0].LdStTotals);

    ConPrinter::Print("FP Saturation: {}:{} ({})\n", outData[0].FpSaturation, outData[0].FpTotals, fpPercent);
    ConPrinter::Print("Int/FP Saturation: {}:{} ({})\n", outData[0].IntFpSaturation, outData[0].IntFpTotals, intFpPercent);
    ConPrinter::Print("Total Core Saturation: {}:{} ({})\n", totalCoreSaturation, static_cast<u64>(totalCoreTotals), totalCorePercent);
    ConPrinter::Print("LD/ST Saturation: {}:{} ({})\n", outData[0].LdStSaturation, outData[0].LdStTotals, ldStPercent);
}
