#ifndef RA_DATA_UTIL_TRIGGERVALIDATION_H
#define RA_DATA_UTIL_TRIGGERVALIDATION_H
#pragma once

#include "data/models/AssetModelBase.hh"

namespace ra {
namespace data {
namespace util {

class TriggerValidation
{
public:
    static bool Validate(const std::string& sTrigger, std::wstring& sError, ra::data::models::AssetType nType);
};

} // namespace util
} // namespace data
} // namespace ra

#endif RA_DATA_UTIL_TRIGGERVALIDATION_H
