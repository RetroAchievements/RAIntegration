#include "TriggerViewModel.hh"

#include <rcheevos.h>

#include "RA_StringUtils.h"

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
const IntModelProperty TriggerViewModel::EnsureVisibleConditionIndexProperty("TriggerViewModel", "EnsureVisibleConditionIndex", -1);
const IntModelProperty TriggerViewModel::EnsureVisibleGroupIndexProperty("TriggerViewModel", "EnsureVisibleGroupIndex", -1);
const BoolModelProperty TriggerViewModel::IsMeasuredTrackedAsPercentProperty("TriggerViewModel", "IsMeasuredTrackedAsPercent", false);
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

    m_vOperandTypes.Add(ra::etoi(TriggerOperandType::Address), L"Mem");
    m_vOperandTypes.Add(ra::etoi(TriggerOperandType::Value), L"Value");
    m_vOperandTypes.Add(ra::etoi(TriggerOperandType::Delta), L"Delta");
    m_vOperandTypes.Add(ra::etoi(TriggerOperandType::Prior), L"Prior");
    m_vOperandTypes.Add(ra::etoi(TriggerOperandType::BCD), L"BCD");
    m_vOperandTypes.Add(ra::etoi(TriggerOperandType::Float), L"Float");
    m_vOperandTypes.Add(ra::etoi(TriggerOperandType::Inverted), L"Invert");

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
    m_vOperandSizes.Add(ra::etoi(MemSize::SixteenBitBigEndian), L"16-bit BE");
    m_vOperandSizes.Add(ra::etoi(MemSize::TwentyFourBitBigEndian), L"24-bit BE");
    m_vOperandSizes.Add(ra::etoi(MemSize::ThirtyTwoBitBigEndian), L"32-bit BE");
    m_vOperandSizes.Add(ra::etoi(MemSize::BitCount), L"BitCount");
    m_vOperandSizes.Add(ra::etoi(MemSize::Float), L"Float");
    m_vOperandSizes.Add(ra::etoi(MemSize::MBF32), L"MBF32");

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
        m_pConditionSet = nullptr;
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

