#ifndef RA_UI_TRIGGERVIEWMODEL_H
#define RA_UI_TRIGGERVIEWMODEL_H
#pragma once

#include "data\models\CapturedTriggerHits.hh"

#include "ui\ViewModelBase.hh"
#include "ui\ViewModelCollection.hh"

#include "LookupItemViewModel.hh"
#include "TriggerConditionViewModel.hh"

struct rc_condset_t;

namespace ra {
namespace ui {
namespace viewmodels {

class TriggerViewModel : public ViewModelBase, 
    protected ViewModelCollectionBase::NotifyTarget
{
public:
    GSL_SUPPRESS_F6 TriggerViewModel() noexcept;
    ~TriggerViewModel() = default;

    TriggerViewModel(const TriggerViewModel&) noexcept = delete;
    TriggerViewModel& operator=(const TriggerViewModel&) noexcept = delete;
    TriggerViewModel(TriggerViewModel&&) noexcept = delete;
    TriggerViewModel& operator=(TriggerViewModel&&) noexcept = delete;


    static const IntModelProperty SelectedGroupIndexProperty;
    int GetSelectedGroupIndex() const { return GetValue(SelectedGroupIndexProperty); }
    void SetSelectedGroupIndex(int nValue) { SetValue(SelectedGroupIndexProperty, nValue); }

    static const BoolModelProperty PauseOnResetProperty;
    bool IsPauseOnReset() const { return GetValue(PauseOnResetProperty); }
    void SetPauseOnReset(bool nValue) { SetValue(PauseOnResetProperty, nValue); }

    static const BoolModelProperty PauseOnTriggerProperty;
    bool IsPauseOnTrigger() const { return GetValue(PauseOnTriggerProperty); }
    void SetPauseOnTrigger(bool nValue) { SetValue(PauseOnTriggerProperty, nValue); }

    class GroupViewModel : public LookupItemViewModel,
        protected ViewModelCollectionBase::NotifyTarget
    {
    public:
        rc_condset_t* m_pConditionSet = nullptr;

        bool UpdateSerialized(const ViewModelCollection<TriggerConditionViewModel>& vConditions);
        const std::string& GetSerialized() const;
        void ResetSerialized() noexcept { m_sSerialized.clear(); }

    private:
        static void UpdateSerialized(std::string& sSerialized, const ViewModelCollection<TriggerConditionViewModel>& vConditions);
        mutable std::string m_sSerialized;
    };

    ViewModelCollection<GroupViewModel>& Groups() noexcept { return m_vGroups; }
    const ViewModelCollection<GroupViewModel>& Groups() const noexcept { return m_vGroups; }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the index of the group that should be made visible.
    /// </summary>
    static const IntModelProperty EnsureVisibleGroupIndexProperty;

    ViewModelCollection<TriggerConditionViewModel>& Conditions() noexcept { return m_vConditions; }
    const ViewModelCollection<TriggerConditionViewModel>& Conditions() const noexcept { return m_vConditions; }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the index of the condition that should be made visible.
    /// </summary>
    static const IntModelProperty EnsureVisibleConditionIndexProperty;

    /// <summary>
    /// The <see cref="ModelProperty" /> for the number of changes that have been made to the trigger.
    /// </summary>
    static const IntModelProperty VersionProperty;

    std::string Serialize() const;
    void SerializeAppend(std::string& sBuffer) const;

    void InitializeFrom(const rc_trigger_t& pTrigger);
    void InitializeFrom(const std::string& sTrigger, const ra::data::models::CapturedTriggerHits& pCapturedHits);
    void UpdateFrom(const rc_trigger_t& pTrigger);
    void UpdateFrom(const std::string& sTrigger);

    void RemoveSelectedConditions();
    void CopySelectedConditionsToClipboard();
    void PasteFromClipboard();

    void MoveSelectedConditionsUp();
    void MoveSelectedConditionsDown();

    void NewCondition();

    void AddGroup();
    void RemoveGroup();

    /// <summary>
    /// Gets the list of condition types.
    /// </summary>
    const LookupItemViewModelCollection& ConditionTypes() const noexcept
    {
        return m_vConditionTypes;
    }

    /// <summary>
    /// Gets the list of operand types.
    /// </summary>
    const LookupItemViewModelCollection& OperandTypes() const noexcept
    {
        return m_vOperandTypes;
    }

    /// <summary>
    /// Gets the list of operand sizes.
    /// </summary>
    const LookupItemViewModelCollection& OperandSizes() const noexcept
    {
        return m_vOperandSizes;
    }

    /// <summary>
    /// Gets the list of operator types.
    /// </summary>
    const LookupItemViewModelCollection& OperatorTypes() const noexcept
    {
        return m_vOperatorTypes;
    }

    void DoFrame();

protected:
    // ViewModelCollectionBase::NotifyTarget
    void OnViewModelBoolValueChanged(gsl::index nIndex, const BoolModelProperty::ChangeArgs& args) override;

    void OnValueChanged(const IntModelProperty::ChangeArgs& args) override;

private:
    void UpdateVersion();
    void UpdateConditions(const GroupViewModel* pGroup);

    void DeselectAllConditions();
    void UpdateIndicesAndEnsureSelectionVisible();

    ViewModelCollection<GroupViewModel> m_vGroups;
    ViewModelCollection<TriggerConditionViewModel> m_vConditions;

    class ConditionsMonitor : public ViewModelCollectionBase::NotifyTarget
    {
    public:
        ConditionsMonitor(TriggerViewModel& vmTrigger) noexcept
        {
            m_vmTrigger = &vmTrigger;
        }

    protected:
        void OnViewModelIntValueChanged(gsl::index nIndex, const IntModelProperty::ChangeArgs& args) override;
        void OnViewModelAdded(gsl::index nIndex) override;
        void OnViewModelRemoved(gsl::index nIndex) override;
        void OnEndViewModelCollectionUpdate() override;

        void UpdateCurrentGroup();

        TriggerViewModel* m_vmTrigger;
    };

    ConditionsMonitor m_pConditionsMonitor;

    LookupItemViewModelCollection m_vConditionTypes;
    LookupItemViewModelCollection m_vOperandTypes;
    LookupItemViewModelCollection m_vOperandSizes;
    LookupItemViewModelCollection m_vOperatorTypes;

    std::string m_sTriggerBuffer;
    rc_trigger_t* m_pTrigger = nullptr;
};

} // namespace viewmodels
} // namespace ui
} // namespace ra

#endif !RA_UI_TRIGGERVIEWMODEL_H
