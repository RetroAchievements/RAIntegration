#include "ConsoleContext.hh"

#include "RA_Log.h"
#include "RA_StringUtils.h"

namespace ra {
namespace data {

const std::vector<ConsoleContext::MemoryRegion> ConsoleContext::m_vEmptyRegions;

// ===== Apple II =====

class AppleIIConsoleContext : public ConsoleContext
{
public:
    GSL_SUPPRESS_F6 AppleIIConsoleContext() noexcept : ConsoleContext(ConsoleID::AppleII, L"Apple II") {}

    const std::vector<MemoryRegion>& MemoryRegions() const noexcept override { return m_vMemoryRegions; }

private:
    static const std::vector<MemoryRegion> m_vMemoryRegions;
};

const std::vector<ConsoleContext::MemoryRegion> AppleIIConsoleContext::m_vMemoryRegions =
{
    { 0x000000U, 0x00FFFFU, ConsoleContext::AddressType::SystemRAM, "Main RAM" },
    { 0x010000U, 0x01FFFFU, ConsoleContext::AddressType::SystemRAM, "Auxiliary RAM" },
};

// ===== Atari 2600 =====

class Atari2600ConsoleContext : public ConsoleContext
{
public:
    GSL_SUPPRESS_F6 Atari2600ConsoleContext() noexcept : ConsoleContext(ConsoleID::Atari2600, L"Atari2600") {}

    const std::vector<MemoryRegion>& MemoryRegions() const noexcept override { return m_vMemoryRegions; }

private:
    static const std::vector<MemoryRegion> m_vMemoryRegions;
};

const std::vector<ConsoleContext::MemoryRegion> Atari2600ConsoleContext::m_vMemoryRegions =
{
    { 0x000000U, 0x00007FU, ConsoleContext::AddressType::SystemRAM, "System RAM" }, // normally $80-$FF
};

// ===== Atari 7800 =====

class Atari7800ConsoleContext : public ConsoleContext
{
public:
    GSL_SUPPRESS_F6 Atari7800ConsoleContext() noexcept : ConsoleContext(ConsoleID::Atari7800, L"Atari7800") {}

    const std::vector<MemoryRegion>& MemoryRegions() const noexcept override { return m_vMemoryRegions; }

private:
    static const std::vector<MemoryRegion> m_vMemoryRegions;
};

// http://www.atarihq.com/danb/files/78map.txt
// http://pdf.textfiles.com/technical/7800_devkit.pdf
const std::vector<ConsoleContext::MemoryRegion> Atari7800ConsoleContext::m_vMemoryRegions =
{
    { 0x000000U, 0x0017FFU, ConsoleContext::AddressType::HardwareController, "Hardware Interface" },
    { 0x001800U, 0x0027FFU, ConsoleContext::AddressType::SystemRAM, "System RAM" },
    { 0x002800U, 0x002FFFU, ConsoleContext::AddressType::VirtualRAM, "Mirrored RAM" },
    { 0x003000U, 0x0037FFU, ConsoleContext::AddressType::VirtualRAM, "Mirrored RAM" },
    { 0x003800U, 0x003FFFU, ConsoleContext::AddressType::VirtualRAM, "Mirrored RAM" },
    { 0x004000U, 0x007FFFU, ConsoleContext::AddressType::SaveRAM, "Cartridge RAM" },
    { 0x008000U, 0x00FFFFU, ConsoleContext::AddressType::ReadOnlyMemory, "Cartridge ROM" },
};

// ===== ColecoVision =====

class ColecoVisionConsoleContext : public ConsoleContext
{
public:
    GSL_SUPPRESS_F6 ColecoVisionConsoleContext() noexcept : ConsoleContext(ConsoleID::Colecovision, L"ColecoVision") {}

    const std::vector<MemoryRegion>& MemoryRegions() const noexcept override { return m_vMemoryRegions; }

private:
    static const std::vector<MemoryRegion> m_vMemoryRegions;
};

// ColecoVision memory is exposed as a single chunk
const std::vector<ConsoleContext::MemoryRegion> ColecoVisionConsoleContext::m_vMemoryRegions =
{
    { 0x000000U, 0x0003FFU, ConsoleContext::AddressType::SystemRAM, "System RAM" }, // RAM (normally $6000-$63FF)
};

// ===== GameBoy | GameBoy Color =====

class GameBoyConsoleContext : public ConsoleContext
{
public:
    GameBoyConsoleContext(ConsoleID nId, std::wstring&& sName) noexcept : ConsoleContext(nId, std::move(sName)) {}

