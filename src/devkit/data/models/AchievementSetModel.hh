#ifndef RA_DATA_MODELS_ACHIEVEMENTSETMODEL_H
#define RA_DATA_MODELS_ACHIEVEMENTSETMODEL_H
#pragma once

#include "data/DataModelBase.hh"

#include "util/TypeCasts.hh"

namespace ra {
namespace data {
namespace models {

enum AchievementSetType
{
    Core,
    Bonus,
    Specialty,
    Exclusive,
};

class AchievementSetModel : public DataModelBase
{
public:
    AchievementSetModel() noexcept = default;
	~AchievementSetModel() = default;
    AchievementSetModel(const AchievementSetModel&) noexcept = delete;
    AchievementSetModel& operator=(const AchievementSetModel&) noexcept = delete;
    AchievementSetModel(AchievementSetModel&&) noexcept = delete;
    AchievementSetModel& operator=(AchievementSetModel&&) noexcept = delete;

    /// <summary>
    /// A unique value for local achievement sets.
    /// </summary>
    static constexpr uint32_t LocalId = 0xFFFFFFFF;

    /// <summary>
    /// Initializes the achievement set.
    /// </summary>
    /// <param name="nId"></param>
    /// <param name="nBackingGameId"></param>
    /// <param name="sName"></param>
    /// <param name="nType"></param>
    void Initialize(uint32_t nId, uint32_t nBackingGameId, const std::wstring& sName, AchievementSetType nType);

    /// <summary>
    /// The <see cref="ModelProperty" /> for the achievement set type.
    /// </summary>
    static const IntModelProperty TypeProperty;

    /// <summary>
    /// Gets the achievement set type.
    /// </summary>
    AchievementSetType GetType() const { return ra::itoe<AchievementSetType>(GetValue(TypeProperty)); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the achievement set id.
    /// </summary>
    static const IntModelProperty IDProperty;

    /// <summary>
    /// Gets the achievement set id.
    /// </summary>
    uint32_t GetID() const { return ra::to_unsigned(GetValue(IDProperty)); }

    /// <summary>
    /// Sets the achievement set id.
    /// </summary>
    void SetID(uint32_t nValue) { SetValue(IDProperty, nValue); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the backing game id.
    /// </summary>
    static const IntModelProperty BackingGameIDProperty;

    /// <summary>
    /// Gets the backing game id.
    /// </summary>
    uint32_t GetBackingGameID() const { return ra::to_unsigned(GetValue(BackingGameIDProperty)); }

    /// <summary>
    /// Sets the backing game id.
    /// </summary>
    void SetBackingGameID(uint32_t nValue) { SetValue(BackingGameIDProperty, nValue); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the title.
    /// </summary>
    static const StringModelProperty TitleProperty;

    /// <summary>
    /// Gets the title to display.
    /// </summary>
    const std::wstring& GetTitle() const { return GetValue(TitleProperty); }

    /// <summary>
    /// Sets the title to display.
    /// </summary>
    void SetTitle(const std::wstring& sValue) { SetValue(TitleProperty, sValue); }
};

} // namespace models
} // namespace data
} // namespace ra

#endif RA_DATA_MODELS_ACHIEVEMENTSETMODEL_H