const std::string& TriggerViewModel::GroupViewModel::GetSerialized(const TriggerViewModel& vmTrigger) const
{
    if (m_sSerialized == TriggerViewModel::GroupViewModel::NOT_SERIALIZED)
    {
        m_sSerialized.clear();

        if (m_pConditionSet)
        {
            ViewModelCollection<TriggerConditionViewModel> vConditions;
            rc_condition_t* pCondition = m_pConditionSet->conditions;
            for (; pCondition != nullptr; pCondition = pCondition->next)
            {
                auto& vmCondition = vConditions.Add();
                vmCondition.SetTriggerViewModel(&vmTrigger);
                vmCondition.InitializeFrom(*pCondition);
            }

            UpdateSerialized(m_sSerialized, vConditions);
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

            sBuffer.append(vmGroup->GetSerialized(*this));
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
            sSerialized = pGroup->GetSerialized(*this);
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
        vmCondition.SetTriggerViewModel(this);
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
    vmCondition.SetTriggerViewModel(this);
    vmCondition.SetIndex(gsl::narrow_cast<int>(m_vConditions.Count()));

    // assume the user wants to create a condition for the currently watched memory address.
    const auto& pMemoryInspector = ra::services::ServiceLocator::Get<ra::ui::viewmodels::WindowManager>().MemoryInspector;
    const auto nAddress = pMemoryInspector.Viewer().GetAddress();

    // if the code note specifies an explicit size, use it. otherwise, use the selected viewer mode size.
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
    auto nSize = pGameContext.GetCodeNoteMemSize(nAddress);
    if (nSize >= MemSize::Unknown)
        nSize = pMemoryInspector.Viewer().GetSize();

    vmCondition.SetSourceType(TriggerOperandType::Address);
    vmCondition.SetSourceSize(nSize);
    vmCondition.SetSourceValue(nAddress);

    // assume the user wants to compare to the current value of the watched memory address
    vmCondition.SetOperator(TriggerOperatorType::Equals);

    const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::context::EmulatorContext>();
    const auto nValue = pEmulatorContext.ReadMemory(nAddress, nSize);

    vmCondition.SetTargetSize(nSize);
    switch (nSize)
    {
        case MemSize::Float:
        case MemSize::MBF32:
            vmCondition.SetTargetType(TriggerOperandType::Float);
            vmCondition.SetTargetValue(ra::data::U32ToFloat(nValue, nSize));
            break;

        default:
            vmCondition.SetTargetType(TriggerOperandType::Value);
            vmCondition.SetTargetValue(nValue);
            break;
    }

    vmCondition.SetSelected(true);

    m_vConditions.EndUpdate();

    SetValue(EnsureVisibleConditionIndexProperty, gsl::narrow_cast<int>(m_vConditions.Count()) - 1);
}

static void AddAltGroup(ViewModelCollection<TriggerViewModel::GroupViewModel>& vGroups,
    int nId, rc_condset_t* pConditionSet)
{
    auto& vmAltGroup = vGroups.Add();
    vmAltGroup.SetId(nId);
    vmAltGroup.SetLabel(ra::StringPrintf(L"%s%d", (nId < 1000) ? "Alt " : "A", nId));
    vmAltGroup.m_pConditionSet = pConditionSet;
}

void TriggerViewModel::InitializeGroups(const rc_trigger_t& pTrigger)
{
    m_vGroups.RemoveNotifyTarget(*this);
    m_vGroups.BeginUpdate();

    SetMeasuredTrackedAsPercent(m_pTrigger != nullptr ? m_pTrigger->measured_as_percent : false);

    // this will not update the conditions collection because OnValueChange ignores it when m_vGroups.IsUpdating
    SetSelectedGroupIndex(0);

    m_vGroups.Clear();

    auto& vmCoreGroup = m_vGroups.Add();
    vmCoreGroup.SetId(0);
    vmCoreGroup.SetLabel(m_bIsValue ? L"Value" : L"Core");
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

void TriggerViewModel::InitializeFrom(const rc_trigger_t& pTrigger)
{
    m_bIsValue = false;
    m_pTrigger = nullptr;
    InitializeGroups(pTrigger);
}

rc_trigger_t* TriggerViewModel::ParseTrigger(const std::string& sTrigger)
{
    if (m_bIsValue)
    {
        const auto nSize = rc_value_size(sTrigger.c_str());
        if (nSize > 0)
        {
            m_sTriggerBuffer.resize(nSize + sizeof(rc_trigger_t));
            rc_value_t* pValue = rc_parse_value(m_sTriggerBuffer.data(), sTrigger.c_str(), nullptr, 0);
            rc_trigger_t* pTrigger;
            GSL_SUPPRESS_TYPE1 pTrigger = reinterpret_cast<rc_trigger_t*>(m_sTriggerBuffer.data() + nSize);
            memset(pTrigger, 0, sizeof(rc_trigger_t));
            pTrigger->requirement = pValue->conditions;
            pTrigger->alternative = pValue->conditions->next;
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
    m_pTrigger = ParseTrigger(sTrigger);
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
    m_bIsValue = true;
    m_sTriggerBuffer.resize(sizeof(rc_trigger_t));
    GSL_SUPPRESS_TYPE1 m_pTrigger = reinterpret_cast<rc_trigger_t*>(m_sTriggerBuffer.data());
    memset(m_pTrigger, 0, sizeof(rc_trigger_t));
    m_pTrigger->requirement = pValue.conditions;
    m_pTrigger->alternative = (pValue.conditions) ? pValue.conditions->next : nullptr;
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
            AddAltGroup(m_vGroups, gsl::narrow_cast<int>(nIndex), pConditionSet);
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
        UpdateConditions(m_vGroups.GetItemAt(nSelectedIndex));
    }
    else
    {
        // selected group no longer exists, select core group
        SetSelectedGroupIndex(0);
    }
}

void TriggerViewModel::UpdateFrom(const rc_trigger_t& pTrigger)
{
    m_pTrigger = nullptr;
    m_sTriggerBuffer.clear();
    UpdateGroups(pTrigger);
}

void TriggerViewModel::UpdateFrom(const std::string& sTrigger)
{
    m_pTrigger = ParseTrigger(sTrigger);
    if (m_pTrigger)
    {
        UpdateGroups(*m_pTrigger);
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
    m_pTrigger = nullptr;
    m_sTriggerBuffer.resize(sizeof(rc_trigger_t));
    GSL_SUPPRESS_TYPE1 m_pTrigger = reinterpret_cast<rc_trigger_t*>(m_sTriggerBuffer.data());
    memset(m_pTrigger, 0, sizeof(rc_trigger_t));
    m_pTrigger->requirement = pValue.conditions;
    m_pTrigger->alternative = pValue.conditions->next;
    UpdateGroups(*m_pTrigger);
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

    auto* pGroup = m_vmTrigger->m_vGroups.GetItemAt(m_vmTrigger->GetSelectedGroupIndex());
    if (pGroup)
    {
        if (pGroup->UpdateSerialized(m_vmTrigger->m_vConditions))
            m_vmTrigger->UpdateVersion();
    }
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
        if (pGroup != nullptr)
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
        if (pGroup != nullptr)
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
                if (pOldGroup != nullptr) // group may have been deleted
                    pOldGroup->SetSelected(false);
            }

            m_vGroups.AddNotifyTarget(*this);

            UpdateConditions(pGroup);
        }
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
        UpdateConditions(pGroup);
}

void TriggerViewModel::UpdateConditions(const GroupViewModel* pGroup)
{
    m_vConditions.RemoveNotifyTarget(m_pConditionsMonitor);
    m_vConditions.BeginUpdate();

    int nIndex = 0;
    m_bHasHitChain = false;

    if (pGroup != nullptr)
    {
        std::string sBuffer;
        rc_condset_t* pConditions = pGroup->m_pConditionSet;
        if (pConditions == nullptr)
        {
            const auto sTrigger = pGroup->GetSerialized(*this);
            const auto nSize = rc_trigger_size(sTrigger.c_str());
            if (nSize >= 0)
            {
                sBuffer.resize(nSize);
                const auto pTrigger = rc_parse_trigger(sBuffer.data(), sTrigger.c_str(), nullptr, 0);
                pConditions = pTrigger->requirement;
            }
        }

        if (pConditions)
        {
            bool bIsIndirect = false;
            rc_condition_t* pCondition = pConditions->conditions;
            for (; pCondition != nullptr; pCondition = pCondition->next)
            {
                auto* vmCondition = m_vConditions.GetItemAt(gsl::narrow_cast<gsl::index>(nIndex));
                if (vmCondition == nullptr)
                {
                    vmCondition = &m_vConditions.Add();
                    Ensures(vmCondition != nullptr);
                    vmCondition->SetTriggerViewModel(this);
                }

                vmCondition->SetIndex(++nIndex);
                vmCondition->InitializeFrom(*pCondition);
                vmCondition->SetCurrentHits(pCondition->current_hits);
                vmCondition->SetTotalHits(0);

                vmCondition->SetIndirect(bIsIndirect);
                bIsIndirect = (pCondition->type == RC_CONDITION_ADD_ADDRESS);

                m_bHasHitChain |= (pCondition->type == RC_CONDITION_ADD_HITS || pCondition->type == RC_CONDITION_SUB_HITS);
            }
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

    if (nGroups == 1 && !m_bIsValue)
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
    auto* pGroup = m_vGroups.GetItemAt(GetSelectedGroupIndex());
    if (pGroup != nullptr && pGroup->m_pConditionSet)
    {
        m_vConditions.RemoveNotifyTarget(m_pConditionsMonitor);
        m_vConditions.BeginUpdate();

        unsigned int nHits = 0;
        bool bIsHitsChain = false;

        gsl::index nConditionIndex = 0;
        rc_condition_t* pCondition = pGroup->m_pConditionSet->conditions;
        for (; pCondition != nullptr; pCondition = pCondition->next)
        {
            auto* vmCondition = m_vConditions.GetItemAt(nConditionIndex++);
            Expects(vmCondition != nullptr);

            vmCondition->SetCurrentHits(pCondition->current_hits);

            if (m_bHasHitChain)
            {
                switch (vmCondition->GetType())
                {
                    case ra::ui::viewmodels::TriggerConditionType::AddHits:
                        bIsHitsChain = true;
                        nHits += vmCondition->GetCurrentHits();
                        break;

                    case ra::ui::viewmodels::TriggerConditionType::SubHits:
                        bIsHitsChain = true;
                        nHits -= vmCondition->GetCurrentHits();
                        break;

                    case ra::ui::viewmodels::TriggerConditionType::AddAddress:
                    case ra::ui::viewmodels::TriggerConditionType::AddSource:
                    case ra::ui::viewmodels::TriggerConditionType::SubSource:
                    case ra::ui::viewmodels::TriggerConditionType::AndNext:
                    case ra::ui::viewmodels::TriggerConditionType::OrNext:
                    case ra::ui::viewmodels::TriggerConditionType::ResetNextIf:
                        break;

                    default:
                        if (bIsHitsChain)
                        {
                            nHits += vmCondition->GetCurrentHits();
                            vmCondition->SetTotalHits(nHits);
                            bIsHitsChain = false;
                        }
                        nHits = 0;
                        break;
                }
            }
        }

        m_vConditions.EndUpdate();
        m_vConditions.AddNotifyTarget(m_pConditionsMonitor);
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
            rc_condition_t* pCondition = pGroup->m_pConditionSet->conditions;
            for (; pCondition != nullptr; pCondition = pCondition->next)
            {
                // a reset condition cannot remain true with a target hitcount, so only check if it's currently true
                if (pCondition->is_true && pCondition->type == RC_CONDITION_RESET_IF)
                {
                    bIsReset = true;
                    break;
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

void TriggerViewModel::UpdateConditionColors(const rc_trigger_t* pTrigger)
{
    // update the condition rows for the currently selected group
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
                rc_condition_t* pCondition = pSelectedGroup->m_pConditionSet->conditions;
                for (; pCondition != nullptr; pCondition = pCondition->next, ++nConditionIndex)
                {
                    auto* vmCondition = m_vConditions.GetItemAt(nConditionIndex);
                    if (vmCondition != nullptr)
                    {
                        if (pCondition->pause && bFirstPause)
                        {
                            vmCondition->UpdateRowColor(pCondition);

                            // processing stops when a PauseIf has met its hit target
                            if (pCondition->type == RC_CONDITION_PAUSE_IF)
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
                rc_condition_t* pCondition = pSelectedGroup->m_pConditionSet->conditions;
                for (; pCondition != nullptr; pCondition = pCondition->next, ++nConditionIndex)
                {
                    auto* vmCondition = m_vConditions.GetItemAt(nConditionIndex);
                    if (vmCondition != nullptr)
                        vmCondition->UpdateRowColor(pCondition);
                }
            }
        }
    }

    for (; nConditionIndex < gsl::narrow_cast<gsl::index>(m_vConditions.Count()); ++nConditionIndex)
    {
        auto* pCondition = m_vConditions.GetItemAt(nConditionIndex);
        if (pCondition != nullptr)
            pCondition->UpdateRowColor(nullptr);
    }
}

} // namespace viewmodels
} // namespace ui
} // namespace ra
