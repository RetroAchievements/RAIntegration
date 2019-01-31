#include "ConsoleContext.hh"

#include "RA_Log.h"
#include "RA_StringUtils.h"

namespace ra {
namespace data {

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
            return std::make_unique<ConsoleContext>(nId, L"GameBoy");

        case ConsoleID::GBA:
            return std::make_unique<ConsoleContext>(nId, L"GameBoy Advance");

        case ConsoleID::GBC:
            return std::make_unique<ConsoleContext>(nId, L"GameBoy Color");

        case ConsoleID::Intellivision:
            return std::make_unique<ConsoleContext>(nId, L"Intellivision");

        case ConsoleID::Jaguar:
            return std::make_unique<ConsoleContext>(nId, L"Jaguar");

        case ConsoleID::Lynx:
            return std::make_unique<ConsoleContext>(nId, L"Lynx");

        case ConsoleID::MasterSystem:
            return std::make_unique<ConsoleContext>(nId, L"Master System");

        case ConsoleID::MegaDrive:
            return std::make_unique<ConsoleContext>(nId, L"MegaDrive / Genesis");

        case ConsoleID::MSDOS:
            return std::make_unique<ConsoleContext>(nId, L"MS-DOS");

        case ConsoleID::MSX:
            return std::make_unique<ConsoleContext>(nId, L"MSX");

        case ConsoleID::N64:
            return std::make_unique<ConsoleContext>(nId, L"Nintendo 64");

        case ConsoleID::NeoGeoPocket:
            return std::make_unique<ConsoleContext>(nId, L"NeoGeo Pocket");

        case ConsoleID::NES:
            return std::make_unique<ConsoleContext>(nId, L"Nintendo Entertainment System");

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

        default:
            return std::make_unique<ConsoleContext>(ConsoleID::UnknownConsoleID, L"Unknown");
    }
}

} // namespace data
} // namespace ra
