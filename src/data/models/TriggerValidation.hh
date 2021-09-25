#ifndef RA_DATA_TRIGGER_VALIDATION_H
#define RA_DATA_TRIGGER_VALIDATION_H
#pragma once

#include "data\Types.hh"

namespace ra {
namespace data {
namespace models {

class TriggerValidation
{
public:
    static bool Validate(const std::string& sTrigger, std::wstring& sError);
    static MemSize MapRcheevosMemSize(char nSize) noexcept;
};

} // namespace models
} // namespace data
} // namespace ra

#endif RA_DATA_TRIGGER_VALIDATION_H
