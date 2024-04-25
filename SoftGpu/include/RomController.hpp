/**
 * @file
 */
#pragma once

#include <Objects.hpp>
#include <NumTypes.hpp>
#include <ConPrinter.hpp>
#include <PCIController.hpp>

class Processor;

class RomController final
{
    DEFAULT_DESTRUCT(RomController);
    DELETE_CM(RomController);
public:
    static inline constexpr const char* EXPANSION_ROM_ENV_VAR = "SOFT_GPU_ROM";
public:
    RomController(Processor* const processor) noexcept
        : m_Processor(processor)
    {
        InitRom();
    }

    u16 PciReadExpansionRom(const u64 address, const u16 size, u32* const data) noexcept;
private:
    /**
     * @brief Initializes the ROM.
     *
     * This method is responsible for initializing the ROM. It first clears the expansion ROM memory.
     * Then it retrieves the path to the ROM file from the environment variable "SOFT_GPU_ROM".
     * If the path is valid, it opens the ROM file and loads its contents into the expansion ROM memory.
     * If any step fails, an error message is printed to the console.
     *
     * @note This method is called in the constructor of the RomController class.
     *
     * @exception This method does not throw exceptions but handles them internally.
     *
     * @see RomController::RomController(Processor* const processor) noexcept
     */
    void InitRom() noexcept
    {
        ::std::memset(m_ExpansionRom, 0, sizeof(m_ExpansionRom));

        size_t trueLibraryPathLength = 0;
        if(const errno_t err = getenv_s(&trueLibraryPathLength, nullptr, 0, EXPANSION_ROM_ENV_VAR))
        {
            ConPrinter::PrintLn(u8"Initial call to getenv_s failed, Error: {}, Path Length: {}.", err, static_cast<unsigned>(trueLibraryPathLength));
            return;
        }

        if(!trueLibraryPathLength)
        {
            ConPrinter::PrintLn(u8"Failed to get the path length to a ROM file to load, Path Length: {}.", static_cast<unsigned>(trueLibraryPathLength));
            return;
        }

        char* romPathBuffer = new(::std::nothrow) char[trueLibraryPathLength];

        if(!romPathBuffer)
        {
            ConPrinter::PrintLn("Failed to allocate space for the device path.");
            return;
        }

        if(const errno_t err = getenv_s(&trueLibraryPathLength, romPathBuffer, trueLibraryPathLength, EXPANSION_ROM_ENV_VAR))
        {
            delete[] romPathBuffer;
            ConPrinter::PrintLn("Failed to get the path to ROM file to load with a valid buffer. Error: {}", err);
            return;
        }

        ConPrinter::PrintLn(u8"Loading ROM file: {}", romPathBuffer);

        FILE* romFile;

        const errno_t romFileOpenError = fopen_s(&romFile, romPathBuffer, "rb");

        delete[] romPathBuffer;

        if(romFileOpenError)
        {
            ConPrinter::PrintLn(u8"Could not open ROM file.");
            return;
        }

        if(fseek(romFile, 0, SEEK_END))
        {
            (void) fclose(romFile);
            ConPrinter::PrintLn(u8"Could not load ROM file [seek_end].");
            return;
        }

        const long fileSize = ftell(romFile);

        if(static_cast<uSys>(fileSize) > sizeof(m_ExpansionRom))
        {
            (void) fclose(romFile);
            ConPrinter::PrintLn(u8"Could not load ROM file [file size].");
            return;
        }

        if(fseek(romFile, 0, SEEK_SET))
        {
            (void) fclose(romFile);
            ConPrinter::PrintLn(u8"Could not load ROM file [seek_set].");
            return;
        }

        (void) fread(m_ExpansionRom, 1, fileSize, romFile);

        (void) fclose(romFile);

        ConPrinter::PrintLn(u8"Rom First Bytes: 0x{XP0}{XP0}{XP0}{XP0}", m_ExpansionRom[0], m_ExpansionRom[1], m_ExpansionRom[2], m_ExpansionRom[3]);
        ConPrinter::PrintLn(u8"Last Non Zero Byte (Should Be A7): 0x{XP0}", m_ExpansionRom[0x6DEB]);
        ConPrinter::PrintLn(u8"Last Byte: 0x{XP0}", m_ExpansionRom[0x6DFF]);
    }
private:
    Processor* m_Processor;
    u8 m_ExpansionRom[PciController::EXPANSION_ROM_SIZE];
};
