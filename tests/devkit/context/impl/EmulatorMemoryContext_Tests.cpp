#include "context/impl/EmulatorMemoryContext.hh"

#include "tests/devkit/services/mocks/MockClock.hh"
#include "tests/devkit/services/mocks/MockDebuggerDetector.hh"

#include "testutil\CppUnitTest.hh"

namespace ra {
namespace context {
namespace impl {
namespace tests {

TEST_CLASS(EmulatorMemoryContext_Tests)
{
private:
    class EmulatorMemoryContextHarness : public EmulatorMemoryContext
    {
    public:
        ra::services::mocks::MockClock mockClock;
        ra::services::mocks::MockDebuggerDetector mockDebuggerDetector;

        GSL_SUPPRESS_F6 EmulatorMemoryContextHarness() noexcept
            : m_Override(this)
        {
        }

    private:
        ra::services::ServiceLocator::ServiceOverride<EmulatorMemoryContext> m_Override;
    };

    class EmulatorMemoryContextNotifyHarness : public EmulatorMemoryContext::NotifyTarget
    {
    public:
        void OnByteWritten(ra::data::ByteAddress nAddress, uint8_t nValue) noexcept override
        {
            nLastByteWrittenAddress = nAddress;
            nLastByteWritten = nValue;
        }

        ra::data::ByteAddress nLastByteWrittenAddress = 0xFFFFFFFFU;
        uint8_t nLastByteWritten = 0xFF;
    };

    static std::array<uint8_t, 64> memory;

    static uint8_t ReadMemory0(uint32_t nAddress) noexcept { return memory.at(nAddress); }
    static uint8_t ReadMemory1(uint32_t nAddress) noexcept { return memory.at(gsl::narrow_cast<size_t>(nAddress) + 10); }
    static uint8_t ReadMemory2(uint32_t nAddress) noexcept { return memory.at(gsl::narrow_cast<size_t>(nAddress) + 20); }
    static uint8_t ReadMemory3(uint32_t nAddress) noexcept { return memory.at(gsl::narrow_cast<size_t>(nAddress) + 30); }

    static uint32_t ReadMemoryBlock0(uint32_t nAddress, uint8_t* pBuffer, uint32_t nBytes) noexcept
    {
        memcpy(pBuffer, &memory.at(nAddress), nBytes);
        return nBytes;
    }

    static uint32_t ReadMemoryBlock1(uint32_t nAddress, uint8_t* pBuffer, uint32_t nBytes) noexcept
    {
        memcpy(pBuffer, &memory.at(gsl::narrow_cast<size_t>(nAddress)) + 10, nBytes);
        return nBytes;
    }

    static void WriteMemory0(uint32_t nAddress, uint8_t nValue) noexcept { memory.at(nAddress) = nValue; }
    static void WriteMemory1(uint32_t nAddress, uint8_t nValue) noexcept { memory.at(gsl::narrow_cast<size_t>(nAddress) + 10) = nValue; }
    static void WriteMemory2(uint32_t nAddress, uint8_t nValue) noexcept { memory.at(gsl::narrow_cast<size_t>(nAddress) + 20) = nValue; }
    static void WriteMemory3(uint32_t nAddress, uint8_t nValue) noexcept { memory.at(gsl::narrow_cast<size_t>(nAddress) + 30) = nValue; }

    static void InitializeMemory()
    {
        for (size_t i = 0; i < memory.size(); ++i)
            memory.at(i) = gsl::narrow_cast<uint8_t>(i);
    }

public:
    TEST_METHOD(TestIsValidAddress)
    {
        EmulatorMemoryContextHarness emulator;
        emulator.AddMemoryBlock(0, 20, &ReadMemory0, &WriteMemory0);
        emulator.AddMemoryBlock(1, 10, &ReadMemory2, &WriteMemory2);
        Assert::AreEqual({ 30U }, emulator.TotalMemorySize());

        Assert::IsTrue(emulator.IsValidAddress(12U));
        Assert::IsTrue(emulator.IsValidAddress(25U));
        Assert::IsTrue(emulator.IsValidAddress(29U));
        Assert::IsFalse(emulator.IsValidAddress(30U));
    }

    TEST_METHOD(TestReadMemoryByte)
    {
        InitializeMemory();

        EmulatorMemoryContextHarness emulator;
        emulator.AddMemoryBlock(0, 20, &ReadMemory0, &WriteMemory0);
        emulator.AddMemoryBlock(1, 10, &ReadMemory2, &WriteMemory2);
        Assert::AreEqual({ 30U }, emulator.TotalMemorySize());

        Assert::AreEqual(12, static_cast<int>(emulator.ReadMemoryByte(12U)));
        Assert::AreEqual(25, static_cast<int>(emulator.ReadMemoryByte(25U)));
        Assert::AreEqual(4, static_cast<int>(emulator.ReadMemoryByte(4U)));
        Assert::AreEqual(29, static_cast<int>(emulator.ReadMemoryByte(29U)));
        Assert::AreEqual(0, static_cast<int>(emulator.ReadMemoryByte(30U)));
    }

    TEST_METHOD(TestAddMemoryBlocksOutOfOrder)
    {
        InitializeMemory();

        EmulatorMemoryContextHarness emulator;
        emulator.AddMemoryBlock(1, 10, &ReadMemory2, &WriteMemory2);
        emulator.AddMemoryBlock(0, 20, &ReadMemory0, &WriteMemory0);
        Assert::AreEqual({ 30U }, emulator.TotalMemorySize());

        Assert::AreEqual(12, static_cast<int>(emulator.ReadMemoryByte(12U)));
        Assert::AreEqual(25, static_cast<int>(emulator.ReadMemoryByte(25U)));
        Assert::AreEqual(4, static_cast<int>(emulator.ReadMemoryByte(4U)));
        Assert::AreEqual(29, static_cast<int>(emulator.ReadMemoryByte(29U)));
        Assert::AreEqual(0, static_cast<int>(emulator.ReadMemoryByte(30U)));
    }

    TEST_METHOD(TestAddMemoryBlockDoesNotOverwrite)
    {
        InitializeMemory();

        EmulatorMemoryContextHarness emulator;
        emulator.AddMemoryBlock(0, 20, &ReadMemory0, &WriteMemory0);
        emulator.AddMemoryBlock(1, 10, &ReadMemory2, &WriteMemory2);
        emulator.AddMemoryBlock(0, 20, &ReadMemory3, &WriteMemory3);
        Assert::AreEqual({ 30U }, emulator.TotalMemorySize());

        Assert::AreEqual(12, static_cast<int>(emulator.ReadMemoryByte(12U)));
        Assert::AreEqual(25, static_cast<int>(emulator.ReadMemoryByte(25U)));
        Assert::AreEqual(4, static_cast<int>(emulator.ReadMemoryByte(4U)));
        Assert::AreEqual(29, static_cast<int>(emulator.ReadMemoryByte(29U)));
        Assert::AreEqual(0, static_cast<int>(emulator.ReadMemoryByte(30U)));
    }

    TEST_METHOD(TestClearMemoryBlocks)
    {
        InitializeMemory();

        EmulatorMemoryContextHarness emulator;
        emulator.AddMemoryBlock(0, 20, &ReadMemory0, &WriteMemory0);
        emulator.AddMemoryBlock(1, 10, &ReadMemory2, &WriteMemory2);
        emulator.ClearMemoryBlocks();
        emulator.AddMemoryBlock(0, 20, &ReadMemory3, &WriteMemory3);
        Assert::AreEqual({ 20U }, emulator.TotalMemorySize());

        Assert::AreEqual(32, static_cast<int>(emulator.ReadMemoryByte(2U)));
        Assert::AreEqual(41, static_cast<int>(emulator.ReadMemoryByte(11U)));
        Assert::AreEqual(0, static_cast<int>(emulator.ReadMemoryByte(55U)));
    }

    TEST_METHOD(TestInvalidMemoryBlock)
    {
        InitializeMemory();

        EmulatorMemoryContextHarness emulator;
        emulator.AddMemoryBlock(0, 20, &ReadMemory0, &WriteMemory0);
        emulator.AddMemoryBlock(1, 10, nullptr, nullptr);
        emulator.AddMemoryBlock(2, 10, &ReadMemory3, &WriteMemory3);
        Assert::AreEqual({ 40U }, emulator.TotalMemorySize());

        Assert::IsTrue(emulator.IsValidAddress(12U));
        Assert::IsTrue(emulator.IsValidAddress(19U));
        Assert::IsFalse(emulator.IsValidAddress(20U));
        Assert::IsFalse(emulator.IsValidAddress(29U));
        Assert::IsTrue(emulator.IsValidAddress(30U));
        Assert::IsTrue(emulator.IsValidAddress(39U));
        Assert::IsFalse(emulator.IsValidAddress(40U));

        Assert::AreEqual(12, static_cast<int>(emulator.ReadMemoryByte(12U)));
        Assert::AreEqual(19, static_cast<int>(emulator.ReadMemoryByte(19U)));
        Assert::AreEqual(0, static_cast<int>(emulator.ReadMemoryByte(20U)));
        Assert::AreEqual(0, static_cast<int>(emulator.ReadMemoryByte(29U)));
        Assert::AreEqual(30, static_cast<int>(emulator.ReadMemoryByte(30U)));
        Assert::AreEqual(39, static_cast<int>(emulator.ReadMemoryByte(39U)));
        Assert::AreEqual(0, static_cast<int>(emulator.ReadMemoryByte(40U)));
    }

    TEST_METHOD(TestReadMemory)
    {
        memory.at(4) = 0xA8;
        memory.at(5) = 0x00;
        memory.at(6) = 0x37;
        memory.at(7) = 0x2E;

        EmulatorMemoryContextHarness emulator;
        emulator.AddMemoryBlock(0, 20, &ReadMemory0, &WriteMemory0);

        // ReadMemory calls ReadMemoryByte for small sizes
        Assert::AreEqual(0, static_cast<int>(emulator.ReadMemory(4U, ra::data::Memory::Size::Bit0)));
        Assert::AreEqual(0, static_cast<int>(emulator.ReadMemory(4U, ra::data::Memory::Size::Bit1)));
        Assert::AreEqual(0, static_cast<int>(emulator.ReadMemory(4U, ra::data::Memory::Size::Bit2)));
        Assert::AreEqual(1, static_cast<int>(emulator.ReadMemory(4U, ra::data::Memory::Size::Bit3)));
        Assert::AreEqual(0, static_cast<int>(emulator.ReadMemory(4U, ra::data::Memory::Size::Bit4)));
        Assert::AreEqual(1, static_cast<int>(emulator.ReadMemory(4U, ra::data::Memory::Size::Bit5)));
        Assert::AreEqual(0, static_cast<int>(emulator.ReadMemory(4U, ra::data::Memory::Size::Bit6)));
        Assert::AreEqual(1, static_cast<int>(emulator.ReadMemory(4U, ra::data::Memory::Size::Bit7)));
        Assert::AreEqual(3, static_cast<int>(emulator.ReadMemory(4U, ra::data::Memory::Size::BitCount)));
        Assert::AreEqual(8, static_cast<int>(emulator.ReadMemory(4U, ra::data::Memory::Size::NibbleLower)));
        Assert::AreEqual(10, static_cast<int>(emulator.ReadMemory(4U, ra::data::Memory::Size::NibbleUpper)));
        Assert::AreEqual(0xA8, static_cast<int>(emulator.ReadMemory(4U, ra::data::Memory::Size::EightBit)));
        Assert::AreEqual(0xA8, static_cast<int>(emulator.ReadMemory(4U, ra::data::Memory::Size::SixteenBit)));
        Assert::AreEqual(0x2E37, static_cast<int>(emulator.ReadMemory(6U, ra::data::Memory::Size::SixteenBit)));
        Assert::AreEqual(0x2E3700, static_cast<int>(emulator.ReadMemory(5U, ra::data::Memory::Size::TwentyFourBit)));
        Assert::AreEqual(0x2E3700A8, static_cast<int>(emulator.ReadMemory(4U, ra::data::Memory::Size::ThirtyTwoBit)));
        Assert::AreEqual(0x372E, static_cast<int>(emulator.ReadMemory(6U, ra::data::Memory::Size::SixteenBitBigEndian)));
        Assert::AreEqual(0x00372E, static_cast<int>(emulator.ReadMemory(5U, ra::data::Memory::Size::TwentyFourBitBigEndian)));
        Assert::AreEqual(0xA800372EU, emulator.ReadMemory(4U, ra::data::Memory::Size::ThirtyTwoBitBigEndian));

        memory.at(4) ^= 0xFF; // toggle all bits and verify again
        Assert::AreEqual(1, static_cast<int>(emulator.ReadMemory(4U, ra::data::Memory::Size::Bit0)));
        Assert::AreEqual(1, static_cast<int>(emulator.ReadMemory(4U, ra::data::Memory::Size::Bit1)));
        Assert::AreEqual(1, static_cast<int>(emulator.ReadMemory(4U, ra::data::Memory::Size::Bit2)));
        Assert::AreEqual(0, static_cast<int>(emulator.ReadMemory(4U, ra::data::Memory::Size::Bit3)));
        Assert::AreEqual(1, static_cast<int>(emulator.ReadMemory(4U, ra::data::Memory::Size::Bit4)));
        Assert::AreEqual(0, static_cast<int>(emulator.ReadMemory(4U, ra::data::Memory::Size::Bit5)));
        Assert::AreEqual(1, static_cast<int>(emulator.ReadMemory(4U, ra::data::Memory::Size::Bit6)));
        Assert::AreEqual(0, static_cast<int>(emulator.ReadMemory(4U, ra::data::Memory::Size::Bit7)));
        Assert::AreEqual(5, static_cast<int>(emulator.ReadMemory(4U, ra::data::Memory::Size::BitCount)));
        Assert::AreEqual(7, static_cast<int>(emulator.ReadMemory(4U, ra::data::Memory::Size::NibbleLower)));
        Assert::AreEqual(5, static_cast<int>(emulator.ReadMemory(4U, ra::data::Memory::Size::NibbleUpper)));
        Assert::AreEqual(0x57, static_cast<int>(emulator.ReadMemory(4U, ra::data::Memory::Size::EightBit)));
    }

    TEST_METHOD(TestReadMemoryBlock)
    {
        for (size_t i = 0; i < memory.size(); i++)
            memory.at(i) = gsl::narrow_cast<uint8_t>(i);

        memory.at(4) = 0xA8;
        memory.at(5) = 0x00;
        memory.at(6) = 0x37;
        memory.at(7) = 0x2E;

        memory.at(14) = 0x57;

        EmulatorMemoryContextHarness emulator;
        emulator.AddMemoryBlock(0, 20, &ReadMemory1, &WriteMemory0); // purposefully use ReadMemory1 to detect using byte reader for non-byte reads
        emulator.AddMemoryBlockReader(0, &ReadMemoryBlock0);

        // ReadMemory calls ReadMemoryByte for small sizes - should call ReadMemory1 ($4 => $14)
        Assert::AreEqual(1, static_cast<int>(emulator.ReadMemory(4U, ra::data::Memory::Size::Bit0)));
        Assert::AreEqual(1, static_cast<int>(emulator.ReadMemory(4U, ra::data::Memory::Size::Bit1)));
        Assert::AreEqual(1, static_cast<int>(emulator.ReadMemory(4U, ra::data::Memory::Size::Bit2)));
        Assert::AreEqual(0, static_cast<int>(emulator.ReadMemory(4U, ra::data::Memory::Size::Bit3)));
        Assert::AreEqual(1, static_cast<int>(emulator.ReadMemory(4U, ra::data::Memory::Size::Bit4)));
        Assert::AreEqual(0, static_cast<int>(emulator.ReadMemory(4U, ra::data::Memory::Size::Bit5)));
        Assert::AreEqual(1, static_cast<int>(emulator.ReadMemory(4U, ra::data::Memory::Size::Bit6)));
        Assert::AreEqual(0, static_cast<int>(emulator.ReadMemory(4U, ra::data::Memory::Size::Bit7)));
        Assert::AreEqual(5, static_cast<int>(emulator.ReadMemory(4U, ra::data::Memory::Size::BitCount)));
        Assert::AreEqual(7, static_cast<int>(emulator.ReadMemory(4U, ra::data::Memory::Size::NibbleLower)));
        Assert::AreEqual(5, static_cast<int>(emulator.ReadMemory(4U, ra::data::Memory::Size::NibbleUpper)));
        Assert::AreEqual(0x57, static_cast<int>(emulator.ReadMemory(4U, ra::data::Memory::Size::EightBit)));

        // sizes larger than 8 bits should use the block reader ($4 => $4)
        Assert::AreEqual(0xA8, static_cast<int>(emulator.ReadMemory(4U, ra::data::Memory::Size::SixteenBit)));
        Assert::AreEqual(0x2E37, static_cast<int>(emulator.ReadMemory(6U, ra::data::Memory::Size::SixteenBit)));
        Assert::AreEqual(0x2E3700, static_cast<int>(emulator.ReadMemory(5U, ra::data::Memory::Size::TwentyFourBit)));
        Assert::AreEqual(0x2E3700A8, static_cast<int>(emulator.ReadMemory(4U, ra::data::Memory::Size::ThirtyTwoBit)));
        Assert::AreEqual(0x372E, static_cast<int>(emulator.ReadMemory(6U, ra::data::Memory::Size::SixteenBitBigEndian)));
        Assert::AreEqual(0x00372E, static_cast<int>(emulator.ReadMemory(5U, ra::data::Memory::Size::TwentyFourBitBigEndian)));
        Assert::AreEqual(0xA800372EU, emulator.ReadMemory(4U, ra::data::Memory::Size::ThirtyTwoBitBigEndian));

        // test the block reader directly
        uint8_t buffer[16];
        emulator.ReadMemory(0U, buffer, sizeof(buffer));
        for (size_t i = 0; i < sizeof(buffer); i++)
            Assert::AreEqual(gsl::at(buffer, i), memory.at(i));

        emulator.ReadMemory(4U, buffer, 1);
        Assert::AreEqual(gsl::at(buffer, 0), memory.at(4));
        Assert::AreEqual(gsl::at(buffer, 1), memory.at(1));
    }

    TEST_METHOD(TestReadMemoryBuffer)
    {
        InitializeMemory();

        EmulatorMemoryContextHarness emulator;
        emulator.AddMemoryBlock(0, 20, &ReadMemory0, &WriteMemory0);
        emulator.AddMemoryBlock(1, 10, &ReadMemory2, &WriteMemory2);
        Assert::AreEqual({ 30U }, emulator.TotalMemorySize());

        uint8_t buffer[32];

        // simple read within a block
        emulator.ReadMemory(6U, buffer, 12);
        Assert::IsTrue(memcmp(buffer, &memory.at(6), 11) == 0);

        // read across block
        emulator.ReadMemory(19U, buffer, 4);
        Assert::IsTrue(memcmp(buffer, &memory.at(19), 4) == 0);

        // read end of block
        emulator.ReadMemory(13U, buffer, 7);
        Assert::IsTrue(memcmp(buffer, &memory.at(13), 7) == 0);

        // read start of block
        emulator.ReadMemory(20U, buffer, 5);
        Assert::IsTrue(memcmp(buffer, &memory.at(20), 5) == 0);

        // read passed end of total memory
        emulator.ReadMemory(28U, buffer, 4);
        Assert::AreEqual(28, static_cast<int>(buffer[0]));
        Assert::AreEqual(29, static_cast<int>(buffer[1]));
        Assert::AreEqual(0, static_cast<int>(buffer[2]));
        Assert::AreEqual(0, static_cast<int>(buffer[3]));

        // read outside memory
        buffer[0] = buffer[1] = buffer[2] = buffer[3] = 0xFF;
        emulator.ReadMemory(37U, buffer, 4);
        Assert::AreEqual(0, static_cast<int>(buffer[0]));
        Assert::AreEqual(0, static_cast<int>(buffer[1]));
        Assert::AreEqual(0, static_cast<int>(buffer[2]));
        Assert::AreEqual(0, static_cast<int>(buffer[3]));
    }

    TEST_METHOD(TestReadMemoryBufferInvalidMemoryBlock)
    {
        constexpr uint8_t zero = 0;
        InitializeMemory();

        EmulatorMemoryContextHarness emulator;
        emulator.AddMemoryBlock(0, 20, &ReadMemory0, &WriteMemory0);
        emulator.AddMemoryBlock(1, 10, nullptr, nullptr);
        emulator.AddMemoryBlock(2, 10, &ReadMemory3, &WriteMemory3);
        Assert::AreEqual({ 40U }, emulator.TotalMemorySize());

        uint8_t buffer[32];

        // simple read within valid block
        emulator.ReadMemory(6U, buffer, 12);
        Assert::IsTrue(memcmp(buffer, &memory.at(6), 11) == 0);

        // simple read within invalid block
        emulator.ReadMemory(22U, buffer, 4);
        Assert::AreEqual(zero, buffer[0]);
        Assert::AreEqual(zero, buffer[1]);
        Assert::AreEqual(zero, buffer[2]);
        Assert::AreEqual(zero, buffer[3]);

        // read across block (valid -> invalid)
        emulator.ReadMemory(16U, buffer, 8);
        Assert::AreEqual(memory.at(16), buffer[0]);
        Assert::AreEqual(memory.at(17), buffer[1]);
        Assert::AreEqual(memory.at(18), buffer[2]);
        Assert::AreEqual(memory.at(19), buffer[3]);
        Assert::AreEqual(zero, buffer[4]);
        Assert::AreEqual(zero, buffer[5]);
        Assert::AreEqual(zero, buffer[6]);
        Assert::AreEqual(zero, buffer[7]);

        // read across block (invalid -> valid)
        emulator.ReadMemory(26U, buffer, 8);
        Assert::AreEqual(zero, buffer[0]);
        Assert::AreEqual(zero, buffer[1]);
        Assert::AreEqual(zero, buffer[2]);
        Assert::AreEqual(zero, buffer[3]);
        Assert::AreEqual(memory.at(30), buffer[4]);
        Assert::AreEqual(memory.at(31), buffer[5]);
        Assert::AreEqual(memory.at(32), buffer[6]);
        Assert::AreEqual(memory.at(33), buffer[7]);
    }

    TEST_METHOD(TestWriteMemoryByte)
    {
        InitializeMemory();

        EmulatorMemoryContextHarness emulator;
        emulator.AddMemoryBlock(0, 20, &ReadMemory0, &WriteMemory0);
        emulator.AddMemoryBlock(1, 10, &ReadMemory2, &WriteMemory2);
        Assert::AreEqual({ 30U }, emulator.TotalMemorySize());

        // attempt to write all 64 bytes
        for (ra::data::ByteAddress i = 0; i < memory.size(); ++i)
            emulator.WriteMemoryByte(i, gsl::narrow_cast<uint8_t>(i + 4));

        // only the 30 mapped bytes should be updated
        for (uint8_t i = 0; i < emulator.TotalMemorySize(); ++i)
            Assert::AreEqual(static_cast<uint8_t>(i + 4), memory.at(i));

        // the others should have their original values
        for (uint8_t i = gsl::narrow_cast<uint8_t>(emulator.TotalMemorySize()); i < memory.size(); ++i)
            Assert::AreEqual(i, memory.at(i));
    }

    TEST_METHOD(TestWriteMemoryByteEvents)
    {
        InitializeMemory();

        EmulatorMemoryContextHarness emulator;
        emulator.AddMemoryBlock(0, 20, &ReadMemory0, &WriteMemory0);
        emulator.AddMemoryBlock(1, 10, &ReadMemory2, &WriteMemory2);
        Assert::AreEqual({ 30U }, emulator.TotalMemorySize());

        EmulatorMemoryContextNotifyHarness notify;
        emulator.AddNotifyTarget(notify);

        // write to first block
        uint8_t nByte = 0xCE;
        emulator.WriteMemoryByte(6U, nByte);
        Assert::AreEqual(6U, notify.nLastByteWrittenAddress);
        Assert::AreEqual(nByte, notify.nLastByteWritten);

        // write to second block
        nByte = 0xEE;
        emulator.WriteMemoryByte(26U, nByte);
        Assert::AreEqual(26U, notify.nLastByteWrittenAddress);
        Assert::AreEqual(nByte, notify.nLastByteWritten);

        // write to invalid address
        emulator.WriteMemoryByte(36U, nByte + 3);
        Assert::AreEqual(26U, notify.nLastByteWrittenAddress);
        Assert::AreEqual(nByte, notify.nLastByteWritten);

        // write partial value
        nByte = 0xC4;
        emulator.WriteMemory(6U, ra::data::Memory::Size::NibbleLower, 0x04);
        Assert::AreEqual(6U, notify.nLastByteWrittenAddress);
        Assert::AreEqual(nByte, notify.nLastByteWritten);
    }

    TEST_METHOD(TestWriteMemory)
    {
        memory.at(4) = 0xA8;
        memory.at(5) = 0x60;
        memory.at(6) = 0x37;
        memory.at(7) = 0x2E;
        memory.at(8) = 0x04;

        EmulatorMemoryContextHarness emulator;
        emulator.AddMemoryBlock(0, 20, &ReadMemory0, &WriteMemory0);

        emulator.WriteMemory(4U, ra::data::Memory::Size::EightBit, 0xFC);
        Assert::AreEqual((uint8_t)0xFC, memory.at(4));
        Assert::AreEqual((uint8_t)0x60, memory.at(5));

        emulator.WriteMemory(4U, ra::data::Memory::Size::EightBit, 0x12345678);
        Assert::AreEqual((uint8_t)0x78, memory.at(4));
        Assert::AreEqual((uint8_t)0x60, memory.at(5));

        emulator.WriteMemory(4U, ra::data::Memory::Size::SixteenBit, 0xABCD);
        Assert::AreEqual((uint8_t)0xCD, memory.at(4));
        Assert::AreEqual((uint8_t)0xAB, memory.at(5));
        Assert::AreEqual((uint8_t)0x37, memory.at(6));

        emulator.WriteMemory(4U, ra::data::Memory::Size::SixteenBit, 0x12345678);
        Assert::AreEqual((uint8_t)0x78, memory.at(4));
        Assert::AreEqual((uint8_t)0x56, memory.at(5));
        Assert::AreEqual((uint8_t)0x37, memory.at(6));

        emulator.WriteMemory(4U, ra::data::Memory::Size::TwentyFourBit, 0x12345678);
        Assert::AreEqual((uint8_t)0x78, memory.at(4));
        Assert::AreEqual((uint8_t)0x56, memory.at(5));
        Assert::AreEqual((uint8_t)0x34, memory.at(6));
        Assert::AreEqual((uint8_t)0x2E, memory.at(7));

        emulator.WriteMemory(4U, ra::data::Memory::Size::ThirtyTwoBit, 0x76543210);
        Assert::AreEqual((uint8_t)0x10, memory.at(4));
        Assert::AreEqual((uint8_t)0x32, memory.at(5));
        Assert::AreEqual((uint8_t)0x54, memory.at(6));
        Assert::AreEqual((uint8_t)0x76, memory.at(7));
        Assert::AreEqual((uint8_t)0x04, memory.at(8));

        emulator.WriteMemory(4U, ra::data::Memory::Size::SixteenBitBigEndian, 0x12345678);
        Assert::AreEqual((uint8_t)0x56, memory.at(4));
        Assert::AreEqual((uint8_t)0x78, memory.at(5));
        Assert::AreEqual((uint8_t)0x54, memory.at(6));

        emulator.WriteMemory(4U, ra::data::Memory::Size::TwentyFourBitBigEndian, 0x12345678);
        Assert::AreEqual((uint8_t)0x34, memory.at(4));
        Assert::AreEqual((uint8_t)0x56, memory.at(5));
        Assert::AreEqual((uint8_t)0x78, memory.at(6));
        Assert::AreEqual((uint8_t)0x76, memory.at(7));

        emulator.WriteMemory(4U, ra::data::Memory::Size::ThirtyTwoBitBigEndian, 0x76543210);
        Assert::AreEqual((uint8_t)0x76, memory.at(4));
        Assert::AreEqual((uint8_t)0x54, memory.at(5));
        Assert::AreEqual((uint8_t)0x32, memory.at(6));
        Assert::AreEqual((uint8_t)0x10, memory.at(7));
        Assert::AreEqual((uint8_t)0x04, memory.at(8));

        memory.at(4) = 0xFF;
        emulator.WriteMemory(4U, ra::data::Memory::Size::Bit0, 0x00);
        Assert::AreEqual((uint8_t)0xFE, memory.at(4));
        emulator.WriteMemory(4U, ra::data::Memory::Size::Bit0, 0x01);
        Assert::AreEqual((uint8_t)0xFF, memory.at(4));

        emulator.WriteMemory(4U, ra::data::Memory::Size::Bit1, 0x00);
        Assert::AreEqual((uint8_t)0xFD, memory.at(4));
        emulator.WriteMemory(4U, ra::data::Memory::Size::Bit1, 0x01);
        Assert::AreEqual((uint8_t)0xFF, memory.at(4));

        emulator.WriteMemory(4U, ra::data::Memory::Size::Bit2, 0x00);
        Assert::AreEqual((uint8_t)0xFB, memory.at(4));
        emulator.WriteMemory(4U, ra::data::Memory::Size::Bit2, 0x01);
        Assert::AreEqual((uint8_t)0xFF, memory.at(4));

        emulator.WriteMemory(4U, ra::data::Memory::Size::Bit3, 0x00);
        Assert::AreEqual((uint8_t)0xF7, memory.at(4));
        emulator.WriteMemory(4U, ra::data::Memory::Size::Bit3, 0x01);
        Assert::AreEqual((uint8_t)0xFF, memory.at(4));

        emulator.WriteMemory(4U, ra::data::Memory::Size::Bit4, 0x00);
        Assert::AreEqual((uint8_t)0xEF, memory.at(4));
        emulator.WriteMemory(4U, ra::data::Memory::Size::Bit4, 0x01);
        Assert::AreEqual((uint8_t)0xFF, memory.at(4));

        emulator.WriteMemory(4U, ra::data::Memory::Size::Bit5, 0x00);
        Assert::AreEqual((uint8_t)0xDF, memory.at(4));
        emulator.WriteMemory(4U, ra::data::Memory::Size::Bit5, 0x01);
        Assert::AreEqual((uint8_t)0xFF, memory.at(4));

        emulator.WriteMemory(4U, ra::data::Memory::Size::Bit6, 0x00);
        Assert::AreEqual((uint8_t)0xBF, memory.at(4));
        emulator.WriteMemory(4U, ra::data::Memory::Size::Bit6, 0x01);
        Assert::AreEqual((uint8_t)0xFF, memory.at(4));

        emulator.WriteMemory(4U, ra::data::Memory::Size::Bit7, 0x00);
        Assert::AreEqual((uint8_t)0x7F, memory.at(4));
        emulator.WriteMemory(4U, ra::data::Memory::Size::Bit7, 0x01);
        Assert::AreEqual((uint8_t)0xFF, memory.at(4));

        emulator.WriteMemory(4U, ra::data::Memory::Size::NibbleLower, 0x00);
        Assert::AreEqual((uint8_t)0xF0, memory.at(4));
        emulator.WriteMemory(4U, ra::data::Memory::Size::NibbleLower, 0x0F);
        Assert::AreEqual((uint8_t)0xFF, memory.at(4));

        emulator.WriteMemory(4U, ra::data::Memory::Size::NibbleUpper, 0x00);
        Assert::AreEqual((uint8_t)0x0F, memory.at(4));
        emulator.WriteMemory(4U, ra::data::Memory::Size::NibbleUpper, 0x0F);
        Assert::AreEqual((uint8_t)0xFF, memory.at(4));

        emulator.WriteMemory(4U, ra::data::Memory::Size::BitCount, 0x00);
        Assert::AreEqual((uint8_t)0xFF, memory.at(4));
        emulator.WriteMemory(4U, ra::data::Memory::Size::BitCount, 0x02);
        Assert::AreEqual((uint8_t)0xFF, memory.at(4));
    }

    TEST_METHOD(TestIsMemoryInsecureCached)
    {
        EmulatorMemoryContextHarness emulator;
        Assert::IsFalse(emulator.IsMemoryInsecure());

        emulator.mockDebuggerDetector.SetDebuggerPresent(true);
        Assert::IsFalse(emulator.IsMemoryInsecure());

        emulator.mockClock.AdvanceTime(std::chrono::seconds(5));
        Assert::IsFalse(emulator.IsMemoryInsecure());

        emulator.mockClock.AdvanceTime(std::chrono::seconds(6));
        Assert::IsTrue(emulator.IsMemoryInsecure());
    }

    TEST_METHOD(TestIsMemoryInsecureRememberedUntilReset)
    {
        EmulatorMemoryContextHarness emulator;
        emulator.mockDebuggerDetector.SetDebuggerPresent(true);
        Assert::IsTrue(emulator.IsMemoryInsecure());

        emulator.mockDebuggerDetector.SetDebuggerPresent(false);
        Assert::IsTrue(emulator.IsMemoryInsecure());

        emulator.mockClock.AdvanceTime(std::chrono::seconds(100));
        Assert::IsTrue(emulator.IsMemoryInsecure());

        emulator.ResetMemoryModified();
        Assert::IsFalse(emulator.IsMemoryInsecure());
    }

    TEST_METHOD(TestIsMemoryInsecureRecheckedWhenReset)
    {
        EmulatorMemoryContextHarness emulator;
        emulator.mockDebuggerDetector.SetDebuggerPresent(true);
        Assert::IsTrue(emulator.IsMemoryInsecure());

        emulator.mockClock.AdvanceTime(std::chrono::seconds(100));
        Assert::IsTrue(emulator.IsMemoryInsecure());

        emulator.ResetMemoryModified();
        Assert::IsTrue(emulator.IsMemoryInsecure());
    }

    TEST_METHOD(TestFormatAddress)
    {
        EmulatorMemoryContextHarness emulator;
        emulator.AddMemoryBlock(0, 0x80, nullptr, nullptr);

        Assert::AreEqual(std::string("0x0000"), emulator.FormatAddress(0x00000000U));
        Assert::AreEqual(std::string("0x00ff"), emulator.FormatAddress(0x000000FFU));
        Assert::AreEqual(std::string("0x0100"), emulator.FormatAddress(0x00000100U));
        Assert::AreEqual(std::string("0xffff"), emulator.FormatAddress(0x0000FFFFU));
        Assert::AreEqual(std::string("0x010000"), emulator.FormatAddress(0x00010000U));
        Assert::AreEqual(std::string("0xffffff"), emulator.FormatAddress(0x00FFFFFFU));
        Assert::AreEqual(std::string("0x01000000"), emulator.FormatAddress(0x01000000U));
        Assert::AreEqual(std::string("0xffffffff"), emulator.FormatAddress(0xFFFFFFFFU));

        emulator.AddMemoryBlock(1, 0x80, nullptr, nullptr); // total size = 0x100
        Assert::AreEqual(std::string("0x0000"), emulator.FormatAddress(0x00000000U));
        Assert::AreEqual(std::string("0x00ff"), emulator.FormatAddress(0x000000FFU));
        Assert::AreEqual(std::string("0x0100"), emulator.FormatAddress(0x00000100U));
        Assert::AreEqual(std::string("0xffff"), emulator.FormatAddress(0x0000FFFFU));
        Assert::AreEqual(std::string("0x010000"), emulator.FormatAddress(0x00010000U));
        Assert::AreEqual(std::string("0xffffff"), emulator.FormatAddress(0x00FFFFFFU));
        Assert::AreEqual(std::string("0x01000000"), emulator.FormatAddress(0x01000000U));
        Assert::AreEqual(std::string("0xffffffff"), emulator.FormatAddress(0xFFFFFFFFU));

        emulator.AddMemoryBlock(2, 0xFF00, nullptr, nullptr); // total size = 0x10000
        Assert::AreEqual(std::string("0x0000"), emulator.FormatAddress(0x00000000U));
        Assert::AreEqual(std::string("0x00ff"), emulator.FormatAddress(0x000000FFU));
        Assert::AreEqual(std::string("0x0100"), emulator.FormatAddress(0x00000100U));
        Assert::AreEqual(std::string("0xffff"), emulator.FormatAddress(0x0000FFFFU));
        Assert::AreEqual(std::string("0x010000"), emulator.FormatAddress(0x00010000U));
        Assert::AreEqual(std::string("0xffffff"), emulator.FormatAddress(0x00FFFFFFU));
        Assert::AreEqual(std::string("0x01000000"), emulator.FormatAddress(0x01000000U));
        Assert::AreEqual(std::string("0xffffffff"), emulator.FormatAddress(0xFFFFFFFFU));

        emulator.AddMemoryBlock(3, 1, nullptr, nullptr); // total size = 0x10001
        Assert::AreEqual(std::string("0x000000"), emulator.FormatAddress(0x00000000U));
        Assert::AreEqual(std::string("0x0000ff"), emulator.FormatAddress(0x000000FFU));
        Assert::AreEqual(std::string("0x000100"), emulator.FormatAddress(0x00000100U));
        Assert::AreEqual(std::string("0x00ffff"), emulator.FormatAddress(0x0000FFFFU));
        Assert::AreEqual(std::string("0x010000"), emulator.FormatAddress(0x00010000U));
        Assert::AreEqual(std::string("0xffffff"), emulator.FormatAddress(0x00FFFFFFU));
        Assert::AreEqual(std::string("0x01000000"), emulator.FormatAddress(0x01000000U));
        Assert::AreEqual(std::string("0xffffffff"), emulator.FormatAddress(0xFFFFFFFFU));

        emulator.AddMemoryBlock(4, 0xFEFFFF, nullptr, nullptr); // total size = 0x1000000
        Assert::AreEqual(std::string("0x000000"), emulator.FormatAddress(0x00000000U));
        Assert::AreEqual(std::string("0x0000ff"), emulator.FormatAddress(0x000000FFU));
        Assert::AreEqual(std::string("0x000100"), emulator.FormatAddress(0x00000100U));
        Assert::AreEqual(std::string("0x00ffff"), emulator.FormatAddress(0x0000FFFFU));
        Assert::AreEqual(std::string("0x010000"), emulator.FormatAddress(0x00010000U));
        Assert::AreEqual(std::string("0xffffff"), emulator.FormatAddress(0x00FFFFFFU));
        Assert::AreEqual(std::string("0x01000000"), emulator.FormatAddress(0x01000000U));
        Assert::AreEqual(std::string("0xffffffff"), emulator.FormatAddress(0xFFFFFFFFU));

        emulator.AddMemoryBlock(5, 1, nullptr, nullptr); // total size = 0x1000001
        Assert::AreEqual(std::string("0x00000000"), emulator.FormatAddress(0x00000000U));
        Assert::AreEqual(std::string("0x000000ff"), emulator.FormatAddress(0x000000FFU));
        Assert::AreEqual(std::string("0x00000100"), emulator.FormatAddress(0x00000100U));
        Assert::AreEqual(std::string("0x0000ffff"), emulator.FormatAddress(0x0000FFFFU));
        Assert::AreEqual(std::string("0x00010000"), emulator.FormatAddress(0x00010000U));
        Assert::AreEqual(std::string("0x00ffffff"), emulator.FormatAddress(0x00FFFFFFU));
        Assert::AreEqual(std::string("0x01000000"), emulator.FormatAddress(0x01000000U));
        Assert::AreEqual(std::string("0xffffffff"), emulator.FormatAddress(0xFFFFFFFFU));
    }

    TEST_METHOD(TestCaptureMemory)
    {
        EmulatorMemoryContextHarness emulator;

        InitializeMemory();
        emulator.AddMemoryBlock(0, 20, &ReadMemory0, &WriteMemory0);
        emulator.AddMemoryBlock(1, 10, &ReadMemory2, &WriteMemory2);

        std::vector<ra::data::CapturedMemoryBlock> vBlocks;
        emulator.CaptureMemory(vBlocks, 0, 30, 0);
        Assert::AreEqual({ 2 }, vBlocks.size());

        Assert::AreEqual(20U, vBlocks.at(0).GetBytesSize());
        const auto* pBytes = vBlocks.at(0).GetBytes();
        Expects(pBytes != nullptr);
        for (size_t i = 0; i < 20; i++)
            Assert::AreEqual(memory.at(i), pBytes[i]);

        Assert::AreEqual(10U, vBlocks.at(1).GetBytesSize());
        pBytes = vBlocks.at(1).GetBytes();
        Expects(pBytes != nullptr);
        for (size_t i = 0; i < 10; i++)
            Assert::AreEqual(memory.at(i + 20), pBytes[i]);
    }

    TEST_METHOD(TestCaptureMemoryOffset)
    {
        EmulatorMemoryContextHarness emulator;

        InitializeMemory();
        emulator.AddMemoryBlock(0, 20, &ReadMemory0, &WriteMemory0);
        emulator.AddMemoryBlock(1, 10, &ReadMemory2, &WriteMemory2);

        std::vector<ra::data::CapturedMemoryBlock> vBlocks;
        emulator.CaptureMemory(vBlocks, 8, 20, 0);
        Assert::AreEqual({ 2 }, vBlocks.size());

        Assert::AreEqual(12U, vBlocks.at(0).GetBytesSize());
        const auto* pBytes = vBlocks.at(0).GetBytes();
        Expects(pBytes != nullptr);
        for (size_t i = 0; i < 12; i++)
            Assert::AreEqual(memory.at(i + 8), pBytes[i]);

        Assert::AreEqual(8U, vBlocks.at(1).GetBytesSize());
        pBytes = vBlocks.at(1).GetBytes();
        Expects(pBytes != nullptr);
        for (size_t i = 0; i < 8; i++)
            Assert::AreEqual(memory.at(i + 20), pBytes[i]);
    }

    TEST_METHOD(TestCaptureMemorySecondBlockOnly)
    {
        EmulatorMemoryContextHarness emulator;

        InitializeMemory();
        emulator.AddMemoryBlock(0, 20, &ReadMemory0, &WriteMemory0);
        emulator.AddMemoryBlock(1, 10, &ReadMemory2, &WriteMemory2);

        std::vector<ra::data::CapturedMemoryBlock> vBlocks;
        emulator.CaptureMemory(vBlocks, 20, 10, 0);
        Assert::AreEqual({ 1 }, vBlocks.size());

        Assert::AreEqual(10U, vBlocks.at(0).GetBytesSize());
        const auto* pBytes = vBlocks.at(0).GetBytes();
        Expects(pBytes != nullptr);
        for (size_t i = 0; i < 10; i++)
            Assert::AreEqual(memory.at(i + 20), pBytes[i]);
    }

    TEST_METHOD(TestCaptureMemoryGap)
    {
        EmulatorMemoryContextHarness emulator;

        InitializeMemory();
        emulator.AddMemoryBlock(0, 10, &ReadMemory0, &WriteMemory0);
        emulator.AddMemoryBlock(1, 10, nullptr, nullptr);
        emulator.AddMemoryBlock(2, 10, &ReadMemory2, &WriteMemory2);

        std::vector<ra::data::CapturedMemoryBlock> vBlocks;
        emulator.CaptureMemory(vBlocks, 0, 30, 0);
        Assert::AreEqual({ 2 }, vBlocks.size());

        Assert::AreEqual(10U, vBlocks.at(0).GetBytesSize());
        const auto* pBytes = vBlocks.at(0).GetBytes();
        Expects(pBytes != nullptr);
        for (size_t i = 0; i < 10; i++)
            Assert::AreEqual(memory.at(i), pBytes[i]);

        Assert::AreEqual(10U, vBlocks.at(1).GetBytesSize());
        pBytes = vBlocks.at(1).GetBytes();
        Expects(pBytes != nullptr);
        for (size_t i = 0; i < 10; i++)
            Assert::AreEqual(memory.at(i + 20), pBytes[i]);
    }

    TEST_METHOD(TestCaptureMemoryGapBoundary)
    {
        EmulatorMemoryContextHarness emulator;

        InitializeMemory();
        emulator.AddMemoryBlock(0, 10, &ReadMemory0, &WriteMemory0);
        emulator.AddMemoryBlock(1, 10, nullptr, nullptr);
        emulator.AddMemoryBlock(2, 10, &ReadMemory2, &WriteMemory2);

        std::vector<ra::data::CapturedMemoryBlock> vBlocks;
        emulator.CaptureMemory(vBlocks, 20, 10, 0);
        Assert::AreEqual({ 1 }, vBlocks.size());

        Assert::AreEqual(10U, vBlocks.at(0).GetBytesSize());
        const auto* pBytes = vBlocks.at(0).GetBytes();
        Expects(pBytes != nullptr);
        for (size_t i = 0; i < 10; i++)
            Assert::AreEqual(memory.at(i + 20), pBytes[i]);
    }
};

std::array<uint8_t, 64> EmulatorMemoryContext_Tests::memory;

} // namespace tests
} // namespace impl
} // namespace context
} // namespace ra
