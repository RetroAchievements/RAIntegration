#include "AchievementSetModel.hh"

#include "context/IRcClient.hh"

#include "util/Strings.hh"

#include <rcheevos/src/rc_client_internal.h>

namespace ra {
namespace data {
namespace models {

const IntModelProperty AchievementSetModel::TypeProperty("AchievementSetModel", "Type", ra::etoi(AchievementSetType::Core));
const IntModelProperty AchievementSetModel::IDProperty("AchievementSetModel", "ID", 0);
const IntModelProperty AchievementSetModel::BackingGameIDProperty("AchievementSetModel", "BackingGameID", 0);
const StringModelProperty AchievementSetModel::TitleProperty("AchievementSetModel", "Name", L"");

void AchievementSetModel::Initialize(uint32_t nId, uint32_t nBackingGameId, const std::wstring& sName, AchievementSetType nType)
{
    SetValue(IDProperty, nId);
    SetValue(BackingGameIDProperty, nBackingGameId);
    SetValue(TitleProperty, sName);
    SetValue(TypeProperty, nType);
}


} // namespace models
} // namespace data
} // namespace ra