    const std::vector<MemoryRegion>& MemoryRegions() const noexcept override { return m_vMemoryRegions; }

private:
    static const std::vector<MemoryRegion> m_vMemoryRegions;
};

// GameBoy memory is directly exposed to the toolkit
const std::vector<ConsoleContext::MemoryRegion> GameBoyConsoleContext::m_vMemoryRegions =
{
    { 0x0000U, 0x00FFU, ConsoleContext::AddressType::HardwareController, "Interrupt vector" },
    { 0x0100U, 0x014FU, ConsoleContext::AddressType::ReadOnlyMemory, "Cartridge header" },
    { 0x0150U, 0x3FFFU, ConsoleContext::AddressType::ReadOnlyMemory, "Cartridge ROM (fixed)" }, // bank 0
    { 0x4000U, 0x7FFFU, ConsoleContext::AddressType::ReadOnlyMemory, "Cartridge ROM (paged)" }, // bank 1-XX (switchable)
    { 0x8000U, 0x97FFU, ConsoleContext::AddressType::VideoRAM, "Tile RAM" },
    { 0x9800U, 0x9BFFU, ConsoleContext::AddressType::VideoRAM, "BG1 map data" },
    { 0x9C00U, 0x9FFFU, ConsoleContext::AddressType::VideoRAM, "BG2 map data" },
    { 0xA000U, 0xBFFFU, ConsoleContext::AddressType::SaveRAM, "Cartridge RAM"},
    { 0xC000U, 0xCFFFU, ConsoleContext::AddressType::SystemRAM, "System RAM (fixed)" },
    { 0xD000U, 0xDFFFU, ConsoleContext::AddressType::SystemRAM, "System RAM (bank 1)" },
    { 0xE000U, 0xFDFFU, ConsoleContext::AddressType::VirtualRAM, "Echo RAM" },
    { 0xFE00U, 0xFE9FU, ConsoleContext::AddressType::VideoRAM, "Sprite RAM"},
    { 0xFEA0U, 0xFEFFU, ConsoleContext::AddressType::ReadOnlyMemory, "Unusable"},
    { 0xFF00U, 0xFF7FU, ConsoleContext::AddressType::HardwareController, "Hardware I/O"},
    { 0xFF80U, 0xFFFEU, ConsoleContext::AddressType::SystemRAM, "Quick RAM"},
    { 0xFFFFU, 0xFFFFU, ConsoleContext::AddressType::HardwareController, "Interrupt enable"},
    { 0x10000U, 0x15FFFU, ConsoleContext::AddressType::SystemRAM, "System RAM (banks 2-7, GBC only)" },
};

// ===== GameBoy Advance =====

class GameBoyAdvanceConsoleContext : public ConsoleContext
{
public:
    GSL_SUPPRESS_F6 GameBoyAdvanceConsoleContext() noexcept : ConsoleContext(ConsoleID::GBA, L"GameBoy Advance") {}

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

// ===== Game Gear =====

class GameGearConsoleContext : public ConsoleContext
{
public:
    GSL_SUPPRESS_F6 GameGearConsoleContext() noexcept : ConsoleContext(ConsoleID::GameGear, L"Game Gear") {}

    const std::vector<MemoryRegion>& MemoryRegions() const noexcept override { return m_vMemoryRegions; }

private:
    static const std::vector<MemoryRegion> m_vMemoryRegions;
};

// Game Gear memory is exposed as a single chunk
// http://www.smspower.org/Development/MemoryMap
const std::vector<ConsoleContext::MemoryRegion> GameGearConsoleContext::m_vMemoryRegions =
{
    { 0x000000U, 0x001FFFU, ConsoleContext::AddressType::SystemRAM, "System RAM" }, // RAM (normally $C000-$DFFF)
    // TODO: should cartridge memory be exposed ($0000-$BFFF)? it's usually just ROM data, but may contain on-cartridge RAM
};

// ===== Jaguar =====

class JaguarConsoleContext : public ConsoleContext
{
public:
    GSL_SUPPRESS_F6 JaguarConsoleContext() noexcept : ConsoleContext(ConsoleID::Jaguar, L"Jaguar") {}

