#include "TriggerViewModel.hh"

#include <rcheevos.h>
#include <rcheevos/src/rcheevos/rc_internal.h>
#include <rcheevos/src/rc_client_internal.h>

#include "RA_StringUtils.h"

#include "data\models\TriggerValidation.hh"

#include "services\AchievementLogicSerializer.hh"
#include "services\AchievementRuntime.hh"
#include "services\IClipboard.hh"
#include "services\ServiceLocator.hh"

#include "ui\EditorTheme.hh"
#include "ui\viewmodels\MessageBoxViewModel.hh"
#include "ui\viewmodels\WindowManager.hh"

namespace ra {
namespace ui {
namespace viewmodels {

const IntModelProperty TriggerViewModel::SelectedGroupIndexProperty("TriggerViewModel", "SelectedGroupIndex", -1);
const IntModelProperty TriggerViewModel::VersionProperty("TriggerViewModel", "Version", 0);
const IntModelProperty TriggerViewModel::EnsureVisibleGroupIndexProperty("TriggerViewModel", "EnsureVisibleGroupIndex", -1);
const BoolModelProperty TriggerViewModel::IsMeasuredTrackedAsPercentProperty("TriggerViewModel", "IsMeasuredTrackedAsPercent", false);
const IntModelProperty TriggerViewModel::ScrollOffsetProperty("TriggerViewModel", "ScrollOffset", 0);
const IntModelProperty TriggerViewModel::ScrollMaximumProperty("TriggerViewModel", "ScrollMaximum", 0);
const IntModelProperty TriggerViewModel::VisibleItemCountProperty("TriggerViewModel", "VisibleItemCount", 8);
const IntModelProperty TriggerViewModel::GroupViewModel::ColorProperty("GroupViewModel", "Color", 0);

TriggerViewModel::TriggerViewModel() noexcept
    : m_pConditionsMonitor(*this)
{
    m_vConditionTypes.Add(ra::etoi(TriggerConditionType::Standard), L"");
    m_vConditionTypes.Add(ra::etoi(TriggerConditionType::PauseIf), L"Pause If");
    m_vConditionTypes.Add(ra::etoi(TriggerConditionType::ResetIf), L"Reset If");
    m_vConditionTypes.Add(ra::etoi(TriggerConditionType::ResetNextIf), L"Reset Next If");
    m_vConditionTypes.Add(ra::etoi(TriggerConditionType::AddSource), L"Add Source");
    m_vConditionTypes.Add(ra::etoi(TriggerConditionType::SubSource), L"Sub Source");
    m_vConditionTypes.Add(ra::etoi(TriggerConditionType::AddHits), L"Add Hits");
    m_vConditionTypes.Add(ra::etoi(TriggerConditionType::SubHits), L"Sub Hits");
    m_vConditionTypes.Add(ra::etoi(TriggerConditionType::AddAddress), L"Add Address");
    m_vConditionTypes.Add(ra::etoi(TriggerConditionType::AndNext), L"And Next");
    m_vConditionTypes.Add(ra::etoi(TriggerConditionType::OrNext), L"Or Next");
    m_vConditionTypes.Add(ra::etoi(TriggerConditionType::Measured), L"Measured");
    m_vConditionTypes.Add(ra::etoi(TriggerConditionType::MeasuredIf), L"Measured If");
    m_vConditionTypes.Add(ra::etoi(TriggerConditionType::Trigger), L"Trigger");
    m_vConditionTypes.Add(ra::etoi(TriggerConditionType::Remember), L"Remember");

    m_vOperandTypes.Add(ra::etoi(TriggerOperandType::Address), L"Mem");
    m_vOperandTypes.Add(ra::etoi(TriggerOperandType::Value), L"Value");
    m_vOperandTypes.Add(ra::etoi(TriggerOperandType::Delta), L"Delta");
    m_vOperandTypes.Add(ra::etoi(TriggerOperandType::Prior), L"Prior");
    m_vOperandTypes.Add(ra::etoi(TriggerOperandType::BCD), L"BCD");
    m_vOperandTypes.Add(ra::etoi(TriggerOperandType::Float), L"Float");
    m_vOperandTypes.Add(ra::etoi(TriggerOperandType::Inverted), L"Invert");
    m_vOperandTypes.Add(ra::etoi(TriggerOperandType::Recall), L"Recall");

    m_vOperandSizes.Add(ra::etoi(MemSize::Bit_0), ra::data::MemSizeString(MemSize::Bit_0));
    m_vOperandSizes.Add(ra::etoi(MemSize::Bit_1), ra::data::MemSizeString(MemSize::Bit_1));
    m_vOperandSizes.Add(ra::etoi(MemSize::Bit_2), ra::data::MemSizeString(MemSize::Bit_2));
    m_vOperandSizes.Add(ra::etoi(MemSize::Bit_3), ra::data::MemSizeString(MemSize::Bit_3));
    m_vOperandSizes.Add(ra::etoi(MemSize::Bit_4), ra::data::MemSizeString(MemSize::Bit_4));
    m_vOperandSizes.Add(ra::etoi(MemSize::Bit_5), ra::data::MemSizeString(MemSize::Bit_5));
    m_vOperandSizes.Add(ra::etoi(MemSize::Bit_6), ra::data::MemSizeString(MemSize::Bit_6));
    m_vOperandSizes.Add(ra::etoi(MemSize::Bit_7), ra::data::MemSizeString(MemSize::Bit_7));
    m_vOperandSizes.Add(ra::etoi(MemSize::Nibble_Lower), ra::data::MemSizeString(MemSize::Nibble_Lower));
    m_vOperandSizes.Add(ra::etoi(MemSize::Nibble_Upper), ra::data::MemSizeString(MemSize::Nibble_Upper));
    m_vOperandSizes.Add(ra::etoi(MemSize::EightBit), ra::data::MemSizeString(MemSize::EightBit));
    m_vOperandSizes.Add(ra::etoi(MemSize::SixteenBit), ra::data::MemSizeString(MemSize::SixteenBit));
    m_vOperandSizes.Add(ra::etoi(MemSize::TwentyFourBit), ra::data::MemSizeString(MemSize::TwentyFourBit));
    m_vOperandSizes.Add(ra::etoi(MemSize::ThirtyTwoBit), ra::data::MemSizeString(MemSize::ThirtyTwoBit));
    m_vOperandSizes.Add(ra::etoi(MemSize::SixteenBitBigEndian), ra::data::MemSizeString(MemSize::SixteenBitBigEndian));
    m_vOperandSizes.Add(ra::etoi(MemSize::TwentyFourBitBigEndian), ra::data::MemSizeString(MemSize::TwentyFourBitBigEndian));
    m_vOperandSizes.Add(ra::etoi(MemSize::ThirtyTwoBitBigEndian), ra::data::MemSizeString(MemSize::ThirtyTwoBitBigEndian));
    m_vOperandSizes.Add(ra::etoi(MemSize::BitCount), ra::data::MemSizeString(MemSize::BitCount));
    m_vOperandSizes.Add(ra::etoi(MemSize::Float), ra::data::MemSizeString(MemSize::Float));
    m_vOperandSizes.Add(ra::etoi(MemSize::FloatBigEndian), ra::data::MemSizeString(MemSize::FloatBigEndian));
    m_vOperandSizes.Add(ra::etoi(MemSize::Double32), ra::data::MemSizeString(MemSize::Double32));
    m_vOperandSizes.Add(ra::etoi(MemSize::Double32BigEndian), ra::data::MemSizeString(MemSize::Double32BigEndian));
    m_vOperandSizes.Add(ra::etoi(MemSize::MBF32), ra::data::MemSizeString(MemSize::MBF32));
    m_vOperandSizes.Add(ra::etoi(MemSize::MBF32LE), ra::data::MemSizeString(MemSize::MBF32LE));

    m_vOperatorTypes.Add(ra::etoi(TriggerOperatorType::Equals), L"=");
    m_vOperatorTypes.Add(ra::etoi(TriggerOperatorType::LessThan), L"<");
    m_vOperatorTypes.Add(ra::etoi(TriggerOperatorType::LessThanOrEqual), L"<=");
    m_vOperatorTypes.Add(ra::etoi(TriggerOperatorType::GreaterThan), L">");
    m_vOperatorTypes.Add(ra::etoi(TriggerOperatorType::GreaterThanOrEqual), L">=");
    m_vOperatorTypes.Add(ra::etoi(TriggerOperatorType::NotEquals), L"!=");
    m_vOperatorTypes.Add(ra::etoi(TriggerOperatorType::None), L"");
    m_vOperatorTypes.Add(ra::etoi(TriggerOperatorType::Multiply), L"*");
    m_vOperatorTypes.Add(ra::etoi(TriggerOperatorType::Divide), L"/");
    m_vOperatorTypes.Add(ra::etoi(TriggerOperatorType::Modulus), L"%");
    m_vOperatorTypes.Add(ra::etoi(TriggerOperatorType::Add), L"+");
    m_vOperatorTypes.Add(ra::etoi(TriggerOperatorType::Subtract), L"-");
    m_vOperatorTypes.Add(ra::etoi(TriggerOperatorType::BitwiseAnd), L"&");
    m_vOperatorTypes.Add(ra::etoi(TriggerOperatorType::BitwiseXor), L"^");
}

std::string TriggerViewModel::Serialize() const
{
    std::string buffer;
    SerializeAppend(buffer);
    return buffer;
}

bool TriggerViewModel::GroupViewModel::UpdateSerialized(const std::string& sSerialized)
{
    if (sSerialized != m_sSerialized)
    {
        m_sSerialized = sSerialized;
        m_pConditionSet = nullptr;
        return true;
    }

    return false;
}


rc_condset_t* TriggerViewModel::GroupViewModel::GetConditionSet(bool bIsValue) const
{
    if (m_pConditionSet != nullptr)
        return m_pConditionSet;

    const auto sTrigger = GetSerialized();

    rc_memrefs_t* pMemrefs = nullptr;
    if (ra::services::ServiceLocator::Exists<ra::services::AchievementRuntime>())
    {
        auto& pRuntime = ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>();
        auto* pGame = pRuntime.GetClient()->game;
        if (pGame != nullptr)
            pMemrefs = pGame->runtime.memrefs;
    }

    rc_preparse_state_t preparse;
    rc_init_preparse_state(&preparse);
    preparse.parse.existing_memrefs = pMemrefs;
    preparse.parse.is_value = bIsValue ? 1 : 0;
    preparse.parse.ignore_non_parse_errors = 1;
    rc_trigger_with_memrefs_t* trigger = RC_ALLOC(rc_trigger_with_memrefs_t, &preparse.parse);
    const char* sMemaddr = sTrigger.c_str();
    rc_parse_trigger_internal(&trigger->trigger, &sMemaddr, &preparse.parse);
    rc_preparse_alloc_memrefs(nullptr, &preparse);

    const auto nSize = preparse.parse.offset;
    if (nSize > 0)
    {
        m_sBuffer.resize(nSize);

        rc_reset_parse_state(&preparse.parse, m_sBuffer.data());
        trigger = RC_ALLOC(rc_trigger_with_memrefs_t, &preparse.parse);
        rc_preparse_alloc_memrefs(&trigger->memrefs, &preparse);

        preparse.parse.existing_memrefs = pMemrefs;
        preparse.parse.memrefs = &trigger->memrefs;
        preparse.parse.is_value = bIsValue ? 1 : 0;
        preparse.parse.ignore_non_parse_errors = 1;

        sMemaddr = sTrigger.c_str();
        rc_parse_trigger_internal(&trigger->trigger, &sMemaddr, &preparse.parse);
        trigger->trigger.has_memrefs = 1;

        m_pConditionSet = trigger->trigger.requirement;
    }

    return m_pConditionSet;
}

const std::string& TriggerViewModel::GroupViewModel::GetSerialized() const
{
    if (m_sSerialized == TriggerViewModel::GroupViewModel::NOT_SERIALIZED)
    {
        m_sSerialized.clear();

        if (m_pConditionSet)
        {
            TriggerConditionViewModel vmCondition;
            vmCondition.SetTriggerViewModel(m_pTriggerViewModel);

            rc_condition_t* pCondition = m_pConditionSet->conditions;
            while (pCondition != nullptr)
            {
                vmCondition.InitializeFrom(*pCondition);
                vmCondition.SerializeAppend(m_sSerialized);
                if (!pCondition->next)
                    break;

                ra::services::AchievementLogicSerializer::AppendConditionSeparator(m_sSerialized);
                pCondition = pCondition->next;
            }
        }
    }

    return m_sSerialized;
}

bool TriggerViewModel::GroupViewModel::UpdateMeasuredFlags()
{
    if (m_pConditionSet)
    {
        rc_condition_t* pCondition = m_pConditionSet->conditions;
        for (; pCondition != nullptr; pCondition = pCondition->next)
        {
            if (pCondition->type == RC_CONDITION_MEASURED)
            {
                m_sSerialized = TriggerViewModel::GroupViewModel::NOT_SERIALIZED;
                return true;
            }
        }
    }
    else if (m_pTriggerViewModel && !m_sSerialized.empty())
    {
        char cOldPrefix = 'G';
        char cNewPrefix = 'M';
        if (m_pTriggerViewModel->IsMeasuredTrackedAsPercent())
        {
            cOldPrefix = 'M';
            cNewPrefix = 'G';
        }

        bool bUpdated = false;
        for (auto nIndex = m_sSerialized.find(cOldPrefix); nIndex != std::string::npos; nIndex = m_sSerialized.find(cOldPrefix, nIndex + 2))
        {
            if (nIndex + 1 < m_sSerialized.length() && m_sSerialized.at(nIndex + 1) == ':')
            {
                m_sSerialized.at(nIndex) = cNewPrefix;
                bUpdated = true;
            }
        }

        return bUpdated;
    }

    return false;
}

void TriggerViewModel::SerializeAppend(std::string& sBuffer) const
{
    for (gsl::index nGroup = 0; nGroup < gsl::narrow_cast<gsl::index>(m_vGroups.Count()); ++nGroup)
    {
        const auto* vmGroup = m_vGroups.GetItemAt(nGroup);
        if (vmGroup != nullptr)
        {
            if (nGroup != 0)
                sBuffer.push_back(m_bIsValue ? '$' : 'S');

            sBuffer.append(vmGroup->GetSerialized());
        }
    }
}

void TriggerViewModel::CopyToClipboard()
{
    const auto sSerialized = Serialize();
    ra::services::ServiceLocator::Get<ra::services::IClipboard>().SetText(ra::Widen(sSerialized));
}

void TriggerViewModel::CopySelectedConditionsToClipboard()
{
    std::string sSerialized;

    auto* pGroup = m_vGroups.GetItemAt(GetSelectedGroupIndex());
    if (!pGroup)
        return;

    auto* pCondSet = pGroup->GetConditionSet(IsValue());
    if (!pCondSet)
        return;

    TriggerConditionViewModel vmCondition;
    rc_condition_t* pCondition = pCondSet->conditions;
    unsigned nScan = 0;

    for (const auto nIndex : m_vSelectedConditions)
    {
        while (nScan < nIndex && pCondition)
        {
            pCondition = pCondition->next;
            ++nScan;
        }

        if (!pCondition)
            break;

        vmCondition.InitializeFrom(*pCondition);
        vmCondition.SerializeAppend(sSerialized);
        sSerialized.push_back('_');
    }

    if (sSerialized.empty())
    {
        // no selection, get the whole group
        sSerialized = pGroup->GetSerialized();
    }
    else if (sSerialized.back() == '_')
    {
        // remove trailing joiner
        sSerialized.pop_back();
    }

    ra::services::ServiceLocator::Get<ra::services::IClipboard>().SetText(ra::Widen(sSerialized));
}

void TriggerViewModel::DeselectAllConditions()
{
    if (!m_vSelectedConditions.empty())
    {
        for (gsl::index nIndex = 0; nIndex < ra::to_signed(m_vConditions.Count()); ++nIndex)
            m_vConditions.SetItemValue(nIndex, TriggerConditionViewModel::IsSelectedProperty, false);

        m_vSelectedConditions.clear();
    }
}

static constexpr int RC_MULTIPLE_GROUPS = -99;

void TriggerViewModel::PasteFromClipboard()
{
    const std::wstring sClipboardText = ra::services::ServiceLocator::Get<ra::services::IClipboard>().GetText();
    if (sClipboardText.empty())
    {
        ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(L"Paste failed.", L"Clipboard did not contain any text.");
        return;
    }

    const auto nResult = AppendMemRefChain(ra::Narrow(sClipboardText));
    if (nResult != RC_OK)
    {
        if (nResult == RC_MULTIPLE_GROUPS)
        {
            ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(
                L"Paste failed.", L"Clipboard contained multiple groups.");
        }
        else
        {
            ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(
                L"Paste failed.", ra::StringPrintf(L"Clipboard did not contain valid %s conditions.", IsValue() ? "value" : "trigger"));
        }
    }
}

int TriggerViewModel::AppendMemRefChain(const std::string& sTrigger)
{
    // have to use internal parsing functions to decode conditions without full trigger/value
    // restriction validation (full validation will occur after new conditions are added)
    rc_preparse_state_t preparse;
    rc_init_preparse_state(&preparse);
    preparse.parse.is_value = IsValue();
    preparse.parse.ignore_non_parse_errors = 1;
    const char* memaddr = sTrigger.c_str();
    Expects(memaddr != nullptr);
    rc_parse_condset(&memaddr, &preparse.parse);

    const auto nSize = preparse.parse.offset;
    if (nSize == 0) // nothing to do
        return RC_OK;

    if (nSize < 0) // error occurred
        return nSize;

    if (*memaddr != '\0')
    {
        if (nSize > 0 && *memaddr == 'S') // alts detected
            return RC_MULTIPLE_GROUPS;

        return RC_INVALID_STATE; // additional test that was not parsed
    }

    auto* pGroup = m_vGroups.GetItemAt(GetSelectedGroupIndex());
    Expects(pGroup != nullptr);
    std::string sSerialized = pGroup->GetSerialized();
    if (!sSerialized.empty())
        ra::services::AchievementLogicSerializer::AppendConditionSeparator(sSerialized);
    sSerialized.append(sTrigger);
    pGroup->UpdateSerialized(sSerialized);

    m_bInitializingConditions = true;

    DeselectAllConditions();

    const auto nCountBefore = GetValue(ScrollMaximumProperty);
    UpdateConditions(pGroup);
    const auto nCountAfter = GetValue(ScrollMaximumProperty);
    EnsureVisible(nCountBefore, nCountAfter - nCountBefore);

    SelectRange(nCountBefore, gsl::narrow_cast<gsl::index>(nCountAfter) - 1, true);

    m_bInitializingConditions = false;

    UpdateVersion();
    return RC_OK;
}

void TriggerViewModel::RemoveSelectedConditions()
{
    const int nSelected = gsl::narrow_cast<int>(m_vSelectedConditions.size());
    if (nSelected == 0)
        return;

    ra::ui::viewmodels::MessageBoxViewModel vmMessageBox;
    vmMessageBox.SetMessage(ra::StringPrintf(L"Are you sure that you want to delete %d %s?",
        nSelected, (nSelected == 1) ? "condition" : "conditions"));
    vmMessageBox.SetButtons(ra::ui::viewmodels::MessageBoxViewModel::Buttons::YesNo);
    vmMessageBox.SetIcon(ra::ui::viewmodels::MessageBoxViewModel::Icon::Warning);

    if (vmMessageBox.ShowModal() != ra::ui::DialogResult::Yes)
        return;

    const auto nFirstVisibleIndex = GetValue(ScrollOffsetProperty);
    const auto nVisibleItemCount = gsl::narrow_cast<int>(m_vConditions.Count());
    auto* pGroup = m_vGroups.GetItemAt(GetSelectedGroupIndex());
    Expects(pGroup != nullptr);
    const auto* pCondSet = pGroup->GetConditionSet(IsValue());
    Expects(pCondSet != nullptr);

    std::string sSerialized;
    TriggerConditionViewModel vmCondition;
    int nIndex = 0;
    for (const auto* pCondition = pCondSet->conditions; pCondition; pCondition = pCondition->next)
    {
        if (m_vSelectedConditions.find(nIndex) == m_vSelectedConditions.end())
        {
            if (!sSerialized.empty())
                ra::services::AchievementLogicSerializer::AppendConditionSeparator(sSerialized);

            if (nIndex < nFirstVisibleIndex || nIndex >= nFirstVisibleIndex + nVisibleItemCount)
            {
                vmCondition.InitializeFrom(*pCondition);
                vmCondition.SerializeAppend(sSerialized);
            }
            else
            {
                m_vConditions.GetItemAt(gsl::narrow_cast<gsl::index>(nIndex) - nFirstVisibleIndex)->SerializeAppend(sSerialized);
            }
        }

        ++nIndex;
    }

    pGroup->UpdateSerialized(sSerialized);

    const auto nNewItemCount = GetValue(ScrollMaximumProperty) - gsl::narrow_cast<int>(m_vSelectedConditions.size());
    m_vSelectedConditions.clear();

    m_bInitializingConditions = true;

    SetValue(ScrollMaximumProperty, nNewItemCount);

    const auto nNewScrollOffset = nNewItemCount - nVisibleItemCount;
    if (nNewScrollOffset < nFirstVisibleIndex && nNewScrollOffset >= 0)
        SetValue(ScrollOffsetProperty, nNewScrollOffset);

    UpdateConditions(pGroup);

    m_bInitializingConditions = false;

    UpdateVersion();
}

void TriggerViewModel::MoveSelectedConditionsUp()
{
    const auto nSelectedCount = m_vSelectedConditions.size();
    if (nSelectedCount == 0)
        return;

    auto* pGroup = m_vGroups.GetItemAt(GetSelectedGroupIndex());
    if (pGroup != nullptr)
    {
        bool bChanged = false;

        std::vector<rc_condition_t*> vConditions;
        {
            rc_condition_t* pCondition = pGroup->m_pConditionSet->conditions;
            for (; pCondition; pCondition = pCondition->next)
                vConditions.push_back(pCondition);
        }

        const size_t nFirstIndex = gsl::narrow_cast<size_t>(*m_vSelectedConditions.begin());
        size_t nInsertIndex = (nFirstIndex > 1) ? nFirstIndex - 1 : 0;
        std::set<unsigned int> vNewSelectedConditions;
        for (const auto nIndex : m_vSelectedConditions)
        {
            if (nIndex > nInsertIndex)
            {
                auto* pMovedCondition = vConditions.at(nIndex);
                for (size_t nMoveIndex = nIndex; nMoveIndex > nInsertIndex; --nMoveIndex)
                    vConditions.at(nMoveIndex) = vConditions.at(nMoveIndex - 1);
                vConditions.at(nInsertIndex) = pMovedCondition;
                bChanged = true;
            }

            vNewSelectedConditions.insert(gsl::narrow_cast<unsigned int>(nInsertIndex));
            ++nInsertIndex;
        }

        if (bChanged)
        {
            m_vSelectedConditions.swap(vNewSelectedConditions);

            TriggerConditionViewModel vmCondition;
            std::string sSerialized;
            for (const auto* pCondition : vConditions)
            {
                Expects(pCondition != nullptr);
                if (!sSerialized.empty())
                    ra::services::AchievementLogicSerializer::AppendConditionSeparator(sSerialized);

                vmCondition.InitializeFrom(*pCondition);
                vmCondition.SerializeAppend(sSerialized);
            }

            pGroup->UpdateSerialized(sSerialized);

            m_bInitializingConditions = true;
            EnsureVisible(gsl::narrow_cast<int>(*m_vSelectedConditions.begin()), gsl::narrow_cast<int>(m_vSelectedConditions.size()));
            UpdateConditions(pGroup);
            m_bInitializingConditions = false;

            UpdateVersion();
        }
    }
}

void TriggerViewModel::MoveSelectedConditionsDown()
{
    const auto nSelectedCount = m_vSelectedConditions.size();
    if (nSelectedCount == 0)
        return;

    auto* pGroup = m_vGroups.GetItemAt(GetSelectedGroupIndex());
    if (pGroup != nullptr)
    {
        bool bChanged = false;

        std::vector<rc_condition_t*> vConditions;
        {
            rc_condition_t* pCondition = pGroup->m_pConditionSet->conditions;
            for (; pCondition; pCondition = pCondition->next)
                vConditions.push_back(pCondition);
        }

        const size_t nLastIndex = gsl::narrow_cast<size_t>(*m_vSelectedConditions.rbegin());
        size_t nInsertIndex = std::min(nLastIndex + 1, vConditions.size() - 1);
        std::set<unsigned int> vNewSelectedConditions;
        for (auto iter = m_vSelectedConditions.rbegin(); iter != m_vSelectedConditions.rend(); ++iter)
        {
            const auto nIndex = *iter;
            if (nIndex < nInsertIndex)
            {
                auto* pMovedCondition = vConditions.at(nIndex);
                for (size_t nMoveIndex = nIndex; nMoveIndex < nInsertIndex; ++nMoveIndex)
                    vConditions.at(nMoveIndex) = vConditions.at(nMoveIndex + 1);
                vConditions.at(nInsertIndex) = pMovedCondition;
                bChanged = true;
            }

            vNewSelectedConditions.insert(gsl::narrow_cast<unsigned int>(nInsertIndex));
            --nInsertIndex;
        }

        if (bChanged)
        {
            m_vSelectedConditions.swap(vNewSelectedConditions);

            TriggerConditionViewModel vmCondition;
            std::string sSerialized;
            for (const auto* pCondition : vConditions)
            {
                Expects(pCondition != nullptr);
                if (!sSerialized.empty())
                    ra::services::AchievementLogicSerializer::AppendConditionSeparator(sSerialized);

                vmCondition.InitializeFrom(*pCondition);
                vmCondition.SerializeAppend(sSerialized);
            }

            pGroup->UpdateSerialized(sSerialized);

            m_bInitializingConditions = true;
            EnsureVisible(gsl::narrow_cast<int>(nInsertIndex) + 1, gsl::narrow_cast<int>(nLastIndex - nInsertIndex));
            UpdateConditions(pGroup);
            m_bInitializingConditions = false;

            UpdateVersion();
        }
    }
}

void TriggerViewModel::EnsureVisible(int nFirstIndex, int nCount)
{
    const auto nFirstVisibleIndex = GetValue(ScrollOffsetProperty);
    if (nFirstIndex < nFirstVisibleIndex)
    {
        if (nFirstIndex < 0)
            nFirstIndex = 0;
        SetValue(ScrollOffsetProperty, nFirstIndex);
    }
    else
    {
        const auto nNumVisibleConditions = GetValue(VisibleItemCountProperty);
        if (nFirstIndex + nCount > nFirstVisibleIndex + nNumVisibleConditions)
        {
            int nNewOffset = nFirstIndex + nCount - nNumVisibleConditions;
            if (nNewOffset < 0)
                nNewOffset = 0;
            SetValue(ScrollOffsetProperty, nNewOffset);
        }
    }
}

void TriggerViewModel::NewCondition()
{
    auto* pGroup = m_vGroups.GetItemAt(GetSelectedGroupIndex());
    if (pGroup == nullptr)
        return;

    // assume the user wants to create a condition for the currently watched memory address.
    const auto& pMemoryInspector = ra::services::ServiceLocator::Get<ra::ui::viewmodels::WindowManager>().MemoryInspector;
    auto sMemRef = pMemoryInspector.GetCurrentAddressMemRefChain();

    if (m_bIsValue && m_vConditions.Count() == 0)
    {
        // first condition added to a value should be Measured and not have a operator/target.
#ifndef NDEBUG
        const auto nIndex = sMemRef.find_last_of('_');
        assert(sMemRef.find_first_of("M:", (nIndex == std::string::npos) ? 0 : nIndex + 1) != std::string::npos);
#endif
    }
    else
    {
        MemSize nSize = MemSize::EightBit;
        const auto nIndex = sMemRef.find_last_of('_');
        auto nIndex2 = (nIndex == std::string::npos) ? 0 : nIndex + 1;
        uint8_t nMemRefSize = RC_MEMSIZE_8_BITS;
        uint32_t nAddress = 0;

        // last condition of memref chain should be Measured. if found, remove the Measured
        if (sMemRef.at(nIndex2) == 'M' && sMemRef.at(nIndex2 + 1) == ':')
        {
            if (m_bIsValue)
            {
                // value deserializer doesn't like conditions with a type. change to AndNext
                sMemRef.at(nIndex2) = 'N';
                nIndex2 += 2;
            }
            else
            {
                sMemRef.erase(nIndex2, 2);
            }
        }
        else if (m_bIsValue)
        {
            // value deserializer doesn't like conditions without a type. insert AndNext
            sMemRef.insert(0, "N:");
        }

        // extract the size
        const char* sMemRefPtr = &sMemRef.at(nIndex2);
        if (rc_parse_memref(&sMemRefPtr, &nMemRefSize, &nAddress) == RC_OK)
            nSize = ra::data::models::TriggerValidation::MapRcheevosMemSize(nMemRefSize);

        // assume the user wants to compare to the current value of the watched memory address
        ra::services::AchievementLogicSerializer::AppendOperator(sMemRef, ra::services::TriggerOperatorType::Equals);

        // pull the value from the memory viewer to avoid reading from the emulator
        nAddress = pMemoryInspector.GetCurrentAddress();
        rc_typed_value_t nTypedValue;
        nTypedValue.type = RC_VALUE_TYPE_UNSIGNED;
        nTypedValue.value.u32 = pMemoryInspector.Viewer().GetValueAtAddress(nAddress);
        if (ra::data::MemSizeBytes(nSize) > 1)
        {
            nTypedValue.value.u32 |= pMemoryInspector.Viewer().GetValueAtAddress(nAddress + 1) << 8;
            if (ra::data::MemSizeBytes(nSize) > 2)
            {
                nTypedValue.value.u32 |= pMemoryInspector.Viewer().GetValueAtAddress(nAddress + 2) << 16;
                nTypedValue.value.u32 |= pMemoryInspector.Viewer().GetValueAtAddress(nAddress + 3) << 24;
            }
        }
        rc_transform_memref_value(&nTypedValue, nMemRefSize);

        // append the value to the condition
        if (nTypedValue.type == RC_VALUE_TYPE_FLOAT)
        {
            ra::services::AchievementLogicSerializer::AppendOperand(sMemRef, ra::services::TriggerOperandType::Float,
                                                                    nSize, nTypedValue.value.f32);
        }
        else
        {
            ra::services::AchievementLogicSerializer::AppendOperand(sMemRef, ra::services::TriggerOperandType::Value,
                                                                    nSize, nTypedValue.value.u32);
        }
    }

    AppendMemRefChain(sMemRef);
}

void TriggerViewModel::AddAltGroup(int nId, rc_condset_t* pConditionSet)
{
    auto& vmAltGroup = m_vGroups.Add();
    vmAltGroup.SetTriggerViewModel(this);
    vmAltGroup.SetId(nId);
    vmAltGroup.SetLabel(ra::StringPrintf(L"%s%d", (nId < 1000) ? "Alt " : "A", nId));
    vmAltGroup.m_pConditionSet = pConditionSet;
}

void TriggerViewModel::InitializeGroups(const rc_trigger_t& pTrigger)
{
    m_vGroups.RemoveNotifyTarget(*this);
    m_vGroups.BeginUpdate();

    SetMeasuredTrackedAsPercent(pTrigger.measured_as_percent);

    // this will not update the conditions collection because OnValueChange ignores it when m_vGroups.IsUpdating
    SetSelectedGroupIndex(0);

    m_vGroups.Clear();

    auto& vmCoreGroup = m_vGroups.Add();
    vmCoreGroup.SetTriggerViewModel(this);
    vmCoreGroup.SetId(0);
    vmCoreGroup.SetLabel(m_bIsValue ? L"Value" : L"Core");
    vmCoreGroup.SetSelected(true);
    if (pTrigger.requirement)
        vmCoreGroup.m_pConditionSet = pTrigger.requirement;

    int nId = 0;
    rc_condset_t* pConditionSet = pTrigger.alternative;
    for (; pConditionSet != nullptr; pConditionSet = pConditionSet->next)
        AddAltGroup(++nId, pConditionSet);

    m_vGroups.EndUpdate();
    m_vGroups.AddNotifyTarget(*this);

    // forcibly update the conditions from the group that was selected earlier
    InitializeConditions(&vmCoreGroup);
}

void TriggerViewModel::InitializeFrom(const rc_trigger_t& pTrigger)
{
    {
        std::lock_guard<std::mutex> lock(m_pMutex);

        m_bIsValue = false;
        m_pTrigger = nullptr;
    }

    InitializeGroups(pTrigger);
}

rc_trigger_t* TriggerViewModel::ParseTrigger(const std::string& sTrigger)
{
    if (m_bIsValue)
    {
        const auto nSize = rc_value_size(sTrigger.c_str());
        if (nSize > 0)
        {
            m_sTriggerBuffer.resize(nSize + sizeof(rc_trigger_with_memrefs_t));
            rc_value_t* pValue = rc_parse_value(m_sTriggerBuffer.data(), sTrigger.c_str(), nullptr, 0);
            rc_trigger_with_memrefs_t* pTriggerWithMemrefs;
            GSL_SUPPRESS_TYPE1 pTriggerWithMemrefs = reinterpret_cast<rc_trigger_with_memrefs_t*>(m_sTriggerBuffer.data() + nSize);
            memset(pTriggerWithMemrefs, 0, sizeof(rc_trigger_with_memrefs_t));
            auto* pTrigger = &pTriggerWithMemrefs->trigger;
            pTrigger->requirement = pValue->conditions;
            pTrigger->alternative = pValue->conditions->next;

            if (pValue->has_memrefs)
            {
                rc_value_with_memrefs_t* pValueWithMemrefs;
                GSL_SUPPRESS_TYPE1 pValueWithMemrefs = reinterpret_cast<rc_value_with_memrefs_t*>(pValue);
                memcpy(&pTriggerWithMemrefs->memrefs, &pValueWithMemrefs->memrefs,
                       sizeof(pTriggerWithMemrefs->memrefs));
                pTrigger->has_memrefs = 1;
            }

            return pTrigger;
        }
    }
    else
    {
        const auto nSize = rc_trigger_size(sTrigger.c_str());
        if (nSize > 0)
        {
            m_sTriggerBuffer.resize(nSize);
            return rc_parse_trigger(m_sTriggerBuffer.data(), sTrigger.c_str(), nullptr, 0);
        }
    }

    return nullptr;
}

void TriggerViewModel::InitializeFrom(const std::string& sTrigger, const ra::data::models::CapturedTriggerHits& pCapturedHits)
{
    {
        std::lock_guard<std::mutex> lock(m_pMutex);

        m_pTrigger = ParseTrigger(sTrigger);
    }

    if (m_pTrigger)
    {
        pCapturedHits.Restore(m_pTrigger, sTrigger);
        InitializeGroups(*m_pTrigger);
    }
    else if (m_bIsValue)
    {
        rc_value_t pValue;
        memset(&pValue, 0, sizeof(pValue));
        InitializeFrom(pValue);
    }
    else
    {
        rc_trigger_t pTrigger;
        memset(&pTrigger, 0, sizeof(pTrigger));
        InitializeFrom(pTrigger);
    }
}

void TriggerViewModel::InitializeFrom(const rc_value_t& pValue)
{
    {
        std::lock_guard<std::mutex> lock(m_pMutex);

        m_bIsValue = true;
        m_sTriggerBuffer.resize(sizeof(rc_trigger_t));
        GSL_SUPPRESS_TYPE1 m_pTrigger = reinterpret_cast<rc_trigger_t*>(m_sTriggerBuffer.data());
        memset(m_pTrigger, 0, sizeof(rc_trigger_t));
        m_pTrigger->requirement = pValue.conditions;
        m_pTrigger->alternative = (pValue.conditions) ? pValue.conditions->next : nullptr;
    }

    InitializeGroups(*m_pTrigger);
}

void TriggerViewModel::UpdateGroups(const rc_trigger_t& pTrigger)
{
    const int nSelectedIndex = GetSelectedGroupIndex();

    m_vGroups.RemoveNotifyTarget(*this);
    m_vGroups.BeginUpdate();

    // update core group
    auto* vmCoreGroup = m_vGroups.GetItemAt(0);
    Expects(vmCoreGroup != nullptr);
    vmCoreGroup->m_pConditionSet = pTrigger.requirement;
    vmCoreGroup->ResetSerialized();

    // update alt groups (add any new alt groups)
    gsl::index nIndex = 0;
    rc_condset_t* pConditionSet = pTrigger.alternative;
    for (; pConditionSet != nullptr; pConditionSet = pConditionSet->next)
    {
        auto* vmAltGroup = m_vGroups.GetItemAt(++nIndex);
        if (vmAltGroup != nullptr)
        {
            vmAltGroup->m_pConditionSet = pConditionSet;
            vmAltGroup->ResetSerialized();
        }
        else
        {
            AddAltGroup(gsl::narrow_cast<int>(nIndex), pConditionSet);
        }
    }

    // remove any extra alt groups
    ++nIndex;
    while (m_vGroups.Count() > ra::to_unsigned(nIndex))
        m_vGroups.RemoveAt(m_vGroups.Count() - 1);

    m_vGroups.EndUpdate();
    m_vGroups.AddNotifyTarget(*this);

    if (nSelectedIndex < gsl::narrow_cast<int>(m_vGroups.Count()))
    {
        // forcibly update the conditions from the selected group
        m_bInitializingConditions = true;
        UpdateConditions(m_vGroups.GetItemAt(nSelectedIndex));
        m_bInitializingConditions = false;
    }
    else
    {
        // selected group no longer exists, select core group
        SetSelectedGroupIndex(0);
    }
}

void TriggerViewModel::UpdateFrom(const rc_trigger_t& pTrigger)
{
    {
        std::lock_guard<std::mutex> lock(m_pMutex);

        m_pTrigger = nullptr;
        m_sTriggerBuffer.clear();
    }

    UpdateGroups(pTrigger);
}

void TriggerViewModel::UpdateFrom(const std::string& sTrigger)
{
    {
        std::lock_guard<std::mutex> lock(m_pMutex);

        m_pTrigger = ParseTrigger(sTrigger);
    }

    if (m_pTrigger)
    {
        UpdateGroups(*m_pTrigger);

        DispatchMemoryRead([this] { UpdateMemrefs(); });
    }
    else
    {
        rc_trigger_t pTrigger;
        memset(&pTrigger, 0, sizeof(pTrigger));
        UpdateGroups(pTrigger);
    }
}

void TriggerViewModel::UpdateFrom(const rc_value_t& pValue)
{
    {
        std::lock_guard<std::mutex> lock(m_pMutex);

        m_pTrigger = nullptr;
        m_sTriggerBuffer.resize(sizeof(rc_trigger_t));
        GSL_SUPPRESS_TYPE1 m_pTrigger = reinterpret_cast<rc_trigger_t*>(m_sTriggerBuffer.data());
        memset(m_pTrigger, 0, sizeof(rc_trigger_t));
        m_pTrigger->requirement = pValue.conditions;
        m_pTrigger->alternative = pValue.conditions->next;
    }

    UpdateGroups(*m_pTrigger);
}

void TriggerViewModel::SelectRange(gsl::index nFrom, gsl::index nTo, bool bValue)
{
    m_vConditions.RemoveNotifyTarget(*this);

    if (bValue)
    {
        for (auto nIndex = nFrom; nIndex <= nTo; ++nIndex)
            m_vSelectedConditions.insert(gsl::narrow_cast<unsigned int>(nIndex));
    }
    else if (nFrom == 0 && nTo == gsl::narrow_cast<gsl::index>(GetValue(ScrollMaximumProperty)) - 1)
    {
        m_vSelectedConditions.clear();
    }
    else
    {
        for (auto nIndex = nFrom; nIndex <= nTo; ++nIndex)
            m_vSelectedConditions.erase(gsl::narrow_cast<unsigned int>(nIndex));
    }

    const auto nFirstVisibleIndex = gsl::narrow_cast<gsl::index>(GetValue(ScrollOffsetProperty));
    nTo -= nFirstVisibleIndex;
    if (nTo >= 0)
    {
        if (nTo >= gsl::narrow_cast<gsl::index>(m_vConditions.Count()))
            nTo = gsl::narrow_cast<gsl::index>(m_vConditions.Count()) - 1;

        nFrom -= nFirstVisibleIndex;
        if (nFrom < 0)
            nFrom = 0;

        for (auto nIndex = nFrom; nIndex <= nTo; ++nIndex)
            m_vConditions.SetItemValue(nIndex, TriggerConditionViewModel::IsSelectedProperty, bValue);
    }

    m_vConditions.AddNotifyTarget(*this);
}

int TriggerViewModel::SetSelectedItemValues(const IntModelProperty& pProperty, int nNewValue)
{
    if (m_vSelectedConditions.empty())
        return 0;

    m_vConditions.RemoveNotifyTarget(m_pConditionsMonitor);
    UpdateCurrentGroup(false, &pProperty, nNewValue);
    m_vConditions.AddNotifyTarget(m_pConditionsMonitor);

    return gsl::narrow_cast<int>(m_vSelectedConditions.size());
}

void TriggerViewModel::OnViewModelBoolValueChanged(gsl::index, const BoolModelProperty::ChangeArgs& args)
{
    if (args.Property == GroupViewModel::IsSelectedProperty)
    {
        for (gsl::index i = 0; i < ra::to_signed(m_vGroups.Count()); ++i)
        {
            if (m_vGroups.GetItemAt(i)->IsSelected())
            {
                SetSelectedGroupIndex(gsl::narrow_cast<int>(i));
                return;
            }
        }

        SetSelectedGroupIndex(-1);
    }
}

void TriggerViewModel::ConditionsMonitor::OnViewModelIntValueChanged(gsl::index, const IntModelProperty::ChangeArgs& args)
{
    if (args.Property == TriggerConditionViewModel::SourceTypeProperty ||
        args.Property == TriggerConditionViewModel::SourceSizeProperty ||
        args.Property == TriggerConditionViewModel::TargetTypeProperty ||
        args.Property == TriggerConditionViewModel::TargetSizeProperty ||
        args.Property == TriggerConditionViewModel::TypeProperty ||
        args.Property == TriggerConditionViewModel::OperatorProperty ||
        args.Property == TriggerConditionViewModel::RequiredHitsProperty)
    {
        UpdateCurrentGroup();
    }
}

void TriggerViewModel::ConditionsMonitor::OnViewModelStringValueChanged(gsl::index, const StringModelProperty::ChangeArgs& args)
{
    if (args.Property == TriggerConditionViewModel::SourceValueProperty ||
        args.Property == TriggerConditionViewModel::TargetValueProperty)
    {
        UpdateCurrentGroup();
    }
}

void TriggerViewModel::ConditionsMonitor::OnViewModelAdded(gsl::index)
{
    if (!m_vmTrigger->Conditions().IsUpdating())
        UpdateCurrentGroup();
}

void TriggerViewModel::ConditionsMonitor::OnViewModelRemoved(gsl::index)
{
    if (!m_vmTrigger->Conditions().IsUpdating())
        UpdateCurrentGroup();
}

void TriggerViewModel::ConditionsMonitor::OnEndViewModelCollectionUpdate()
{
    UpdateCurrentGroup();
}

void TriggerViewModel::ConditionsMonitor::UpdateCurrentGroup()
{
    if (m_nUpdateCount > 0)
    {
        m_bUpdatePending = true;
        return;
    }

    m_vmTrigger->UpdateCurrentGroup(false, nullptr, 0);
}

void TriggerViewModel::ConditionsMonitor::EndUpdate()
{
    if (m_nUpdateCount > 0)
    {
        if (--m_nUpdateCount == 0)
        {
            if (m_bUpdatePending)
            {
                m_bUpdatePending = false;
                UpdateCurrentGroup();
            }
        }
    }
}

void TriggerViewModel::SuspendConditionMonitor() const noexcept
{
    m_pConditionsMonitor.BeginUpdate();
}

void TriggerViewModel::ResumeConditionMonitor() const
{
    m_pConditionsMonitor.EndUpdate();
}

void TriggerViewModel::UpdateCurrentGroup(bool bRememberHits, const IntModelProperty* pProperty, int nNewValue)
{
    auto* pGroup = m_vGroups.GetItemAt(GetSelectedGroupIndex());
    if (!pGroup)
        return;

    auto* pCondSet = pGroup->GetConditionSet(IsValue());
    if (!pCondSet)
        return;

    const auto nConditionCount = GetValue(ScrollMaximumProperty);
    const auto nFirstVisibleIndex = GetValue(ScrollOffsetProperty);
    const auto nVisibleItemCount = gsl::narrow_cast<int>(m_vConditions.Count());

    std::vector<unsigned> vCurrentHits;

    std::string sSerialized;
    TriggerConditionViewModel vmCondition;
    TriggerConditionViewModel* pvmCondition = nullptr;
    rc_condition_t* pCondition = pCondSet->conditions;
    int nIndex = 0;
    while (pCondition != nullptr)
    {
        if (bRememberHits)
            vCurrentHits.push_back(pCondition->current_hits);

        if (nIndex < nFirstVisibleIndex || nIndex >= nFirstVisibleIndex + nVisibleItemCount)
        {
            vmCondition.InitializeFrom(*pCondition);
            pvmCondition = &vmCondition;
        }
        else
        {
            pvmCondition = m_vConditions.GetItemAt(gsl::narrow_cast<gsl::index>(nIndex) - nFirstVisibleIndex);
            Expects(pvmCondition != nullptr);
        }

        if (pProperty && m_vSelectedConditions.find(gsl::narrow_cast<unsigned int>(nIndex)) != m_vSelectedConditions.end())
            pvmCondition->PushValue(*pProperty, nNewValue);

        pvmCondition->SerializeAppend(sSerialized);

        if (!pCondition->next)
            break;

        ++nIndex;
        if (nIndex == nConditionCount)
            break;

        ra::services::AchievementLogicSerializer::AppendConditionSeparator(sSerialized);
        pCondition = pCondition->next;
    }

    if (pGroup->UpdateSerialized(sSerialized))
    {
        UpdateVersion();

        if (bRememberHits)
        {
            nIndex = 0;
            for (pCondition = pCondSet->conditions; pCondition && ra::to_unsigned(nIndex) < vCurrentHits.size(); pCondition = pCondition->next)
                pCondition->current_hits = vCurrentHits.at(nIndex++);
        }
    }
}

void TriggerViewModel::UpdateVersion()
{
    SetValue(VersionProperty, GetValue(VersionProperty) + 1);
}

void TriggerViewModel::UpdateVersionAndRestoreHits()
{
    // WARNING: this method should only be called when the number and type of conditions is known to not change
    std::vector<unsigned> vCurrentHits;
    for (gsl::index i = 0; i < gsl::narrow_cast<gsl::index>(m_vGroups.Count()); ++i)
    {
        auto* pGroup = m_vGroups.GetItemAt(i);
        if (pGroup != nullptr && pGroup->m_pConditionSet)
        {
            const auto* pCondition = pGroup->m_pConditionSet->conditions;
            while (pCondition != nullptr)
            {
                vCurrentHits.push_back(pCondition->current_hits);
                pCondition = pCondition->next;
            }
        }
    }

    UpdateVersion();

    gsl::index nHitIndex = 0;
    for (gsl::index i = 0; i < gsl::narrow_cast<gsl::index>(m_vGroups.Count()); ++i)
    {
        auto* pGroup = m_vGroups.GetItemAt(i);
        if (pGroup != nullptr && pGroup->m_pConditionSet)
        {
            auto* pCondition = pGroup->m_pConditionSet->conditions;
            while (pCondition != nullptr)
            {
                pCondition->current_hits = vCurrentHits.at(nHitIndex++);
                pCondition = pCondition->next;
            }
        }
    }
}

void TriggerViewModel::OnValueChanged(const IntModelProperty::ChangeArgs& args)
{
    if (args.Property == ScrollOffsetProperty)
    {
        if (!m_bInitializingConditions)
            UpdateConditions();
    }
    else if (args.Property == SelectedGroupIndexProperty)
    {
        // ignore change caused in InitializeFrom
        if (!m_vGroups.IsUpdating())
        {
            m_vSelectedConditions.clear();

            // disable notifications while updating the selected group to prevent re-entrancy
            m_vGroups.RemoveNotifyTarget(*this);

            auto* pGroup = m_vGroups.GetItemAt(args.tNewValue);
            if (pGroup != nullptr) // may be null if out of range (i.e. -1)
                pGroup->SetSelected(true);

            if (args.tOldValue != -1)
            {
                auto* pOldGroup = m_vGroups.GetItemAt(args.tOldValue);
                if (pOldGroup != nullptr) // group may have been deleted
                    pOldGroup->SetSelected(false);
            }

            m_vGroups.AddNotifyTarget(*this);

            InitializeConditions(pGroup);
        }
    }
    else if (args.Property == VisibleItemCountProperty)
    {
        UpdateConditions();
    }

    ViewModelBase::OnValueChanged(args);
}

void TriggerViewModel::OnValueChanged(const BoolModelProperty::ChangeArgs& args)
{
    if (args.Property == IsMeasuredTrackedAsPercentProperty)
    {
        // ignore change caused in InitializeFrom
        if (!m_vGroups.IsUpdating())
        {
            bool bChanged = false;
            for (gsl::index i = 0; i < gsl::narrow_cast<gsl::index>(m_vGroups.Count()); ++i)
            {
                auto* pGroup = m_vGroups.GetItemAt(i);
                if (pGroup != nullptr)
                    bChanged |= pGroup->UpdateMeasuredFlags();
            }

            if (bChanged)
                UpdateVersionAndRestoreHits();
        }
    }

    ViewModelBase::OnValueChanged(args);
}

void TriggerViewModel::UpdateConditions()
{
    auto* pGroup = m_vGroups.GetItemAt(GetSelectedGroupIndex());
    if (pGroup != nullptr)
    {
        m_bInitializingConditions = true;
        UpdateConditions(pGroup);
        m_bInitializingConditions = false;
    }
}

void TriggerViewModel::InitializeConditions(const GroupViewModel* pGroup)
{
    m_bInitializingConditions = true;
    SetValue(ScrollOffsetProperty, 0);

    UpdateConditions(pGroup);
    m_bInitializingConditions = false;
}

void TriggerViewModel::UpdateConditions(const GroupViewModel* pGroup)
{
    int nNewScrollMaximum = 0;

    {
        std::lock_guard<std::mutex> lock(m_pMutex);

        const auto nVisibleConditions = GetValue(VisibleItemCountProperty);

        if (!m_vConditions.IsUpdating())
            m_vConditions.RemoveNotifyTarget(m_pConditionsMonitor);
        m_vConditions.BeginUpdate();

        int nIndex = 0;
        m_bHasHitChain = false;

        if (pGroup != nullptr)
        {
            rc_condset_t* pConditions = pGroup->GetConditionSet(IsValue());
            if (pConditions)
            {
                const auto nFirstCondition = GetValue(ScrollOffsetProperty);
                int nVisibleIndex = 0;

                bool bIsIndirect = false;
                rc_condition_t* pCondition = pConditions->conditions;
                for (; pCondition != nullptr; pCondition = pCondition->next)
                {
                    if (nIndex++ >= nFirstCondition && nVisibleIndex < nVisibleConditions)
                    {
                        auto* vmCondition = m_vConditions.GetItemAt(gsl::narrow_cast<gsl::index>(nVisibleIndex++));
                        if (vmCondition == nullptr)
                        {
                            vmCondition = &m_vConditions.Add();
                            if (vmCondition == nullptr) // this should be an Ensures(), but the exception trips up the code analysis when the lock is used in an inner scope
                                break;
                            vmCondition->SetTriggerViewModel(this);
                        }

                        vmCondition->SetIndex(nIndex);
                        vmCondition->InitializeFrom(*pCondition);
                        vmCondition->SetCurrentHits(pCondition->current_hits);
                        vmCondition->SetTotalHits(0);

                        vmCondition->SetIndirect(bIsIndirect);

                        if (m_vSelectedConditions.empty())
                            vmCondition->SetSelected(false);
                        else
                            vmCondition->SetSelected(m_vSelectedConditions.find(nIndex - 1) != m_vSelectedConditions.end());
                    }

                    bIsIndirect = (pCondition->type == RC_CONDITION_ADD_ADDRESS);

                    m_bHasHitChain |=
                        (pCondition->type == RC_CONDITION_ADD_HITS || pCondition->type == RC_CONDITION_SUB_HITS);
                }

                nNewScrollMaximum = nIndex;
            }

            if (m_bHasHitChain)
                UpdateTotalHits(pGroup);
        }

        if (nIndex > nVisibleConditions)
            nIndex = nVisibleConditions;
        for (gsl::index nScan = m_vConditions.Count() - 1; nScan >= nIndex; --nScan)
            m_vConditions.RemoveAt(nScan);
    }

    SetValue(ScrollMaximumProperty, nNewScrollMaximum);

    m_vConditions.EndUpdate();
    if (!m_vConditions.IsUpdating())
        m_vConditions.AddNotifyTarget(m_pConditionsMonitor);
}

static unsigned int ParseNumeric(const std::wstring& sValue)
{
    unsigned int nValue = 0;
    std::wstring sError;
    if (ra::ParseNumeric(sValue, nValue, sError))
        return nValue;

    return 0;
}

void TriggerViewModel::ToggleDecimal()
{
    m_vConditions.RemoveNotifyTarget(m_pConditionsMonitor);
    m_vConditions.BeginUpdate();

    for (gsl::index nIndex = m_vConditions.Count() - 1; nIndex >= 0; --nIndex)
    {
        auto* pItem = m_vConditions.GetItemAt(nIndex);
        if (!pItem)
            continue;

        if (pItem->GetSourceType() == TriggerOperandType::Value)
            pItem->SetSourceValue(ParseNumeric(pItem->GetSourceValue()));
        if (pItem->GetTargetType() == TriggerOperandType::Value)
            pItem->SetTargetValue(ParseNumeric(pItem->GetTargetValue()));
    }

    m_vConditions.EndUpdate();
    m_vConditions.AddNotifyTarget(m_pConditionsMonitor);
}

void TriggerViewModel::UpdateTotalHits(const GroupViewModel* pGroup)
{
    int nHits = 0;
    bool bIsHitsChain = false;

    const gsl::index nFirstVisibleCondition = gsl::narrow_cast<gsl::index>(GetValue(ScrollOffsetProperty));
    const gsl::index nNumVisibleConditions = gsl::narrow_cast<gsl::index>(GetValue(VisibleItemCountProperty));
    gsl::index nConditionIndex = 0;
    rc_condition_t* pCondition = pGroup->m_pConditionSet->conditions;
    for (; pCondition != nullptr; pCondition = pCondition->next)
    {
        if (pCondition->type == RC_CONDITION_ADD_HITS)
        {
            bIsHitsChain = true;
            nHits += pCondition->current_hits;
        }
        else if (pCondition->type == RC_CONDITION_SUB_HITS)
        {
            bIsHitsChain = true;
            nHits -= pCondition->current_hits;
        }
        else if (!rc_condition_is_combining(pCondition))
        {
            if (bIsHitsChain)
            {
                if (nConditionIndex >= nFirstVisibleCondition)
                {
                    auto* vmCondition = m_vConditions.GetItemAt(nConditionIndex - nFirstVisibleCondition);
                    if (vmCondition != nullptr)
                    {
                        nHits += pCondition->current_hits;
                        vmCondition->SetTotalHits(nHits);
                    }
                }
                bIsHitsChain = false;
            }
            nHits = 0;
        }

        ++nConditionIndex;
        if (nConditionIndex == nFirstVisibleCondition + nNumVisibleConditions)
            break;
    }
}

void TriggerViewModel::AddGroup()
{
    const int nGroups = gsl::narrow_cast<int>(m_vGroups.Count());

    if (nGroups == 1 && !m_bIsValue)
    {
        // only a core group, add two alt groups
        AddAltGroup(nGroups, nullptr);
        AddAltGroup(nGroups + 1, nullptr);
    }
    else
    {
        // already at least one alt group, only add one more
        AddAltGroup(nGroups, nullptr);
    }

    SetSelectedGroupIndex(nGroups);
    SetValue(EnsureVisibleGroupIndexProperty, nGroups);

    UpdateVersion();
}

void TriggerViewModel::RemoveGroup()
{
    int nIndex = GetSelectedGroupIndex();
    if (nIndex < 0) // no selection
        return;

    if (nIndex == 0)
    {
        ra::ui::viewmodels::MessageBoxViewModel::ShowWarningMessage(L"The core group cannot be removed.");
        return;
    }

    const auto* pGroup = m_vGroups.GetItemAt(nIndex);
    Expects(pGroup != nullptr);

    if (m_vConditions.Count() > 0)
    {
        ra::ui::viewmodels::MessageBoxViewModel vmWarning;
        vmWarning.SetHeader(ra::StringPrintf(L"Are you sure that you want to delete %s?", pGroup->GetLabel()));
        vmWarning.SetMessage(L"The group is not empty, and this operation cannot be reverted.");
        vmWarning.SetIcon(ra::ui::viewmodels::MessageBoxViewModel::Icon::Warning);
        vmWarning.SetButtons(ra::ui::viewmodels::MessageBoxViewModel::Buttons::YesNo);

        const auto& pEditor = ra::services::ServiceLocator::Get<ra::ui::viewmodels::WindowManager>().AssetEditor;
        if (vmWarning.ShowModal(pEditor) != ra::ui::DialogResult::Yes)
            return;
    }

    SetSelectedGroupIndex(-1);

    m_vGroups.RemoveAt(nIndex);

    if (nIndex == gsl::narrow_cast<int>(m_vGroups.Count()))
    {
        --nIndex;
    }
    else
    {
        for (gsl::index nScan = gsl::narrow_cast<gsl::index>(nIndex); nScan < gsl::narrow_cast<gsl::index>(m_vGroups.Count()); ++nScan)
            m_vGroups.GetItemAt(nScan)->SetLabel(ra::StringPrintf(L"Alt %d", nScan));
    }

    SetSelectedGroupIndex(nIndex);
    SetValue(EnsureVisibleGroupIndexProperty, nIndex);

    UpdateVersion();
}

void TriggerViewModel::DoFrame()
{
    // if the trigger is managed by the viewmodel (not the runtime) then we need to update the memrefs
    UpdateMemrefs();

    {
        std::lock_guard<std::mutex> lock(m_pMutex);

        auto* pGroup = m_vGroups.GetItemAt(GetSelectedGroupIndex());
        if (pGroup == nullptr || !pGroup->m_pConditionSet)
            return;

        if (!m_vConditions.IsUpdating())
            m_vConditions.RemoveNotifyTarget(m_pConditionsMonitor);

        m_vConditions.BeginUpdate();

        const gsl::index nFirstVisibleCondition = gsl::narrow_cast<gsl::index>(GetValue(ScrollOffsetProperty));
        const gsl::index nNumVisibleConditions = gsl::narrow_cast<gsl::index>(GetValue(VisibleItemCountProperty));
        gsl::index nConditionIndex = 0;
        rc_condition_t* pCondition = pGroup->m_pConditionSet->conditions;
        for (; pCondition != nullptr; pCondition = pCondition->next)
        {
            if (nConditionIndex >= nFirstVisibleCondition)
            {
                const auto nVisibleIndex = (nConditionIndex - nFirstVisibleCondition);
                if (nVisibleIndex == nNumVisibleConditions)
                    break;

                auto* vmCondition = m_vConditions.GetItemAt(nVisibleIndex);
                if (vmCondition == nullptr)
                {
                    // assume the trigger is being updated on another thread and we'll
                    // resynchronize the hit counts on the next frame
                    break;
                }

                vmCondition->SetCurrentHits(pCondition->current_hits);
            }

            ++nConditionIndex;
        }

        if (m_bHasHitChain)
            UpdateTotalHits(pGroup);
    }

    m_vConditions.EndUpdate();

    if (!m_vConditions.IsUpdating())
        m_vConditions.AddNotifyTarget(m_pConditionsMonitor);
}

void TriggerViewModel::UpdateMemrefs()
{
    if (m_pTrigger)
    {
        std::lock_guard<std::mutex> lock(m_pMutex);

        auto* memrefs = rc_trigger_get_memrefs(m_pTrigger);
        if (memrefs)
            rc_update_memref_values(memrefs, rc_peek_callback, nullptr);
    }
}

bool TriggerViewModel::BuildHitChainTooltip(std::wstring& sTooltip,
    const ViewModelCollection<TriggerConditionViewModel>& vmConditions, gsl::index nIndex)
{
    unsigned int nHits = 0, nConditionHits = 0;
    bool bIsHitsChain = false;
    sTooltip.clear();

    for (gsl::index nScan = 0; nScan < gsl::narrow_cast<gsl::index>(vmConditions.Count()); ++nScan)
    {
        const auto* vmCondition = vmConditions.GetItemAt(nScan);
        Expects(vmCondition != nullptr);

        switch (vmCondition->GetType())
        {
            case ra::ui::viewmodels::TriggerConditionType::AddHits:
                bIsHitsChain = true;
                nConditionHits = vmCondition->GetCurrentHits();
                if (nConditionHits != 0)
                {
                    sTooltip.append(ra::StringPrintf(L"+%u (condition %d)\n", nConditionHits, vmCondition->GetIndex()));
                    nHits += nConditionHits;
                }
                break;

            case ra::ui::viewmodels::TriggerConditionType::SubHits:
                bIsHitsChain = true;
                nConditionHits = vmCondition->GetCurrentHits();
                if (nConditionHits != 0)
                {
                    sTooltip.append(ra::StringPrintf(L"-%u (condition %d)\n", nConditionHits, vmCondition->GetIndex()));
                    nHits -= nConditionHits;
                }
                break;

            case ra::ui::viewmodels::TriggerConditionType::AddAddress:
            case ra::ui::viewmodels::TriggerConditionType::AddSource:
            case ra::ui::viewmodels::TriggerConditionType::SubSource:
            case ra::ui::viewmodels::TriggerConditionType::Remember:
            case ra::ui::viewmodels::TriggerConditionType::AndNext:
            case ra::ui::viewmodels::TriggerConditionType::OrNext:
            case ra::ui::viewmodels::TriggerConditionType::ResetNextIf:
                break;

            default:
                if (nScan >= nIndex)
                {
                    if (bIsHitsChain)
                    {
                        nConditionHits = vmCondition->GetCurrentHits();
                        if (nConditionHits != 0)
                        {
                            sTooltip.append(ra::StringPrintf(L"+%u (condition %d)\n", nConditionHits, vmCondition->GetIndex()));
                            nHits += nConditionHits;
                        }

                        sTooltip.append(ra::StringPrintf(L"%d/%u total", ra::to_signed(nHits), vmCondition->GetRequiredHits()));
                    }

                    return true;
                }

                bIsHitsChain = false;
                nHits = 0;
                sTooltip.clear();
                break;
        }
    }

    return false;
}

void TriggerViewModel::UpdateColors(const rc_trigger_t* pTrigger)
{
    UpdateGroupColors(pTrigger);
    UpdateConditionColors(pTrigger);
}

void TriggerViewModel::UpdateGroupColors(const rc_trigger_t* pTrigger)
{
    const auto nDefaultColor = ra::ui::Color(ra::to_unsigned(GroupViewModel::ColorProperty.GetDefaultValue()));

    if (pTrigger == nullptr)
    {
        // no trigger, or trigger not active, disable highlights
        for (gsl::index i = 0; i < gsl::narrow_cast<gsl::index>(m_vGroups.Count()); ++i)
        {
            auto* pGroup = m_vGroups.GetItemAt(i);
            if (pGroup != nullptr)
                pGroup->SetColor(nDefaultColor);
        }

        return;
    }

    const auto& pTheme = ra::services::ServiceLocator::Get<ra::ui::EditorTheme>();

    if (!pTrigger->has_hits)
    {
        // trigger has been reset, or is waiting
        for (gsl::index i = 0; i < gsl::narrow_cast<gsl::index>(m_vGroups.Count()); ++i)
        {
            auto* pGroup = m_vGroups.GetItemAt(i);
            if (pGroup == nullptr || pGroup->m_pConditionSet == nullptr)
                continue;

            bool bIsReset = false;
            if (pGroup->m_pConditionSet->num_reset_conditions > 0)
            {
                rc_condition_t* pCondition = pGroup->m_pConditionSet->conditions;
                for (; pCondition != nullptr; pCondition = pCondition->next)
                {
                    // if the second is_true bit is non-zero, the condition was responsible for resetting the trigger.
                    if (pCondition->type == RC_CONDITION_RESET_IF && (pCondition->is_true & 0x02))
                    {
                        bIsReset = true;
                        break;
                    }
                }
            }

            if (bIsReset)
                pGroup->SetColor(pTheme.ColorTriggerResetTrue());
            else
                pGroup->SetColor(nDefaultColor);
        }
    }
    else
    {
        // trigger is active
        for (gsl::index i = 0; i < gsl::narrow_cast<gsl::index>(m_vGroups.Count()); ++i)
        {
            auto* pGroup = m_vGroups.GetItemAt(i);
            if (pGroup == nullptr || pGroup->m_pConditionSet == nullptr)
                continue;

            if (pGroup->m_pConditionSet->is_paused)
            {
                pGroup->SetColor(pTheme.ColorTriggerPauseTrue());
            }
            else
            {
                bool bIsTrue = true;
                rc_condition_t* pCondition = pGroup->m_pConditionSet->conditions;
                for (; bIsTrue && pCondition != nullptr; pCondition = pCondition->next)
                {
                    // target hitcount met, ignore current truthiness
                    if (pCondition->required_hits != 0 && pCondition->current_hits == pCondition->required_hits)
                        continue;

                    // if it's a non-true logical condition, the group is not ready to trigger
                    if (!pCondition->is_true)
                    {
                        switch (pCondition->type)
                        {
                            case RC_CONDITION_MEASURED:
                            case RC_CONDITION_MEASURED_IF:
                            case RC_CONDITION_STANDARD:
                            case RC_CONDITION_TRIGGER:
                                bIsTrue = false;
                                break;

                            default:
                                break;
                        }
                    }
                }

                if (bIsTrue)
                    pGroup->SetColor(pTheme.ColorTriggerIsTrue());
                else
                    pGroup->SetColor(nDefaultColor);
            }
        }
    }
}

static bool CausedReset(const rc_condset_t* pCondSet) noexcept
{
    if (pCondSet->num_reset_conditions == 0)
        return false;

    GSL_SUPPRESS_TYPE3
    const rc_condition_t* pPauseConditions = rc_condset_get_conditions(const_cast<rc_condset_t*>(pCondSet));
    const rc_condition_t* pResetCondition = pPauseConditions + pCondSet->num_pause_conditions;
    const rc_condition_t* pStop = pResetCondition + pCondSet->num_reset_conditions;
    for (; pResetCondition < pStop; ++pResetCondition)
    {
        if (pResetCondition->type == RC_CONDITION_RESET_IF && (pResetCondition->is_true & 2) != 0)
            return true;
    }

    return false;
}

static bool WasTriggerReset(const rc_trigger_t* pTrigger) noexcept
{
    if (pTrigger->has_hits)
        return false;

    if (pTrigger->requirement && CausedReset(pTrigger->requirement))
        return true;

    for (const auto* pAlt = pTrigger->alternative; pAlt; pAlt = pAlt->next)
    {
        if (CausedReset(pAlt))
            return true;
    }

    return false;
}

static void UpdateTruthiness(rc_condset_t* pCondSet) noexcept
{
    rc_eval_state_t eval_state;
    memset(&eval_state, 0, sizeof(eval_state));
    eval_state.and_next = 1; /* this is important, we don't care about any of the other initial state */

    rc_condition_t* pPauseConditions = rc_condset_get_conditions(pCondSet);
    rc_condition_t* pResetConditions = pPauseConditions + pCondSet->num_pause_conditions;
    rc_condition_t* pOtherConditions = pResetConditions + pCondSet->num_reset_conditions;
    const uint32_t nNumConditions = pCondSet->num_hittarget_conditions +
        pCondSet->num_measured_conditions + pCondSet->num_other_conditions;

    rc_test_condset_internal(pOtherConditions, nNumConditions, &eval_state, 0);
    rc_reset_condset(pCondSet);
}

void TriggerViewModel::UpdateConditionColors(const rc_trigger_t* pTrigger)
{
    std::lock_guard<std::mutex> lock(m_pMutex);

    // update the condition rows for the currently selected group
    const auto nFirstCondition = GetValue(ScrollOffsetProperty);
    const auto nVisibleConditions = GetValue(VisibleItemCountProperty);

    gsl::index nConditionIndex = 0;
    if (pTrigger)
    {
        auto* pSelectedGroup = m_vGroups.GetItemAt(GetSelectedGroupIndex());
        if (pSelectedGroup && pSelectedGroup->m_pConditionSet)
        {
            if (pSelectedGroup->m_pConditionSet->is_paused)
            {
                // when a condset is paused, processing stops when the first pause condition is true. only highlight it
                bool bFirstPause = true;
                const rc_condition_t* pPauseConditions = rc_condset_get_conditions(pSelectedGroup->m_pConditionSet);
                const rc_condition_t* pEndPauseConditions = pPauseConditions + pSelectedGroup->m_pConditionSet->num_pause_conditions;

                rc_condition_t* pCondition = pSelectedGroup->m_pConditionSet->conditions;
                for (; pCondition != nullptr; pCondition = pCondition->next, ++nConditionIndex)
                {
                    if (nConditionIndex < nFirstCondition)
                        continue;
                    if (nConditionIndex - nFirstCondition >= nVisibleConditions)
                        break;

                    auto* vmCondition = m_vConditions.GetItemAt(nConditionIndex - nFirstCondition);
                    if (vmCondition != nullptr)
                    {
                        if (pCondition < pEndPauseConditions && pCondition >= pPauseConditions && bFirstPause)
                        {
                            vmCondition->UpdateRowColor(pCondition);

                            // processing stops when a PauseIf has met its hit target
                            if (pCondition->type == RC_CONDITION_PAUSE_IF && pCondition->is_true)
                            {
                                if (pCondition->required_hits == 0 || pCondition->current_hits == pCondition->required_hits)
                                    bFirstPause = false;
                            }
                        }
                        else
                        {
                            vmCondition->UpdateRowColor(nullptr);
                        }
                    }
                }
            }
            else
            {
                // if a ResetIf is true, some of the conditions may not have been evaluated
                // this frame. force evaluation and explicitly reset again.
                if (WasTriggerReset(pTrigger))
                    UpdateTruthiness(pSelectedGroup->m_pConditionSet);

                rc_condition_t* pCondition = pSelectedGroup->m_pConditionSet->conditions;
                for (; pCondition != nullptr; pCondition = pCondition->next, ++nConditionIndex)
                {
                    if (nConditionIndex < nFirstCondition)
                        continue;
                    if (nConditionIndex - nFirstCondition >= nVisibleConditions)
                        break;

                    auto* vmCondition = m_vConditions.GetItemAt(nConditionIndex - nFirstCondition);
                    if (vmCondition != nullptr)
                        vmCondition->UpdateRowColor(pCondition);
                }
            }
        }
    }

    for (; nConditionIndex < gsl::narrow_cast<gsl::index>(nFirstCondition) + nVisibleConditions; ++nConditionIndex)
    {
        auto* pCondition = m_vConditions.GetItemAt(nConditionIndex - nFirstCondition);
        if (pCondition != nullptr)
            pCondition->UpdateRowColor(nullptr);
    }
}

} // namespace viewmodels
} // namespace ui
} // namespace ra
