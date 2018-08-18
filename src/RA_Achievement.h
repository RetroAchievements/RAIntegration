#ifndef RA_ACHIEVEMENT_H
#define RA_ACHIEVEMENT_H
#pragma once

#include "RA_Condition.h"

// just in-case
#ifndef _ARRAY_
#include <array>
#endif // !_ARRAY_

//////////////////////////////////////////////////////////////////////////
//	Achievement
//////////////////////////////////////////////////////////////////////////

namespace ra {

// TBD: either use the max value of a signed int for Achievement_DirtyFlags::All or make it's underlying_type
//      a size_type (such as std::size_t, std::uintptr_t).
enum class Achievement_DirtyFlags : std::size_t
{
    NotDirty    = 0,
    Title       = 1 << 0,
    Desc        = 1 << 1,
    Points      = 1 << 2,
    Author      = 1 << 3,
    ID          = 1 << 4,
    Badge       = 1 << 5,
    Conditions  = 1 << 6,
    Votes       = 1 << 7,
    Description = 1 << 8,

    All = to_unsigned(-1)
};

} // namespace ra

class Achievement
{
public:
    explicit Achievement(_In_ ra::AchievementSetType nType) noexcept;

public:
    void Clear() noexcept;
    BOOL Test();

    size_t AddCondition(size_t nConditionGroup, const Condition& pNewCond);
    size_t InsertCondition(size_t nConditionGroup, size_t nIndex, const Condition& pNewCond);
    BOOL RemoveCondition(size_t nConditionGroup, unsigned int nConditionID);
    void RemoveAllConditions(size_t nConditionGroup);

    void Set(const Achievement& rRHS);

    inline BOOL Active() const { return m_bActive; }
    void SetActive(BOOL bActive);

    inline BOOL Modified() const { return m_bModified; }
    void SetModified(BOOL bModified);

    inline BOOL GetPauseOnTrigger() const { return m_bPauseOnTrigger; }
    void SetPauseOnTrigger(BOOL bPause) { m_bPauseOnTrigger = bPause; }

    inline BOOL GetPauseOnReset() const { return m_bPauseOnReset; }
    void SetPauseOnReset(BOOL bPause) { m_bPauseOnReset = bPause; }

    _NODISCARD _CONSTANT_FN IsCoreAchievement() const noexcept { return (m_nSetType == ra::AchievementSetType::Core); }

    void SetID(ra::AchievementID nID);
    _NODISCARD _CONSTANT_FN ID() const noexcept { return m_nAchievementID; }

    inline const std::string& Title() const { return m_sTitle; }
    void SetTitle(const std::string& sTitle) { m_sTitle = sTitle; }
    inline const std::string& Description() const { return m_sDescription; }
    void SetDescription(const std::string& sDescription) { m_sDescription = sDescription; }
    inline const std::string& Author() const { return m_sAuthor; }
    void SetAuthor(const std::string& sAuthor) { m_sAuthor = sAuthor; }
    inline unsigned int Points() const { return m_nPointValue; }
    void SetPoints(unsigned int nPoints) { m_nPointValue = nPoints; }

    inline time_t CreatedDate() const { return m_nTimestampCreated; }
    void SetCreatedDate(time_t nTimeCreated) { m_nTimestampCreated = nTimeCreated; }
    inline time_t ModifiedDate() const { return m_nTimestampModified; }
    void SetModifiedDate(time_t nTimeModified) { m_nTimestampModified = nTimeModified; }

    inline unsigned short Upvotes() const { return m_nUpvotes; }
    inline unsigned short Downvotes() const { return m_nDownvotes; }

    inline const std::string& Progress() const { return m_sProgress; }
    void SetProgressIndicator(const std::string& sProgress) { m_sProgress = sProgress; }
    inline const std::string& ProgressMax() const { return m_sProgressMax; }
    void SetProgressIndicatorMax(const std::string& sProgressMax) { m_sProgressMax = sProgressMax; }
    inline const std::string& ProgressFmt() const { return m_sProgressFmt; }
    void SetProgressIndicatorFormat(const std::string& sProgressFmt) { m_sProgressFmt = sProgressFmt; }


    void AddConditionGroup();
    void RemoveConditionGroup();

    inline size_t NumConditionGroups() const { return m_vConditions.GroupCount(); }
    inline size_t NumConditions(size_t nGroup) const { return nGroup < m_vConditions.GroupCount() ? m_vConditions.GetGroup(nGroup).Count() : 0; }

    inline HBITMAP BadgeImage() const { return m_hBadgeImage; }
    inline HBITMAP BadgeImageLocked() const { return m_hBadgeImageLocked; }
    inline const std::string& BadgeImageURI() const { return m_sBadgeImageURI; }

    void SetBadgeImage(const std::string& sFilename);
    void ClearBadgeImage() noexcept;

    Condition& GetCondition(size_t nCondGroup, size_t i) { return m_vConditions.GetGroup(nCondGroup).GetAt(i); }

    std::string CreateMemString() const;

    void Reset();

    //	Returns the new char* offset after parsing.
    const char* ParseLine(const char* buffer);

    //	Parse from json element
    void Parse(const Value& element);

    //	Used for rendering updates when editing achievements. Usually always false.
    _CONSTANT_FN GetDirtyFlags() const { return m_nDirtyFlags; }
    _CONSTANT_FN IsDirty() const noexcept { return (m_nDirtyFlags != ra::Achievement_DirtyFlags{}); }
    _CONSTANT_FN SetDirtyFlag(_In_ ra::Achievement_DirtyFlags nFlags) noexcept
    {
        using namespace ra::bitwise_ops;
        const auto _{ m_nDirtyFlags |= nFlags };
    }
    _CONSTANT_FN ClearDirtyFlag() noexcept { m_nDirtyFlags = ra::Achievement_DirtyFlags{}; }

private:
    /*const*/ ra::AchievementSetType m_nSetType;

    ra::AchievementID m_nAchievementID;
    ConditionSet m_vConditions;

    std::string m_sTitle;
    std::string m_sDescription;
    std::string m_sAuthor;
    std::string m_sBadgeImageURI;

    unsigned int m_nPointValue;
    BOOL m_bActive;
    BOOL m_bModified;
    BOOL m_bPauseOnTrigger;
    BOOL m_bPauseOnReset;

    //	Progress:
    BOOL m_bProgressEnabled;	//	on/off

    std::string m_sProgress;	//	How to calculate the progress so far (syntactical)
    std::string m_sProgressMax;	//	Upper limit of the progress (syntactical? value?)
    std::string m_sProgressFmt;	//	Format of the progress to be shown (currency? step?)

    float m_fProgressLastShown;	//	The last shown progress

    ra::Achievement_DirtyFlags m_nDirtyFlags = ra::Achievement_DirtyFlags{}; // Use for rendering when editing.

    time_t m_nTimestampCreated;
    time_t m_nTimestampModified;

    unsigned short m_nUpvotes;
    unsigned short m_nDownvotes;

    HBITMAP m_hBadgeImage;
    HBITMAP m_hBadgeImageLocked;
};



#endif // !RA_ACHIEVEMENT_H