    const std::vector<MemoryRegion>& MemoryRegions() const noexcept override { return m_vMemoryRegions; }

private:
    static const std::vector<MemoryRegion> m_vMemoryRegions;
};

// https://www.mulle-kybernetik.com/jagdox/memorymap.html
const std::vector<ConsoleContext::MemoryRegion> JaguarConsoleContext::m_vMemoryRegions =
{
    { 0x000000U, 0x1FFFFFU, ConsoleContext::AddressType::SystemRAM, "System RAM" },
};

// ===== Lynx =====

class LynxConsoleContext : public ConsoleContext
{
public:
    GSL_SUPPRESS_F6 LynxConsoleContext() noexcept : ConsoleContext(ConsoleID::Lynx, L"Lynx") {}

    const std::vector<MemoryRegion>& MemoryRegions() const noexcept override { return m_vMemoryRegions; }

private:
    static const std::vector<MemoryRegion> m_vMemoryRegions;
};

// http://www.retroisle.com/atari/lynx/Technical/Programming/lynxprgdumm.php
const std::vector<ConsoleContext::MemoryRegion> LynxConsoleContext::m_vMemoryRegions =
{
    { 0x000000U, 0x0000FFU, ConsoleContext::AddressType::SystemRAM, "Zero Page" },
    { 0x000100U, 0x0001FFU, ConsoleContext::AddressType::SystemRAM, "Stack" },
    { 0x000200U, 0x00FBFFU, ConsoleContext::AddressType::SystemRAM, "System RAM" },
    { 0x00FC00U, 0x00FCFFU, ConsoleContext::AddressType::HardwareController, "SUZY hardware access" },
    { 0x00FD00U, 0x00FDFFU, ConsoleContext::AddressType::HardwareController, "MIKEY hardware access" },
    { 0x00FE00U, 0x00FFF7U, ConsoleContext::AddressType::HardwareController, "Boot ROM" },
    { 0x00FFF8U, 0x00FFFFU, ConsoleContext::AddressType::HardwareController, "Hardware vectors" },
};

// ===== Master System =====

class MasterSystemConsoleContext : public ConsoleContext
{
public:
    GSL_SUPPRESS_F6 MasterSystemConsoleContext() noexcept : ConsoleContext(ConsoleID::MasterSystem, L"Master System") {}

    const std::vector<MemoryRegion>& MemoryRegions() const noexcept override { return m_vMemoryRegions; }

private:
    static const std::vector<MemoryRegion> m_vMemoryRegions;
};

// Master System memory is exposed as a single chunk
// http://www.smspower.org/Development/MemoryMap
const std::vector<ConsoleContext::MemoryRegion> MasterSystemConsoleContext::m_vMemoryRegions =
{
    { 0x000000U, 0x001FFFU, ConsoleContext::AddressType::SystemRAM, "System RAM" }, // RAM (normally $C000-$DFFF)
    // TODO: should cartridge memory be exposed ($0000-$BFFF)? it's usually just ROM data, but may contain on-cartridge RAM
};

// ===== MegaDrive / Genesis | Sega 32X | Sega CD =====

class MegaDriveConsoleContext : public ConsoleContext
{
public:
    MegaDriveConsoleContext(ConsoleID nId, std::wstring&& sName) noexcept : ConsoleContext(nId, std::move(sName)) {}

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
    GSL_SUPPRESS_F6 Nintendo64ConsoleContext() noexcept : ConsoleContext(ConsoleID::N64, L"Nintendo 64") {}

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

// ===== Nintendo DS =====

class NintendoDSConsoleContext : public ConsoleContext
{
public:
    GSL_SUPPRESS_F6 NintendoDSConsoleContext() noexcept : ConsoleContext(ConsoleID::DS, L"Nintendo DS") {}

