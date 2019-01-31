#include "ConsoleContext.hh"

#include "RA_Log.h"
#include "RA_StringUtils.h"

namespace ra {
namespace data {

const std::vector<ConsoleContext::MemoryRegion> ConsoleContext::m_vEmptyRegions;

// ===== GameBoy | GameBoy Color =====

class GameBoyConsoleContext : public ConsoleContext
{
public:
    GameBoyConsoleContext(ConsoleID nId, const std::wstring&& sName) noexcept : ConsoleContext(nId, std::move(sName)) {}

    const std::vector<MemoryRegion>& MemoryRegions() const noexcept override { return m_vMemoryRegions; }

private:
    static const std::vector<MemoryRegion> m_vMemoryRegions;
};

// GameBoy memory is directly exposed to the toolkit
const std::vector<ConsoleContext::MemoryRegion> GameBoyConsoleContext::m_vMemoryRegions =
{
    { 0x0000U, 0x00FFU, ConsoleContext::AddressType::HardwareController, "Interrupt vector" },
    { 0x0100U, 0x014FU, ConsoleContext::AddressType::VirtualRAM, "Cartridge header" },
    { 0x0150U, 0x3FFFU, ConsoleContext::AddressType::VirtualRAM, "Cartridge ROM (fixed)" }, // bank 0
    { 0x4000U, 0x7FFFU, ConsoleContext::AddressType::VirtualRAM, "Cartridge ROM (paged)" }, // bank 1-XX (switchable)
    { 0x8000U, 0x97FFU, ConsoleContext::AddressType::VideoRAM, "Tile RAM" },
    { 0x9800U, 0x9BFFU, ConsoleContext::AddressType::VideoRAM, "BG1 map data" },
    { 0x9C00U, 0x9FFFU, ConsoleContext::AddressType::VideoRAM, "BG2 map data" },
    { 0xA000U, 0xBFFFU, ConsoleContext::AddressType::SaveRAM, "Cartridge RAM"},
    { 0xC000U, 0xCFFFU, ConsoleContext::AddressType::SystemRAM, "System RAM (fixed)" },
    { 0xD000U, 0xDFFFU, ConsoleContext::AddressType::SystemRAM, "System RAM (paged)" },
    { 0xE000U, 0xFDFFU, ConsoleContext::AddressType::VirtualRAM, "Echo RAM" },
    { 0xFE00U, 0xFE9FU, ConsoleContext::AddressType::VideoRAM, "Sprite RAM"},
    { 0xFEA0U, 0xFEFFU, ConsoleContext::AddressType::VirtualRAM, "Unusable"},
    { 0xFF00U, 0xFF7FU, ConsoleContext::AddressType::HardwareController, "Hardware I/O"},
    { 0xFF80U, 0xFFFEU, ConsoleContext::AddressType::SystemRAM, "Quick RAM"},
    { 0xFFFFU, 0xFFFFU, ConsoleContext::AddressType::HardwareController, "Interrupt enable"},
};

// ===== GameBoy Advance =====

class GameBoyAdvanceConsoleContext : public ConsoleContext
{
public:
    GameBoyAdvanceConsoleContext() noexcept : ConsoleContext(ConsoleID::GBA, L"GameBoy Advance") {}

    const std::vector<MemoryRegion>& MemoryRegions() const noexcept override { return m_vMemoryRegions; }

private:
    static const std::vector<MemoryRegion> m_vMemoryRegions;
};

// GameBoy Advance memory is exposed as two chunks
const std::vector<ConsoleContext::MemoryRegion> GameBoyAdvanceConsoleContext::m_vMemoryRegions =
{
    { 0x000000U, 0x007FFFU, ConsoleContext::AddressType::SaveRAM, "Cartridge RAM" }, // internal RAM (normally $03000000-$03007FFF)
    { 0x008000U, 0x047FFFU, ConsoleContext::AddressType::SystemRAM, "System RAM" }, // work RAM (normally $02000000-$0203FFFF)
};

// ===== MegaDrive / Genesis | Sega 32X | Sega CD =====

class MegaDriveConsoleContext : public ConsoleContext
{
public:
    MegaDriveConsoleContext(ConsoleID nId, const std::wstring&& sName) noexcept : ConsoleContext(nId, std::move(sName)) {}

