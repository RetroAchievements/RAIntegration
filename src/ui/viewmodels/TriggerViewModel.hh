#ifndef RA_UI_TRIGGERVIEWMODEL_H
#define RA_UI_TRIGGERVIEWMODEL_H
#pragma once

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
        ViewModelCollection<TriggerConditionViewModel>& Conditions() noexcept { return m_vConditions; }
        const ViewModelCollection<TriggerConditionViewModel>& Conditions() const noexcept { return m_vConditions; }

        std::string Serialize() const;
        void SerializeAppend(std::string& sBuffer) const;

        void InitializeFrom(const struct rc_condset_t& pConditionSet);

        static const IntModelProperty VersionProperty;

    protected:
        void OnViewModelIntValueChanged(gsl::index nIndex, const IntModelProperty::ChangeArgs& args) override;
        void OnViewModelAdded(gsl::index nIndex) override;
        void OnViewModelRemoved(gsl::index nIndex) override;

    private:
        void UpdateVersion();

        ViewModelCollection<TriggerConditionViewModel> m_vConditions;
    };

    ViewModelCollection<GroupViewModel>& Groups() noexcept { return m_vGroups; }
    const ViewModelCollection<GroupViewModel>& Groups() const noexcept { return m_vGroups; }

    static const IntModelProperty VersionProperty;

    std::string Serialize() const;
    void SerializeAppend(std::string& sBuffer) const;

    void InitializeFrom(const rc_trigger_t& pTrigger);

protected:
    // ViewModelCollectionBase::NotifyTarget
    void OnViewModelIntValueChanged(gsl::index nIndex, const IntModelProperty::ChangeArgs& args) override;
    void OnViewModelAdded(gsl::index nIndex) override;
    void OnViewModelRemoved(gsl::index nIndex) override;

private:
    void UpdateVersion();

    ViewModelCollection<GroupViewModel> m_vGroups;
};

} // namespace viewmodels
} // namespace ui
} // namespace ra

#endif !RA_UI_TRIGGERVIEWMODEL_H