    const std::vector<MemoryRegion>& MemoryRegions() const noexcept override { return m_vMemoryRegions; }

private:
    static const std::vector<MemoryRegion> m_vMemoryRegions;
};

// https://www.akkit.org/info/gbatek.htm#dsmemorymaps
const std::vector<ConsoleContext::MemoryRegion> NintendoDSConsoleContext::m_vMemoryRegions =
{
    { 0x000000U, 0x3FFFFFU, ConsoleContext::AddressType::SystemRAM, "System RAM" }, // Main Memory 4MB ($02000000-$023FFFFF)
};

// ===== NeoGeo Pocket =====

class NeoGeoPocketConsoleContext : public ConsoleContext
{
public:
    GSL_SUPPRESS_F6 NeoGeoPocketConsoleContext() noexcept : ConsoleContext(ConsoleID::NeoGeoPocket, L"NeoGeo Pocket") {}

    const std::vector<MemoryRegion>& MemoryRegions() const noexcept override { return m_vMemoryRegions; }

private:
    static const std::vector<MemoryRegion> m_vMemoryRegions;
};

// MednafenNGP only exposes 16KB of memory. This document suggest there should be 32KB:
// http://neopocott.emuunlim.com/docs/tech-11.txt
const std::vector<ConsoleContext::MemoryRegion> NeoGeoPocketConsoleContext::m_vMemoryRegions =
{
    { 0x000000U, 0x003FFFU, ConsoleContext::AddressType::SystemRAM, "System RAM" },
};

// ===== NES =====

class NintendoEntertainmentSystemConsoleContext : public ConsoleContext
{
public:
    GSL_SUPPRESS_F6 NintendoEntertainmentSystemConsoleContext() noexcept : ConsoleContext(ConsoleID::NES, L"Nintendo Entertainment System") {}

    const std::vector<MemoryRegion>& MemoryRegions() const noexcept override { return m_vMemoryRegions; }

private:
    static const std::vector<MemoryRegion> m_vMemoryRegions;
};

// NES memory is directly exposed to the toolkit
// https://wiki.nesdev.com/w/index.php/CPU_memory_map
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
    { 0x4020U, 0x5FFFU, ConsoleContext::AddressType::ReadOnlyMemory, "Cartridge data"}, // varies by mapper
    { 0x6000U, 0x7FFFU, ConsoleContext::AddressType::SaveRAM, "Cartridge RAM"},
    { 0x8000U, 0xFFFFU, ConsoleContext::AddressType::ReadOnlyMemory, "Cartridge ROM"},
};

// ===== Oric =====

class OricConsoleContext : public ConsoleContext
{
public:
    GSL_SUPPRESS_F6 OricConsoleContext() noexcept : ConsoleContext(ConsoleID::Oric, L"Oric") {}

    const std::vector<MemoryRegion>& MemoryRegions() const noexcept override { return m_vMemoryRegions; }

private:
    static const std::vector<MemoryRegion> m_vMemoryRegions;
};

const std::vector<ConsoleContext::MemoryRegion> OricConsoleContext::m_vMemoryRegions =
{
    { 0x000000U, 0x00FFFFU, ConsoleContext::AddressType::SystemRAM, "Main RAM" }, // actual size varies based on machine type, up to 64KB
};

// ===== PC-8800 =====

class PC8800ConsoleContext : public ConsoleContext
{
public:
    GSL_SUPPRESS_F6 PC8800ConsoleContext() noexcept : ConsoleContext(ConsoleID::PC8800, L"PC-8000/8800") {}

    const std::vector<MemoryRegion>& MemoryRegions() const noexcept override { return m_vMemoryRegions; }

private:
    static const std::vector<MemoryRegion> m_vMemoryRegions;
};

const std::vector<ConsoleContext::MemoryRegion> PC8800ConsoleContext::m_vMemoryRegions =
{
    { 0x000000U, 0x00FFFFU, ConsoleContext::AddressType::SystemRAM, "Main RAM" },
    { 0x010000U, 0x010FFFU, ConsoleContext::AddressType::VideoRAM, "Text VRAM" }, // technically VRAM, but often used as system RAM
};

// ===== PCEngine =====

class PCEngineConsoleContext : public ConsoleContext
{
public:
    GSL_SUPPRESS_F6 PCEngineConsoleContext() noexcept : ConsoleContext(ConsoleID::PCEngine, L"PCEngine / TurboGrafx 16") {}