    const std::vector<MemoryRegion>& MemoryRegions() const noexcept override { return m_vMemoryRegions; }

private:
    static const std::vector<MemoryRegion> m_vMemoryRegions;
};

// MegaDrive memory is exposed as two chunks
const std::vector<ConsoleContext::MemoryRegion> MegaDriveConsoleContext::m_vMemoryRegions =
{
    { 0x000000U, 0x00FFFFU, ConsoleContext::AddressType::SystemRAM, "System RAM" }, // 68000 RAM (normally $FF0000-$FFFFFF)
    { 0x010000U, 0x01FFFFU, ConsoleContext::AddressType::SaveRAM, "Cartridge RAM" }, // work RAM (normally $000000-$00FFFF)
};

// ===== Nintendo 64 =====

class Nintendo64ConsoleContext : public ConsoleContext
{
public:
    Nintendo64ConsoleContext() noexcept : ConsoleContext(ConsoleID::N64, L"Nintendo 64") {}

    const std::vector<MemoryRegion>& MemoryRegions() const noexcept override { return m_vMemoryRegions; }

private:
    static const std::vector<MemoryRegion> m_vMemoryRegions;
};

// Nintendo 64 memory is exposed as one chunk, that's actually two or three chunks
// https://raw.githubusercontent.com/mikeryan/n64dev/master/docs/n64ops/n64ops%23h.txt
const std::vector<ConsoleContext::MemoryRegion> Nintendo64ConsoleContext::m_vMemoryRegions =
{
    { 0x000000U, 0x1FFFFFU, ConsoleContext::AddressType::SystemRAM, "System RAM" }, // RDRAM range 0 (2MB)
    { 0x200000U, 0x3FFFFFU, ConsoleContext::AddressType::SystemRAM, "System RAM" }, // RDRAM range 1 (2MB)
    { 0x400000U, 0x7FFFFFU, ConsoleContext::AddressType::SystemRAM, "System RAM" }, // Expansion PAK memory (4MB)
};

// ===== NES =====

class NintendoEntertainmentSystemConsoleContext : public ConsoleContext
{
public:
    NintendoEntertainmentSystemConsoleContext() noexcept : ConsoleContext(ConsoleID::NES, L"Nintendo Entertainment System") {}

    const std::vector<MemoryRegion>& MemoryRegions() const noexcept override { return m_vMemoryRegions; }

private:
    static const std::vector<MemoryRegion> m_vMemoryRegions;
};

// NES memory is directly exposed to the toolkit
const std::vector<ConsoleContext::MemoryRegion> NintendoEntertainmentSystemConsoleContext::m_vMemoryRegions =
{
    { 0x0000U, 0x07FFU, ConsoleContext::AddressType::SystemRAM, "System RAM" },
    { 0x0800U, 0x0FFFU, ConsoleContext::AddressType::VirtualRAM, "Mirror RAM" }, // duplicates memory from $0000-$07FF
    { 0x1000U, 0x17FFU, ConsoleContext::AddressType::VirtualRAM, "Mirror RAM" }, // duplicates memory from $0000-$07FF
    { 0x1800U, 0x1FFFU, ConsoleContext::AddressType::VirtualRAM, "Mirror RAM" }, // duplicates memory from $0000-$07FF
    { 0x2000U, 0x2007U, ConsoleContext::AddressType::HardwareController, "PPU Register" },
    { 0x2008U, 0x3FFFU, ConsoleContext::AddressType::VirtualRAM, "Mirrored PPU Register" }, // repeats every 8 bytes
    { 0x4000U, 0x4017U, ConsoleContext::AddressType::HardwareController, "APU and I/O register" },
    { 0x4018U, 0x401FU, ConsoleContext::AddressType::HardwareController, "APU and I/O test register" },
    { 0x4020U, 0x5FFFU, ConsoleContext::AddressType::Unknown, "Cartridge data"}, // varies by mapper
    { 0x6000U, 0x7FFFU, ConsoleContext::AddressType::SaveRAM, "Cartridge RAM"},
    { 0x8000U, 0xFFFFU, ConsoleContext::AddressType::VirtualRAM, "Cartridge ROM"},
};

// ===== SNES =====

class SuperNESConsoleContext : public ConsoleContext
{
public:
    SuperNESConsoleContext() noexcept : ConsoleContext(ConsoleID::SNES, L"Super NES") {}

