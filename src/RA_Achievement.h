#ifndef RA_ACHIEVEMENT_H
#define RA_ACHIEVEMENT_H
#pragma once

#include "RA_Condition.h"
#include "ra_utility.h"

#include <memory>

//////////////////////////////////////////////////////////////////////////
// Achievement
//////////////////////////////////////////////////////////////////////////

class Achievement
{
public:
    enum class DirtyFlags
    {
        Clean,
        Title = 1 << 0,
        Desc = 1 << 1,
        Points = 1 << 2,
        Author = 1 << 3,
        ID = 1 << 4,
        Badge = 1 << 5,
        Conditions = 1 << 6,
        Votes = 1 << 7,
        Description = 1 << 8,

        All = std::numeric_limits<std::underlying_type_t<DirtyFlags>>::max()
    };

    Achievement() noexcept;
    GSL_SUPPRESS_F6 ~Achievement() noexcept;
    Achievement(const Achievement&) noexcept = default;
    Achievement& operator=(const Achievement&) noexcept = default;
    Achievement(Achievement&&) noexcept = default;
    Achievement& operator=(Achievement&&) noexcept = default;

public:
    size_t AddCondition(size_t nConditionGroup, const Condition& pNewCond);
    size_t InsertCondition(size_t nConditionGroup, size_t nIndex, const Condition& pNewCond);
    BOOL RemoveCondition(size_t nConditionGroup, unsigned int nConditionID);
    void RemoveAllConditions(size_t nConditionGroup);

    void CopyFrom(const Achievement& rRHS);

    inline BOOL Active() const noexcept { return m_bActive; }
    GSL_SUPPRESS_F6 void SetActive(BOOL bActive) noexcept;

    inline BOOL Modified() const noexcept { return m_bModified; }
    void SetModified(BOOL bModified) noexcept;

    inline BOOL GetPauseOnTrigger() const noexcept { return m_bPauseOnTrigger; }
    void SetPauseOnTrigger(BOOL bPause) noexcept { m_bPauseOnTrigger = bPause; }

    inline BOOL GetPauseOnReset() const noexcept { return m_bPauseOnReset; }
    void SetPauseOnReset(BOOL bPause);

    void SetID(ra::AchievementID nID) noexcept;
    inline ra::AchievementID ID() const noexcept { return m_nAchievementID; }

    void SetCategory(int nID) noexcept { m_nCategoryID = nID; }
    inline int Category() const noexcept { return m_nCategoryID; }

    inline const std::string& Title() const noexcept { return m_sTitle; }
    void SetTitle(const std::string& sTitle) { m_sTitle = sTitle; }
    inline const std::string& Description() const noexcept { return m_sDescription; }
    void SetDescription(const std::string& sDescription) { m_sDescription = sDescription; }
    inline const std::string& Author() const noexcept { return m_sAuthor; }
    void SetAuthor(const std::string& sAuthor) { m_sAuthor = sAuthor; }
    inline unsigned int Points() const noexcept { return m_nPointValue; }
    void SetPoints(unsigned int nPoints) noexcept { m_nPointValue = nPoints; }

    inline time_t CreatedDate() const noexcept { return m_nTimestampCreated; }
    void SetCreatedDate(time_t nTimeCreated) noexcept { m_nTimestampCreated = nTimeCreated; }
    inline time_t ModifiedDate() const noexcept { return m_nTimestampModified; }
    void SetModifiedDate(time_t nTimeModified) noexcept { m_nTimestampModified = nTimeModified; }

    inline unsigned short Upvotes() const noexcept { return m_nUpvotes; }
    inline unsigned short Downvotes() const noexcept { return m_nDownvotes; }

    inline const std::string& Progress() const noexcept { return m_sProgress; }
    void SetProgressIndicator(const std::string& sProgress) { m_sProgress = sProgress; }
    inline const std::string& ProgressMax() const noexcept { return m_sProgressMax; }
    void SetProgressIndicatorMax(const std::string& sProgressMax) { m_sProgressMax = sProgressMax; }
    inline const std::string& ProgressFmt() const noexcept { return m_sProgressFmt; }
    void SetProgressIndicatorFormat(const std::string& sProgressFmt) { m_sProgressFmt = sProgressFmt; }

    void AddConditionGroup() noexcept;
    void RemoveConditionGroup();

    inline size_t NumConditionGroups() const noexcept { return m_vConditions.GroupCount(); }
    inline size_t NumConditions(size_t nGroup) const
    {
        return nGroup < m_vConditions.GroupCount() ? m_vConditions.GetGroup(nGroup).Count() : 0;
    }

    inline const std::string& BadgeImageURI() const noexcept { return m_sBadgeImageURI; }
    void SetBadgeImage(const std::string& sFilename);

    Condition& GetCondition(size_t nCondGroup, size_t i) { return m_vConditions.GetGroup(nCondGroup).GetAt(i); }
    unsigned int GetConditionHitCount(size_t nCondGroup, size_t i) const noexcept;
    void SetConditionHitCount(size_t nCondGroup, size_t i, unsigned int nHitCount) const noexcept;

    std::string CreateMemString() const;

    void Reset() noexcept;

    void ParseTrigger(const char* pTrigger);

    // Used for rendering updates when editing achievements. Usually always false.
    _NODISCARD _CONSTANT_FN GetDirtyFlags() const noexcept { return m_nDirtyFlags; }
    _NODISCARD _CONSTANT_FN IsDirty() const noexcept { return (m_nDirtyFlags != DirtyFlags()); }

    _CONSTANT_FN SetDirtyFlag(_In_ DirtyFlags nFlags) noexcept
    {
        using namespace ra::bitwise_ops;
        m_nDirtyFlags |= nFlags;
    }

    _CONSTANT_FN ClearDirtyFlag() noexcept { m_nDirtyFlags = DirtyFlags{}; }

    void RebuildTrigger();

    // range for
    _NODISCARD inline auto begin() noexcept { return m_vConditions.begin(); }
    _NODISCARD inline auto begin() const noexcept { return m_vConditions.begin(); }
    _NODISCARD inline auto end() noexcept { return m_vConditions.end(); }
    _NODISCARD inline auto end() const noexcept { return m_vConditions.end(); }

protected:
    void* m_pTrigger = nullptr;                                   // rc_trigger_t
    std::shared_ptr<std::vector<unsigned char>> m_pTriggerBuffer; // buffer for rc_trigger_t

private:
    ra::AchievementID m_nAchievementID{};
    int m_nCategoryID{};

    ConditionSet m_vConditions; //  UI wrappers for trigger

    std::string m_sTitle;
    std::string m_sDescription;
    std::string m_sAuthor;
    std::string m_sBadgeImageURI;

    unsigned int m_nPointValue{};
    BOOL m_bActive{};
    BOOL m_bModified{};
    BOOL m_bPauseOnTrigger{};
    BOOL m_bPauseOnReset{};

    // Progress:
    BOOL m_bProgressEnabled{}; // on/off

    std::string m_sProgress;    // How to calculate the progress so far (syntactical)
    std::string m_sProgressMax; // Upper limit of the progress (syntactical? value?)
    std::string m_sProgressFmt; // Format of the progress to be shown (currency? step?)

    float m_fProgressLastShown{}; // The last shown progress

    DirtyFlags m_nDirtyFlags{}; // Use for rendering when editing.

    time_t m_nTimestampCreated{};
    time_t m_nTimestampModified{};

    unsigned short m_nUpvotes{};
    unsigned short m_nDownvotes{};
};

#endif // !RA_ACHIEVEMENT_H