    const std::vector<MemoryRegion>& MemoryRegions() const noexcept override { return m_vMemoryRegions; }

private:
    static const std::vector<MemoryRegion> m_vMemoryRegions;
};

// https://cc65.github.io/doc/pce.html
const std::vector<ConsoleContext::MemoryRegion> PCEngineConsoleContext::m_vMemoryRegions =
{
    { 0x000000U, 0x001FFFU, ConsoleContext::AddressType::SystemRAM, "System RAM" }, // normally $2000-$3FFF
};

// ===== PlayStation =====

class PlayStationConsoleContext : public ConsoleContext
{
public:
    GSL_SUPPRESS_F6 PlayStationConsoleContext() noexcept : ConsoleContext(ConsoleID::PlayStation, L"PlayStation") {}

    const std::vector<MemoryRegion>& MemoryRegions() const noexcept override { return m_vMemoryRegions; }

private:
    static const std::vector<MemoryRegion> m_vMemoryRegions;
};

// http://www.raphnet.net/electronique/psx_adaptor/Playstation.txt
const std::vector<ConsoleContext::MemoryRegion> PlayStationConsoleContext::m_vMemoryRegions =
{
    // Note that the primary executable filename appears at both $9E18 and $B8B0 (with full path).
    // Since the primary executable is named after the game's serial, it can be used for
    // distinguishing between titles when multiple versions are supported.
    { 0x000000U, 0x00FFFFU, ConsoleContext::AddressType::SystemRAM, "Kernel RAM" },
    { 0x010000U, 0x1FFFFFU, ConsoleContext::AddressType::SystemRAM, "System RAM" },
};

// ===== Pokemon Mini =====

class PokemonMiniConsoleContext : public ConsoleContext
{
public:
    GSL_SUPPRESS_F6 PokemonMiniConsoleContext() noexcept : ConsoleContext(ConsoleID::PokemonMini, L"Pokemon Mini") {}

    const std::vector<MemoryRegion>& MemoryRegions() const noexcept override { return m_vMemoryRegions; }

private:
    static const std::vector<MemoryRegion> m_vMemoryRegions;
};

// https://www.pokemon-mini.net/documentation/memory-map/
const std::vector<ConsoleContext::MemoryRegion> PokemonMiniConsoleContext::m_vMemoryRegions =
{
    { 0x000000U, 0x000FFFU, ConsoleContext::AddressType::SystemRAM, "BIOS RAM" },
    { 0x001000U, 0x001FFFU, ConsoleContext::AddressType::SystemRAM, "System RAM" },
};

// ===== Sega Saturn =====

class SaturnConsoleContext : public ConsoleContext
{
public:
    GSL_SUPPRESS_F6 SaturnConsoleContext() noexcept : ConsoleContext(ConsoleID::Saturn, L"Saturn") {}

    const std::vector<MemoryRegion>& MemoryRegions() const noexcept override { return m_vMemoryRegions; }

private:
    static const std::vector<MemoryRegion> m_vMemoryRegions;
};

// https://segaretro.org/Sega_Saturn_hardware_notes_(2004-04-27)
const std::vector<ConsoleContext::MemoryRegion> SaturnConsoleContext::m_vMemoryRegions =
{
    { 0x000000U, 0x0FFFFFU, ConsoleContext::AddressType::SystemRAM, "Work RAM Low" },  // RAM (normally $00200000-$002FFFFF)
    { 0x100000U, 0x1FFFFFU, ConsoleContext::AddressType::SystemRAM, "Work RAM High" }, // RAM (normally $06000000-$07FFFFFF, mirrored every megabyte)
};

// ===== SG-1000 =====

class SG1000ConsoleContext : public ConsoleContext
{
public:
    GSL_SUPPRESS_F6 SG1000ConsoleContext() noexcept : ConsoleContext(ConsoleID::SG1000, L"SG-1000") {}

