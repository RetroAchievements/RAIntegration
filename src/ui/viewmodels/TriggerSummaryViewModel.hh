#ifndef RA_UI_TRIGGERSUMMARYVIEWMODEL_H
#define RA_UI_TRIGGERSUMMARYVIEWMODEL_H
#pragma once

#include "ui\ViewModelBase.hh"
#include "ui\ViewModelCollection.hh"
#include "ui\Types.hh"

struct rc_condset_t;

namespace ra {
namespace ui {
namespace viewmodels {

class TriggerSummaryViewModel : public ViewModelBase
{
public:
    GSL_SUPPRESS_F6 TriggerSummaryViewModel() noexcept = default;
    ~TriggerSummaryViewModel() = default;

    TriggerSummaryViewModel(const TriggerSummaryViewModel&) noexcept = delete;
    TriggerSummaryViewModel& operator=(const TriggerSummaryViewModel&) noexcept = delete;
    TriggerSummaryViewModel(TriggerSummaryViewModel&&) noexcept = delete;
    TriggerSummaryViewModel& operator=(TriggerSummaryViewModel&&) noexcept = delete;

    void InitializeFrom(const rc_condset_t& pCondSet);

    class TriggerClauseViewModel : public ViewModelBase
    {
    public:
        static const StringModelProperty IndicesProperty;
        const std::wstring& GetIndices() const { return GetValue(IndicesProperty); }
        void SetIndices(const std::wstring& sValue) { SetValue(IndicesProperty, sValue); }

        static const StringModelProperty ReferenceProperty;
        const std::wstring& GetReference() const { return GetValue(ReferenceProperty); }
        void SetReference(const std::wstring& sValue) { SetValue(ReferenceProperty, sValue); }

        static const StringModelProperty OperationProperty;
        const std::wstring& GetOperation() const { return GetValue(OperationProperty); }
        void SetOperation(const std::wstring& sValue) { SetValue(OperationProperty, sValue); }

        static const StringModelProperty TargetProperty;
        const std::wstring& GetTarget() const { return GetValue(TargetProperty); }
        void SetTarget(const std::wstring& sValue) { SetValue(TargetProperty, sValue); }

        std::wstring GetTooltip(const StringModelProperty& nProperty) const;
    };

    const ViewModelCollection<TriggerClauseViewModel>& Clauses() const { return m_vClauses; }

private:
    ViewModelCollection<TriggerClauseViewModel> m_vClauses;
};

} // namespace viewmodels
} // namespace ui
} // namespace ra

#endif !RA_UI_TRIGGERSUMMARYVIEWMODEL_H