    const std::vector<MemoryRegion>& MemoryRegions() const noexcept override { return m_vMemoryRegions; }

private:
    static const std::vector<MemoryRegion> m_vMemoryRegions;
};

// SNES memory is exposed as two chunks
const std::vector<ConsoleContext::MemoryRegion> SuperNESConsoleContext::m_vMemoryRegions =
{
    { 0x000000U, 0x01FFFFU, ConsoleContext::AddressType::SystemRAM, "System RAM" }, // work RAM (normally $7E0000-$7FFFFF)
    { 0x020000U, 0x03FFFFU, ConsoleContext::AddressType::SaveRAM, "Cartridge RAM" }, // cartridge RAM (normally $FE0000-$FF0000; up to 1MB, typically 32KB or less)
};

// ===== ConsoleContext =====

std::unique_ptr<ConsoleContext> ConsoleContext::GetContext(ConsoleID nId)
{
    switch (nId)
    {
        case ConsoleID::Amiga:
            return std::make_unique<ConsoleContext>(nId, L"Amiga");

        case ConsoleID::AmigaST:
            return std::make_unique<ConsoleContext>(nId, L"Amiga ST");

        case ConsoleID::AmstradCPC:
            return std::make_unique<ConsoleContext>(nId, L"Amstrad CPC");

        case ConsoleID::AppleII:
            return std::make_unique<ConsoleContext>(nId, L"Apple II");

        case ConsoleID::Arcade:
            return std::make_unique<ConsoleContext>(nId, L"Arcade");

        case ConsoleID::Atari2600:
            return std::make_unique<ConsoleContext>(nId, L"Atari 2600");

        case ConsoleID::Atari5200:
            return std::make_unique<ConsoleContext>(nId, L"Atari 5200");

        case ConsoleID::Atari7800:
            return std::make_unique<ConsoleContext>(nId, L"Atari 7800");

        case ConsoleID::C64:
            return std::make_unique<ConsoleContext>(nId, L"Commodore 64");

        case ConsoleID::CDi:
            return std::make_unique<ConsoleContext>(nId, L"CD-I");

        case ConsoleID::Colecovision:
            return std::make_unique<ConsoleContext>(nId, L"Colecovision");

        case ConsoleID::Dreamcast:
            return std::make_unique<ConsoleContext>(nId, L"Dreamcast");

        case ConsoleID::DS:
            return std::make_unique<ConsoleContext>(nId, L"Nintendo DS");

        case ConsoleID::Events:
            return std::make_unique<ConsoleContext>(nId, L"Events");

        case ConsoleID::GameCube:
            return std::make_unique<ConsoleContext>(nId, L"GameCube");

        case ConsoleID::GameGear:
            return std::make_unique<ConsoleContext>(nId, L"GameGear");

        case ConsoleID::GB:
            return std::make_unique<GameBoyConsoleContext>(ConsoleID::GB, L"GameBoy");

        case ConsoleID::GBA:
            return std::make_unique<GameBoyAdvanceConsoleContext>();

        case ConsoleID::GBC:
            return std::make_unique<GameBoyConsoleContext>(ConsoleID::GBC, L"GameBoy Color");

        case ConsoleID::Intellivision:
            return std::make_unique<ConsoleContext>(nId, L"Intellivision");

        case ConsoleID::Jaguar:
            return std::make_unique<ConsoleContext>(nId, L"Jaguar");

        case ConsoleID::Lynx:
            return std::make_unique<ConsoleContext>(nId, L"Lynx");

        case ConsoleID::MasterSystem:
            return std::make_unique<MegaDriveConsoleContext>(ConsoleID::MasterSystem, L"Master System");

        case ConsoleID::MegaDrive:
            return std::make_unique<MegaDriveConsoleContext>(ConsoleID::MegaDrive, L"MegaDrive / Genesis");

        case ConsoleID::MSDOS:
            return std::make_unique<ConsoleContext>(nId, L"MS-DOS");

        case ConsoleID::MSX:
            return std::make_unique<ConsoleContext>(nId, L"MSX");

        case ConsoleID::N64:
            return std::make_unique<Nintendo64ConsoleContext>();

        case ConsoleID::NeoGeoPocket:
            return std::make_unique<ConsoleContext>(nId, L"NeoGeo Pocket");

        case ConsoleID::NES:
            return std::make_unique<NintendoEntertainmentSystemConsoleContext>();

        case ConsoleID::PC8800:
            return std::make_unique<ConsoleContext>(nId, L"PC-8800");

        case ConsoleID::PC9800:
            return std::make_unique<ConsoleContext>(nId, L"PC-9800");

        case ConsoleID::PCEngine:
            return std::make_unique<ConsoleContext>(nId, L"PCEngine / TurboGrafx 16");

        case ConsoleID::PCFX:
            return std::make_unique<ConsoleContext>(nId, L"PCFX");

        case ConsoleID::PlayStation:
            return std::make_unique<ConsoleContext>(nId, L"PlayStation");

        case ConsoleID::PlayStation2:
            return std::make_unique<ConsoleContext>(nId, L"PlayStation 2");

        case ConsoleID::PSP:
            return std::make_unique<ConsoleContext>(nId, L"PlayStation Portable");

        case ConsoleID::Saturn:
            return std::make_unique<ConsoleContext>(nId, L"Saturn");

        case ConsoleID::Sega32X:
            return std::make_unique<MegaDriveConsoleContext>(ConsoleID::Sega32X, L"SEGA 32X");

        case ConsoleID::SegaCD:
            return std::make_unique<ConsoleContext>(nId, L"SEGA CD");

        case ConsoleID::SG1000:
            return std::make_unique<ConsoleContext>(nId, L"SG-1000");

        case ConsoleID::SNES:
            return std::make_unique<SuperNESConsoleContext>();

        case ConsoleID::ThreeDO:
            return std::make_unique<ConsoleContext>(nId, L"3D0");

        case ConsoleID::Vectrex:
            return std::make_unique<ConsoleContext>(nId, L"Vectrex");

        case ConsoleID::VIC20:
            return std::make_unique<ConsoleContext>(nId, L"VIC-20");

        case ConsoleID::VirtualBoy:
            return std::make_unique<ConsoleContext>(nId, L"Virtual Boy");

        case ConsoleID::WII:
            return std::make_unique<ConsoleContext>(nId, L"Wii");

        case ConsoleID::WIIU:
            return std::make_unique<ConsoleContext>(nId, L"Wii-U");

        case ConsoleID::WonderSwan:
            return std::make_unique<ConsoleContext>(nId, L"WonderSwan");

        case ConsoleID::X68K:
            return std::make_unique<ConsoleContext>(nId, L"X68K");

        case ConsoleID::Xbox:
            return std::make_unique<ConsoleContext>(nId, L"XBOX");

        case ConsoleID::XboxOne:
            return std::make_unique<ConsoleContext>(nId, L"Xbox One");

        case ConsoleID::ZX81:
            return std::make_unique<ConsoleContext>(nId, L"ZX-81");

        default:
            return std::make_unique<ConsoleContext>(ConsoleID::UnknownConsoleID, L"Unknown");
    }
}

} // namespace data
} // namespace ra