    const std::vector<MemoryRegion>& MemoryRegions() const noexcept override { return m_vMemoryRegions; }

private:
    static const std::vector<MemoryRegion> m_vMemoryRegions;
};

// SG-1000 memory is exposed as a single chunk
// http://www.smspower.org/Development/MemoryMap
const std::vector<ConsoleContext::MemoryRegion> SG1000ConsoleContext::m_vMemoryRegions =
{
    { 0x000000U, 0x0003FFU, ConsoleContext::AddressType::SystemRAM, "System RAM" }, // RAM (normally $C000-$C3FF)
    // TODO: should cartridge memory be exposed ($0000-$BFFF)? it's usually just ROM data, but may contain on-cartridge RAM
    // This not is also concerning: http://www.smspower.org/Development/MemoryMap
    //   Cartridges may disable the system RAM and thus take over the full 64KB address space.
};

// ===== SNES =====

class SuperNESConsoleContext : public ConsoleContext
{
public:
    GSL_SUPPRESS_F6 SuperNESConsoleContext() noexcept : ConsoleContext(ConsoleID::SNES, L"Super NES") {}

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

// ==== WonderSwan ====

class WonderSwanConsoleContext : public ConsoleContext
{
public:
    GSL_SUPPRESS_F6 WonderSwanConsoleContext() noexcept : ConsoleContext(ConsoleID::WonderSwan, L"WonderSwan") {}

    const std::vector<MemoryRegion>& MemoryRegions() const noexcept override { return m_vMemoryRegions; }

private:
    static const std::vector<MemoryRegion> m_vMemoryRegions;
};

// WonderSwan memory is exposed as a single chunk
// http://daifukkat.su/docs/wsman/#ovr_memmap
const std::vector<ConsoleContext::MemoryRegion> WonderSwanConsoleContext::m_vMemoryRegions =
{
    { 0x000000U, 0x00FFFFU, ConsoleContext::AddressType::SystemRAM, "System RAM" }, // internal RAM ends at 0x003FFFU for WonderSwan, WonderSwan Color uses all of the exposed memory.
};

// ===== VirtualBoy =====

class VirtualBoyConsoleContext : public ConsoleContext
{
public:
    GSL_SUPPRESS_F6 VirtualBoyConsoleContext() noexcept : ConsoleContext(ConsoleID::VirtualBoy, L"VirtualBoy") {}

    const std::vector<MemoryRegion>& MemoryRegions() const noexcept override { return m_vMemoryRegions; }

private:
    static const std::vector<MemoryRegion> m_vMemoryRegions;
};

// VirtualBoy memory is exposed as two chunks
const std::vector<ConsoleContext::MemoryRegion> VirtualBoyConsoleContext::m_vMemoryRegions =
{
    { 0x000000U, 0x00FFFFU, ConsoleContext::AddressType::SystemRAM, "System RAM" }, // work RAM (normally $05000000-$0500FFFF)
    { 0x010000U, 0x01FFFFU, ConsoleContext::AddressType::SaveRAM, "Cartridge RAM" }, // cartridge RAM (normally $06000000-$06FFFFFF); up to 16MB, typically 64KB
};

// ===== ConsoleContext =====

const ConsoleContext::MemoryRegion* ConsoleContext::GetMemoryRegion(ra::ByteAddress nAddress) const
{
    const auto& vRegions = MemoryRegions();
    gsl::index nStart = 0;
    gsl::index nEnd = vRegions.size() - 1;
    while (nStart <= nEnd)
    {
        const gsl::index nMid = (nStart + nEnd) / 2;
        const auto& pRegion = vRegions.at(nMid);
        if (pRegion.StartAddress > nAddress)
            nEnd = nMid - 1;
        else if (pRegion.EndAddress < nAddress)
            nStart = nMid + 1;
        else
            return &pRegion;
    }

    return nullptr;
}


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
            return std::make_unique<AppleIIConsoleContext>();

        case ConsoleID::Arcade:
            return std::make_unique<ConsoleContext>(nId, L"Arcade");

        case ConsoleID::Atari2600:
            return std::make_unique<Atari2600ConsoleContext>();

        case ConsoleID::Atari5200:
            return std::make_unique<ConsoleContext>(nId, L"Atari 5200");

        case ConsoleID::Atari7800:
            return std::make_unique<Atari7800ConsoleContext>();

        case ConsoleID::C64:
            return std::make_unique<ConsoleContext>(nId, L"Commodore 64");

        case ConsoleID::CassetteVision:
            return std::make_unique<ConsoleContext>(nId, L"Cassette Vision");

        case ConsoleID::CDi:
            return std::make_unique<ConsoleContext>(nId, L"CD-I");

        case ConsoleID::Colecovision:
            return std::make_unique<ColecoVisionConsoleContext>();

