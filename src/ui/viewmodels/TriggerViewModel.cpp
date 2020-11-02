#include "TriggerViewModel.hh"

#include "rcheevos.h"
#include "RA_StringUtils.h"

#include "services\IClipboard.hh"
#include "services\ServiceLocator.hh"

namespace ra {
namespace ui {
namespace viewmodels {

const IntModelProperty TriggerViewModel::SelectedGroupIndexProperty("TriggerViewModel", "SelectedGroupIndex", -1);
const BoolModelProperty TriggerViewModel::PauseOnResetProperty("TriggerViewModel", "PauseOnReset", false);
const BoolModelProperty TriggerViewModel::PauseOnTriggerProperty("TriggerViewModel", "PauseOnTrigger", false);
const IntModelProperty TriggerViewModel::VersionProperty("TriggerViewModel", "Version", 0);

TriggerViewModel::TriggerViewModel() noexcept
    : m_pConditionsMonitor(*this)
{
    m_vConditionTypes.Add(ra::etoi(TriggerConditionType::Standard), L"");
    m_vConditionTypes.Add(ra::etoi(TriggerConditionType::PauseIf), L"Pause If");
    m_vConditionTypes.Add(ra::etoi(TriggerConditionType::ResetIf), L"Reset If");
    m_vConditionTypes.Add(ra::etoi(TriggerConditionType::AddSource), L"Add Source");
    m_vConditionTypes.Add(ra::etoi(TriggerConditionType::SubSource), L"Sub Source");
    m_vConditionTypes.Add(ra::etoi(TriggerConditionType::AddHits), L"Add Hits");
    m_vConditionTypes.Add(ra::etoi(TriggerConditionType::AddAddress), L"Add Address");
    m_vConditionTypes.Add(ra::etoi(TriggerConditionType::AndNext), L"And Next");
    m_vConditionTypes.Add(ra::etoi(TriggerConditionType::OrNext), L"Or Next");
    m_vConditionTypes.Add(ra::etoi(TriggerConditionType::Measured), L"Measured");
    m_vConditionTypes.Add(ra::etoi(TriggerConditionType::MeasuredIf), L"Measured If");
    m_vConditionTypes.Add(ra::etoi(TriggerConditionType::Trigger), L"Trigger");

    m_vOperandTypes.Add(ra::etoi(TriggerOperandType::Address), L"Mem");
    m_vOperandTypes.Add(ra::etoi(TriggerOperandType::Value), L"Value");
    m_vOperandTypes.Add(ra::etoi(TriggerOperandType::Delta), L"Delta");
    m_vOperandTypes.Add(ra::etoi(TriggerOperandType::Prior), L"Prior");
    m_vOperandTypes.Add(ra::etoi(TriggerOperandType::BCD), L"BCD");

    m_vOperandSizes.Add(ra::etoi(MemSize::Bit_0), L"Bit0");
    m_vOperandSizes.Add(ra::etoi(MemSize::Bit_1), L"Bit1");
    m_vOperandSizes.Add(ra::etoi(MemSize::Bit_2), L"Bit2");
    m_vOperandSizes.Add(ra::etoi(MemSize::Bit_3), L"Bit3");
    m_vOperandSizes.Add(ra::etoi(MemSize::Bit_4), L"Bit4");
    m_vOperandSizes.Add(ra::etoi(MemSize::Bit_5), L"Bit5");
    m_vOperandSizes.Add(ra::etoi(MemSize::Bit_6), L"Bit6");
    m_vOperandSizes.Add(ra::etoi(MemSize::Bit_7), L"Bit7");
    m_vOperandSizes.Add(ra::etoi(MemSize::Nibble_Lower), L"Lower4");
    m_vOperandSizes.Add(ra::etoi(MemSize::Nibble_Upper), L"Upper4");
    m_vOperandSizes.Add(ra::etoi(MemSize::EightBit), L"8-bit");
    m_vOperandSizes.Add(ra::etoi(MemSize::SixteenBit), L"16-bit");
    m_vOperandSizes.Add(ra::etoi(MemSize::TwentyFourBit), L"24-bit");
    m_vOperandSizes.Add(ra::etoi(MemSize::ThirtyTwoBit), L"32-bit");
    m_vOperandSizes.Add(ra::etoi(MemSize::BitCount), L"BitCount");

    m_vOperatorTypes.Add(ra::etoi(TriggerOperatorType::Equals), L"=");
    m_vOperatorTypes.Add(ra::etoi(TriggerOperatorType::LessThan), L"<");
    m_vOperatorTypes.Add(ra::etoi(TriggerOperatorType::LessThanOrEqual), L"<=");
    m_vOperatorTypes.Add(ra::etoi(TriggerOperatorType::GreaterThan), L">");
    m_vOperatorTypes.Add(ra::etoi(TriggerOperatorType::GreaterThanOrEqual), L">=");
    m_vOperatorTypes.Add(ra::etoi(TriggerOperatorType::NotEquals), L"!=");
    m_vOperatorTypes.Add(ra::etoi(TriggerOperatorType::None), L"");
    m_vOperatorTypes.Add(ra::etoi(TriggerOperatorType::Multiply), L"*");
    m_vOperatorTypes.Add(ra::etoi(TriggerOperatorType::Divide), L"/");
    m_vOperatorTypes.Add(ra::etoi(TriggerOperatorType::BitwiseAnd), L"&");
}

std::string TriggerViewModel::Serialize() const
{
    std::string buffer;
    SerializeAppend(buffer);
    return buffer;
}

bool TriggerViewModel::GroupViewModel::UpdateSerialized(const ViewModelCollection<TriggerConditionViewModel>& vConditions)
{
    std::string sSerialized;
    UpdateSerialized(sSerialized, vConditions);

    if (sSerialized != m_sSerialized)
    {
        m_sSerialized.swap(sSerialized);
        return true;
    }

    return false;
}

void TriggerViewModel::GroupViewModel::UpdateSerialized(std::string& sSerialized, const ViewModelCollection<TriggerConditionViewModel>& vConditions)
{
    for (gsl::index nIndex = 0; nIndex < gsl::narrow_cast<gsl::index>(vConditions.Count()); ++nIndex)
    {
        const auto* vmCondition = vConditions.GetItemAt(nIndex);
        if (vmCondition != nullptr)
        {
            vmCondition->SerializeAppend(sSerialized);
            sSerialized.push_back('_');
        }
    }

    if (!sSerialized.empty() && sSerialized.back() == '_')
        sSerialized.pop_back();
}

const std::string& TriggerViewModel::GroupViewModel::GetSerialized() const
{
    if (m_sSerialized.empty() && m_pConditionSet)
    {
        ViewModelCollection<TriggerConditionViewModel> vConditions;
        rc_condition_t* pCondition = m_pConditionSet->conditions;
        for (; pCondition != nullptr; pCondition = pCondition->next)
        {
            auto& vmCondition = vConditions.Add();
            vmCondition.InitializeFrom(*pCondition);
        }

        UpdateSerialized(m_sSerialized, vConditions);
    }

    return m_sSerialized;
}

void TriggerViewModel::SerializeAppend(std::string& sBuffer) const
{
    for (gsl::index nGroup = 0; nGroup < gsl::narrow_cast<gsl::index>(m_vGroups.Count()); ++nGroup)
    {
        const auto* vmGroup = m_vGroups.GetItemAt(nGroup);
        if (vmGroup != nullptr)
        {
            if (nGroup != 0)
                sBuffer.push_back('S');

            sBuffer.append(vmGroup->GetSerialized());
        }
    }
}

void TriggerViewModel::CopySelectedConditionsToClipboard()
{
    std::string sSerialized;

    for (gsl::index nIndex = 0; nIndex < gsl::narrow_cast<gsl::index>(m_vConditions.Count()); ++nIndex)
    {
        const auto* vmCondition = m_vConditions.GetItemAt(nIndex);
        if (vmCondition != nullptr && vmCondition->IsSelected())
        {
            vmCondition->SerializeAppend(sSerialized);
            sSerialized.push_back('_');
        }
    }

    if (sSerialized.empty())
    {
        // no selection, get the whole group
        const auto* pGroup = m_vGroups.GetItemAt(GetSelectedGroupIndex());
        if (pGroup != nullptr)
            sSerialized = pGroup->GetSerialized();
    }
    else if (sSerialized.back() == '_')
    {
        // remove trailing joiner
        sSerialized.pop_back();
    }

    ra::services::ServiceLocator::Get<ra::services::IClipboard>().SetText(ra::Widen(sSerialized));
}

static void AddAltGroup(ViewModelCollection<TriggerViewModel::GroupViewModel>& vGroups,
    int nId, rc_condset_t* pConditionSet)
{
    auto& vmAltGroup = vGroups.Add();
    vmAltGroup.SetId(++nId);
    vmAltGroup.SetLabel(ra::StringPrintf(L"Alt %d", nId));
    vmAltGroup.m_pConditionSet = pConditionSet;
}

void TriggerViewModel::InitializeFrom(const rc_trigger_t& pTrigger)
{
    m_vGroups.RemoveNotifyTarget(*this);
    m_vGroups.BeginUpdate();

    // this will not update the conditions collection because OnValueChange ignores it when m_vGroups.IsUpdating
    SetSelectedGroupIndex(0);

    m_vGroups.Clear();

    auto& vmCoreGroup = m_vGroups.Add();
    vmCoreGroup.SetId(0);
    vmCoreGroup.SetLabel(L"Core");
    vmCoreGroup.SetSelected(true);
    if (pTrigger.requirement)
        vmCoreGroup.m_pConditionSet = pTrigger.requirement;

    int nId = 0;
    rc_condset_t* pConditionSet = pTrigger.alternative;
    for (; pConditionSet != nullptr; pConditionSet = pConditionSet->next)
        AddAltGroup(m_vGroups, ++nId, pConditionSet);

    m_vGroups.EndUpdate();
    m_vGroups.AddNotifyTarget(*this);

    // forcibly update the conditions from the group that was selected earlier
    UpdateConditions(&vmCoreGroup);
}

void TriggerViewModel::InitializeFrom(const std::string& sTrigger)
{
    const auto nSize = rc_trigger_size(sTrigger.c_str());
    if (nSize > 0)
    {
        m_sTriggerBuffer.resize(nSize);
        m_pTrigger = rc_parse_trigger(m_sTriggerBuffer.data(), sTrigger.c_str(), nullptr, 0);
        InitializeFrom(*m_pTrigger);
    }
    else
    {
        rc_trigger_t pTrigger;
        memset(&pTrigger, 0, sizeof(pTrigger));
        InitializeFrom(pTrigger);
    }
}

void TriggerViewModel::UpdateFrom(const rc_trigger_t& pTrigger)
{
    const int nSelectedIndex = GetSelectedGroupIndex();

    m_vGroups.RemoveNotifyTarget(*this);
    m_vGroups.BeginUpdate();

    // update core group
    auto* vmCoreGroup = m_vGroups.GetItemAt(0);
    Expects(vmCoreGroup != nullptr);
    vmCoreGroup->m_pConditionSet = pTrigger.requirement;

    // update alt groups (add any new alt groups)
    gsl::index nIndex = 0;
    rc_condset_t* pConditionSet = pTrigger.alternative;
    for (; pConditionSet != nullptr; pConditionSet = pConditionSet->next)
    {
        auto* vmAltGroup = m_vGroups.GetItemAt(++nIndex);
        if (vmAltGroup != nullptr)
            vmAltGroup->m_pConditionSet = pConditionSet;
        else
            AddAltGroup(m_vGroups, gsl::narrow_cast<int>(nIndex), pConditionSet);
    }

    // remove any extra alt groups
    ++nIndex;
    while (m_vGroups.Count() > ra::to_unsigned(nIndex))
        m_vGroups.RemoveAt(m_vGroups.Count() - 1);

    m_vGroups.EndUpdate();
    m_vGroups.AddNotifyTarget(*this);

    // forcibly update the conditions from the selected group
    UpdateConditions(m_vGroups.GetItemAt(nSelectedIndex));
}

void TriggerViewModel::UpdateFrom(const std::string& sTrigger)
{
    const auto nSize = rc_trigger_size(sTrigger.c_str());
    if (nSize > 0)
    {
        m_sTriggerBuffer.resize(nSize);
        m_pTrigger = rc_parse_trigger(m_sTriggerBuffer.data(), sTrigger.c_str(), nullptr, 0);
        UpdateFrom(*m_pTrigger);
    }
    else
    {
        rc_trigger_t pTrigger;
        memset(&pTrigger, 0, sizeof(pTrigger));
        UpdateFrom(pTrigger);
    }
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
    if (args.Property != TriggerConditionViewModel::CurrentHitsProperty)
        UpdateCurrentGroup();
}

void TriggerViewModel::ConditionsMonitor::OnViewModelAdded(gsl::index)
{
    UpdateCurrentGroup();
}

void TriggerViewModel::ConditionsMonitor::OnViewModelRemoved(gsl::index)
{
    UpdateCurrentGroup();
}

void TriggerViewModel::ConditionsMonitor::UpdateCurrentGroup()
{
    auto* pGroup = m_vmTrigger->m_vGroups.GetItemAt(m_vmTrigger->GetSelectedGroupIndex());
    if (pGroup)
    {
        if (pGroup->UpdateSerialized(m_vmTrigger->m_vConditions))
            m_vmTrigger->UpdateVersion();
    }
}

void TriggerViewModel::UpdateVersion()
{
    SetValue(VersionProperty, GetValue(VersionProperty) + 1);
}

void TriggerViewModel::OnValueChanged(const IntModelProperty::ChangeArgs& args)
{
    if (args.Property == SelectedGroupIndexProperty)
    {
        // ignore change caused in InitializeFrom
        if (!m_vGroups.IsUpdating())
        {
            // disable notifications while updating the selected group to prevent re-entrancy
            m_vGroups.RemoveNotifyTarget(*this);

            auto* pGroup = m_vGroups.GetItemAt(args.tNewValue);
            if (pGroup != nullptr) // may be null if out of range (i.e. -1)
                pGroup->SetSelected(true);

            if (args.tOldValue != -1)
            {
                auto* pOldGroup = m_vGroups.GetItemAt(args.tOldValue);
                Expects(pOldGroup != nullptr);
                pOldGroup->SetSelected(false);
            }

            m_vGroups.AddNotifyTarget(*this);

            UpdateConditions(pGroup);
        }
    }

    ViewModelBase::OnValueChanged(args);
}

void TriggerViewModel::UpdateConditions(const GroupViewModel* pGroup)
{
    m_vConditions.RemoveNotifyTarget(m_pConditionsMonitor);
    m_vConditions.BeginUpdate();

    m_vConditions.Clear();

    if (pGroup != nullptr && pGroup->m_pConditionSet)
    {
        rc_condition_t* pCondition = pGroup->m_pConditionSet->conditions;
        for (; pCondition != nullptr; pCondition = pCondition->next)
        {
            auto& vmCondition = m_vConditions.Add();
            vmCondition.SetIndex(gsl::narrow_cast<int>(m_vConditions.Count()));
            vmCondition.InitializeFrom(*pCondition);
        }
    }

    m_vConditions.EndUpdate();
    m_vConditions.AddNotifyTarget(m_pConditionsMonitor);
}

void TriggerViewModel::DoFrame()
{
    auto* pGroup = m_vGroups.GetItemAt(GetSelectedGroupIndex());
    if (pGroup != nullptr)
    {
        m_vConditions.BeginUpdate();

        gsl::index nConditionIndex = 0;
        rc_condition_t* pCondition = pGroup->m_pConditionSet->conditions;
        for (; pCondition != nullptr; pCondition = pCondition->next)
        {
            auto* vmCondition = m_vConditions.GetItemAt(nConditionIndex++);
            Expects(vmCondition != nullptr);

            vmCondition->SetCurrentHits(pCondition->current_hits);
        }

        m_vConditions.EndUpdate();
    }
}

} // namespace viewmodels
} // namespace ui
} // namespace ra
