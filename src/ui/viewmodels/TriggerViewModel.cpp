#include "TriggerViewModel.hh"

#include "rcheevos.h"
#include "RA_StringUtils.h"

namespace ra {
namespace ui {
namespace viewmodels {

const IntModelProperty TriggerViewModel::SelectedGroupIndexProperty("TriggerViewModel", "SelectedGroupIndex", -1);
const BoolModelProperty TriggerViewModel::PauseOnResetProperty("TriggerViewModel", "PauseOnReset", false);
const BoolModelProperty TriggerViewModel::PauseOnTriggerProperty("TriggerViewModel", "PauseOnTrigger", false);
const IntModelProperty TriggerViewModel::VersionProperty("TriggerViewModel", "Version", 0);
const IntModelProperty TriggerViewModel::GroupViewModel::VersionProperty("GroupViewModel", "Version", 0);

std::string TriggerViewModel::GroupViewModel::Serialize() const
{
    std::string buffer;
    SerializeAppend(buffer);
    return buffer;
}

void TriggerViewModel::GroupViewModel::SerializeAppend(std::string& sBuffer) const
{
    for (gsl::index nIndex = 0; nIndex < gsl::narrow_cast<gsl::index>(m_vConditions.Count()); ++nIndex)
    {
        const auto* vmCondition = m_vConditions.GetItemAt(nIndex);
        if (vmCondition != nullptr)
        {
            vmCondition->SerializeAppend(sBuffer);
            sBuffer.push_back('_');
        }
    }

    if (!sBuffer.empty() && sBuffer.back() == '_')
        sBuffer.pop_back();
}

void TriggerViewModel::GroupViewModel::InitializeFrom(const rc_condset_t& pConditionSet)
{
    m_vConditions.RemoveNotifyTarget(*this);

    rc_condition_t* pCondition = pConditionSet.conditions;
    for (; pCondition != nullptr; pCondition = pCondition->next)
    {
        auto& vmCondition = m_vConditions.Add();
        vmCondition.SetIndex(gsl::narrow_cast<int>(m_vConditions.Count()));
        vmCondition.InitializeFrom(*pCondition);
    }

    m_vConditions.AddNotifyTarget(*this);
}

void TriggerViewModel::GroupViewModel::OnViewModelIntValueChanged(gsl::index, const IntModelProperty::ChangeArgs&)
{
    UpdateVersion();
}

void TriggerViewModel::GroupViewModel::OnViewModelAdded(gsl::index)
{
    UpdateVersion();
}

void TriggerViewModel::GroupViewModel::OnViewModelRemoved(gsl::index)
{
    UpdateVersion();
}

void TriggerViewModel::GroupViewModel::UpdateVersion()
{
    SetValue(VersionProperty, GetValue(VersionProperty) + 1);
}

std::string TriggerViewModel::Serialize() const
{
    std::string buffer;
    SerializeAppend(buffer);
    return buffer;
}

void TriggerViewModel::SerializeAppend(std::string& sBuffer) const
{
    for (gsl::index nGroup = 0; nGroup < gsl::narrow_cast<gsl::index>(m_vGroups.Count()); ++nGroup)
    {
        const auto* vmGroup = m_vGroups.GetItemAt(nGroup);
        if (vmGroup != nullptr && vmGroup->Conditions().Count() != 0)
        {
            if (nGroup != 0)
                sBuffer.push_back('S');

            vmGroup->SerializeAppend(sBuffer);
        }
    }
}

void TriggerViewModel::InitializeFrom(const rc_trigger_t& pTrigger)
{
    m_vGroups.RemoveNotifyTarget(*this);

    auto& vmCoreGroup = m_vGroups.Add();
    vmCoreGroup.SetId(0);
    vmCoreGroup.SetLabel(L"Core");
    if (pTrigger.requirement)
        vmCoreGroup.InitializeFrom(*pTrigger.requirement);

    const rc_condset_t* pConditionSet = pTrigger.alternative;
    for (; pConditionSet != nullptr; pConditionSet = pConditionSet->next)
    {
        auto& vmAltGroup = m_vGroups.Add();
        const auto nId = gsl::narrow_cast<int>(m_vGroups.Count());
        vmAltGroup.SetId(nId);
        vmAltGroup.SetLabel(ra::StringPrintf(L"Alt %d", nId));
        vmAltGroup.InitializeFrom(*pConditionSet);
    }

    m_vGroups.AddNotifyTarget(*this);
}

void TriggerViewModel::OnViewModelIntValueChanged(gsl::index, const IntModelProperty::ChangeArgs& args)
{
    if (args.Property == GroupViewModel::VersionProperty)
        UpdateVersion();
}

void TriggerViewModel::OnViewModelAdded(gsl::index)
{
    UpdateVersion();
}

void TriggerViewModel::OnViewModelRemoved(gsl::index)
{
    UpdateVersion();
}

void TriggerViewModel::UpdateVersion()
{
    SetValue(VersionProperty, GetValue(VersionProperty) + 1);
}

} // namespace viewmodels
} // namespace ui
} // namespace ra