        case ConsoleID::Dreamcast:
            return std::make_unique<ConsoleContext>(nId, L"Dreamcast");

        case ConsoleID::DS:
            return std::make_unique<NintendoDSConsoleContext>();

        case ConsoleID::Events:
            return std::make_unique<ConsoleContext>(nId, L"Events");

        case ConsoleID::GameCube:
            return std::make_unique<ConsoleContext>(nId, L"GameCube");

        case ConsoleID::GameGear:
            return std::make_unique<GameGearConsoleContext>();

        case ConsoleID::GB:
            return std::make_unique<GameBoyConsoleContext>(ConsoleID::GB, L"GameBoy");

        case ConsoleID::GBA:
            return std::make_unique<GameBoyAdvanceConsoleContext>();

        case ConsoleID::GBC:
            return std::make_unique<GameBoyConsoleContext>(ConsoleID::GBC, L"GameBoy Color");

        case ConsoleID::Intellivision:
            return std::make_unique<ConsoleContext>(nId, L"Intellivision");

        case ConsoleID::Jaguar:
            return std::make_unique<JaguarConsoleContext>();

        case ConsoleID::Lynx:
            return std::make_unique<LynxConsoleContext>();

        case ConsoleID::MasterSystem:
            return std::make_unique<MasterSystemConsoleContext>();

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

        case ConsoleID::Oric:
            return std::make_unique<OricConsoleContext>();

        case ConsoleID::PC8800:
            return std::make_unique<PC8800ConsoleContext>();

        case ConsoleID::PC9800:
            return std::make_unique<ConsoleContext>(nId, L"PC-9800");

        case ConsoleID::PCEngine:
            return std::make_unique<ConsoleContext>(nId, L"PCEngine / TurboGrafx 16");

        case ConsoleID::PCFX:
            return std::make_unique<ConsoleContext>(nId, L"PCFX");

        case ConsoleID::PlayStation:
            return std::make_unique<PlayStationConsoleContext>();

        case ConsoleID::PlayStation2:
            return std::make_unique<ConsoleContext>(nId, L"PlayStation 2");

        case ConsoleID::PokemonMini:
            return std::make_unique<PokemonMiniConsoleContext>();

        case ConsoleID::PSP:
            return std::make_unique<ConsoleContext>(nId, L"PlayStation Portable");

        case ConsoleID::Saturn:
            return std::make_unique<SaturnConsoleContext>();

        case ConsoleID::Sega32X:
            return std::make_unique<MegaDriveConsoleContext>(ConsoleID::Sega32X, L"SEGA 32X");

        case ConsoleID::SegaCD:
            return std::make_unique<MegaDriveConsoleContext>(ConsoleID::SegaCD, L"SEGA CD");

        case ConsoleID::SG1000:
            return std::make_unique<SG1000ConsoleContext>();

        case ConsoleID::SNES:
            return std::make_unique<SuperNESConsoleContext>();

        case ConsoleID::SuperCassetteVision:
            return std::make_unique<ConsoleContext>(nId, L"Super Cassette Vision");

        case ConsoleID::ThreeDO:
            return std::make_unique<ConsoleContext>(nId, L"3D0");

        case ConsoleID::Vectrex:
            return std::make_unique<ConsoleContext>(nId, L"Vectrex");

        case ConsoleID::VIC20:
            return std::make_unique<ConsoleContext>(nId, L"VIC-20");

        case ConsoleID::VirtualBoy:
            return std::make_unique<VirtualBoyConsoleContext>();

        case ConsoleID::WII:
            return std::make_unique<ConsoleContext>(nId, L"Wii");

        case ConsoleID::WIIU:
            return std::make_unique<ConsoleContext>(nId, L"Wii-U");

        case ConsoleID::WonderSwan:
            return std::make_unique<WonderSwanConsoleContext>();

        case ConsoleID::X68K:
            return std::make_unique<ConsoleContext>(nId, L"X68K");

        case ConsoleID::Xbox:
            return std::make_unique<ConsoleContext>(nId, L"XBOX");

        case ConsoleID::ZX81:
            return std::make_unique<ConsoleContext>(nId, L"ZX-81");

        default:
            return std::make_unique<ConsoleContext>(ConsoleID::UnknownConsoleID, L"Unknown");
    }
}

} // namespace data
} // namespace ra
