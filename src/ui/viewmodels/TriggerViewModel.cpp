#include "TriggerViewModel.hh"

#include "rcheevos.h"
#include "RA_StringUtils.h"

#include "services\IClipboard.hh"
#include "services\ServiceLocator.hh"

#include "ui\viewmodels\MessageBoxViewModel.hh"
#include "ui\viewmodels\WindowManager.hh"

namespace ra {
namespace ui {
namespace viewmodels {

const IntModelProperty TriggerViewModel::SelectedGroupIndexProperty("TriggerViewModel", "SelectedGroupIndex", -1);
const BoolModelProperty TriggerViewModel::PauseOnResetProperty("TriggerViewModel", "PauseOnReset", false);
const BoolModelProperty TriggerViewModel::PauseOnTriggerProperty("TriggerViewModel", "PauseOnTrigger", false);
const IntModelProperty TriggerViewModel::VersionProperty("TriggerViewModel", "Version", 0);
const IntModelProperty TriggerViewModel::EnsureVisibleConditionIndexProperty("TriggerViewModel", "EnsureVisibleConditionIndex", -1);
const IntModelProperty TriggerViewModel::EnsureVisibleGroupIndexProperty("TriggerViewModel", "EnsureVisibleGroupIndex", -1);

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

void TriggerViewModel::DeselectAllConditions()
{
    for (gsl::index nIndex = 0; nIndex < ra::to_signed(m_vConditions.Count()); ++nIndex)
        m_vConditions.SetItemValue(nIndex, TriggerConditionViewModel::IsSelectedProperty, false);
}

void TriggerViewModel::PasteFromClipboard()
{
    const std::wstring sClipboardText = ra::services::ServiceLocator::Get<ra::services::IClipboard>().GetText();
    if (sClipboardText.empty())
    {
        ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(L"Paste failed.", L"Clipboard did not contain any text.");
        return;
    }

    std::string sTrigger = ra::Narrow(sClipboardText);
    const auto nSize = rc_trigger_size(sTrigger.c_str());
    if (nSize < 0)
    {
        ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(L"Paste failed.", L"Clipboard did not contain valid trigger conditions.");
        return;
    }

    std::string sTriggerBuffer;
    sTriggerBuffer.resize(nSize);
    const rc_trigger_t* pTrigger = rc_parse_trigger(sTriggerBuffer.data(), sTrigger.c_str(), nullptr, 0);
    if (pTrigger->alternative)
    {
        ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(L"Paste failed.", L"Clipboard contained multiple groups.");
        return;
    }

    m_vConditions.BeginUpdate();

    DeselectAllConditions();

    for (const rc_condition_t* pCondition = pTrigger->requirement->conditions;
        pCondition != nullptr; pCondition = pCondition->next)
    {
        auto& vmCondition = m_vConditions.Add();
        vmCondition.InitializeFrom(*pCondition);
        vmCondition.SetSelected(true);
        vmCondition.SetIndex(gsl::narrow_cast<int>(m_vConditions.Count()));
    }

    m_vConditions.EndUpdate();

    SetValue(EnsureVisibleConditionIndexProperty, gsl::narrow_cast<int>(m_vConditions.Count()) - 1);
}

void TriggerViewModel::RemoveSelectedConditions()
{
    int nSelected = 0;

    for (gsl::index nIndex = 0; nIndex < gsl::narrow_cast<gsl::index>(m_vConditions.Count()); ++nIndex)
    {
        const auto* vmCondition = m_vConditions.GetItemAt(nIndex);
        if (vmCondition != nullptr && vmCondition->IsSelected())
            ++nSelected;
    }

    if (nSelected == 0)
        return;

    ra::ui::viewmodels::MessageBoxViewModel vmMessageBox;
    vmMessageBox.SetMessage(ra::StringPrintf(L"Are you sure that you want to delete %d %s?",
        nSelected, (nSelected == 1) ? "condition" : "conditions"));
    vmMessageBox.SetButtons(ra::ui::viewmodels::MessageBoxViewModel::Buttons::YesNo);
    vmMessageBox.SetIcon(ra::ui::viewmodels::MessageBoxViewModel::Icon::Warning);

    if (vmMessageBox.ShowModal() != ra::ui::DialogResult::Yes)
        return;

    m_vConditions.BeginUpdate();

    for (gsl::index nIndex = gsl::narrow_cast<gsl::index>(m_vConditions.Count()) - 1; nIndex >= 0; --nIndex)
    {
        const auto* vmCondition = m_vConditions.GetItemAt(nIndex);
        if (vmCondition != nullptr && vmCondition->IsSelected())
            m_vConditions.RemoveAt(nIndex);
    }

    m_vConditions.EndUpdate();

    // update indices after EndUpdate to address a synchronization issue with the GridBinding
    for (gsl::index nIndex = 0; nIndex < gsl::narrow_cast<gsl::index>(m_vConditions.Count()); ++nIndex)
    {
        auto* vmCondition = m_vConditions.GetItemAt(nIndex);
        if (vmCondition != nullptr)
            vmCondition->SetIndex(gsl::narrow_cast<int>(nIndex) + 1);
    }
}

void TriggerViewModel::MoveSelectedConditionsUp()
{
    m_vConditions.ShiftItemsUp(TriggerConditionViewModel::IsSelectedProperty);

    UpdateIndicesAndEnsureSelectionVisible();
}

void TriggerViewModel::MoveSelectedConditionsDown()
{
    m_vConditions.ShiftItemsDown(TriggerConditionViewModel::IsSelectedProperty);

    UpdateIndicesAndEnsureSelectionVisible();
}

void TriggerViewModel::UpdateIndicesAndEnsureSelectionVisible()
{
    bool bFoundFirst = false;
    for (gsl::index nIndex = 0; nIndex < gsl::narrow_cast<gsl::index>(m_vConditions.Count()); ++nIndex)
    {
        m_vConditions.SetItemValue(nIndex, TriggerConditionViewModel::IndexProperty, gsl::narrow_cast<int>(nIndex) + 1);

        if (!bFoundFirst && m_vConditions.GetItemValue(nIndex, TriggerConditionViewModel::IsSelectedProperty))
        {
            SetValue(EnsureVisibleConditionIndexProperty, gsl::narrow_cast<int>(nIndex));
            bFoundFirst = true;
        }
    }
}

void TriggerViewModel::NewCondition()
{
    m_vConditions.BeginUpdate();

    DeselectAllConditions();

    auto& vmCondition = m_vConditions.Add();
    vmCondition.SetIndex(gsl::narrow_cast<int>(m_vConditions.Count()));

    // assume the user wants to create a condition for the currently watched memory address.
    const auto& pMemoryInspector = ra::services::ServiceLocator::Get<ra::ui::viewmodels::WindowManager>().MemoryInspector;
    const auto nAddress = pMemoryInspector.Viewer().GetAddress();
    MemSize nSize = MemSize::EightBit;

    // if the code note specifies an explicit size, use it. otherwise, use the selected viewer mode size.
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
    switch (pGameContext.FindCodeNoteSize(nAddress))
    {
        default: // no code note, or code note doesn't specify a size
            nSize = pMemoryInspector.Viewer().GetSize();
            break;

        case 2:
            nSize = MemSize::SixteenBit;
            break;

        case 3:
            nSize = MemSize::TwentyFourBit;
            break;

        case 4:
            nSize = MemSize::ThirtyTwoBit;
            break;
    }

    vmCondition.SetSourceType(TriggerOperandType::Address);
    vmCondition.SetSourceSize(nSize);
    vmCondition.SetSourceValue(nAddress);

    // assume the user wants to compare to the current value of the watched memory address
    vmCondition.SetOperator(TriggerOperatorType::Equals);

    const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::context::EmulatorContext>();
    vmCondition.SetTargetSize(nSize);
    vmCondition.SetTargetType(TriggerOperandType::Value);
    vmCondition.SetTargetValue(pEmulatorContext.ReadMemory(nAddress, nSize));

    vmCondition.SetSelected(true);

    m_vConditions.EndUpdate();

    SetValue(EnsureVisibleConditionIndexProperty, gsl::narrow_cast<int>(m_vConditions.Count()) - 1);
}

static void AddAltGroup(ViewModelCollection<TriggerViewModel::GroupViewModel>& vGroups,
    int nId, rc_condset_t* pConditionSet)
{
    auto& vmAltGroup = vGroups.Add();
    vmAltGroup.SetId(nId);
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

void TriggerViewModel::InitializeFrom(const std::string& sTrigger, const ra::data::models::CapturedTriggerHits& pCapturedHits)
{
    const auto nSize = rc_trigger_size(sTrigger.c_str());
    if (nSize > 0)
    {
        m_sTriggerBuffer.resize(nSize);
        m_pTrigger = rc_parse_trigger(m_sTriggerBuffer.data(), sTrigger.c_str(), nullptr, 0);
        pCapturedHits.Restore(m_pTrigger, sTrigger);
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

    int nIndex = 0;

    if (pGroup != nullptr && pGroup->m_pConditionSet)
    {
        rc_condition_t* pCondition = pGroup->m_pConditionSet->conditions;
        for (; pCondition != nullptr; pCondition = pCondition->next)
        {
            auto* vmCondition = m_vConditions.GetItemAt(gsl::narrow_cast<gsl::index>(nIndex));
            if (vmCondition == nullptr)
            {
                vmCondition = &m_vConditions.Add();
                Ensures(vmCondition != nullptr);
            }

            vmCondition->SetIndex(++nIndex);
            vmCondition->InitializeFrom(*pCondition);
        }
    }

    for (gsl::index nScan = m_vConditions.Count() - 1; nScan >= nIndex; --nScan)
        m_vConditions.RemoveAt(nScan);

    m_vConditions.EndUpdate();
    m_vConditions.AddNotifyTarget(m_pConditionsMonitor);
}

void TriggerViewModel::AddGroup()
{
    const int nGroups = gsl::narrow_cast<int>(m_vGroups.Count());

    if (nGroups == 1)
    {
        // only a core group, add two alt groups
        AddAltGroup(m_vGroups, nGroups, nullptr);
        AddAltGroup(m_vGroups, nGroups + 1, nullptr);
    }
    else
    {
        // already at least one alt group, only add one more
        AddAltGroup(m_vGroups, nGroups, nullptr);
    }

    SetSelectedGroupIndex(nGroups);
    SetValue(EnsureVisibleGroupIndexProperty, nGroups);

    UpdateVersion();
}

void TriggerViewModel::RemoveGroup()
{
    int nIndex = GetSelectedGroupIndex();
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
