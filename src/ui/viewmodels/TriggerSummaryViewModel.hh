#ifndef RA_UI_TRIGGERSUMMARYVIEWMODEL_H
#define RA_UI_TRIGGERSUMMARYVIEWMODEL_H
#pragma once

#include "ui\WindowViewModelBase.hh"
#include "ui\ViewModelCollection.hh"
#include "ui\Types.hh"

struct rc_condset_t;

namespace ra {
namespace ui {
namespace viewmodels {

class TriggerSummaryViewModel : public WindowViewModelBase
{
public:
    GSL_SUPPRESS_F6 TriggerSummaryViewModel() noexcept = default;
    ~TriggerSummaryViewModel() = default;

    TriggerSummaryViewModel(const TriggerSummaryViewModel&) noexcept = delete;
    TriggerSummaryViewModel& operator=(const TriggerSummaryViewModel&) noexcept = delete;
    TriggerSummaryViewModel(TriggerSummaryViewModel&&) noexcept = delete;
    TriggerSummaryViewModel& operator=(TriggerSummaryViewModel&&) noexcept = delete;

    void InitializeFrom(const rc_condset_t& pCondSet);
    void AddHeaders();

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

        static const StringModelProperty TallyProperty;
        const std::wstring& GetTally() const { return GetValue(TallyProperty); }
        void SetTally(const std::wstring& sValue) { SetValue(TallyProperty, sValue); }

        static const IntModelProperty ColorProperty;
        Color GetColor() const { return Color(ra::to_unsigned(GetValue(ColorProperty))); }
        void SetColor(Color value) { SetValue(ColorProperty, ra::to_signed(value.ARGB)); }

        std::wstring GetTooltip(const StringModelProperty& nProperty) const;

        const rc_condition_t* pCondition = nullptr;
        enum TriggerClauseType : int;
        TriggerClauseType nType = ra::itoe<TriggerClauseType>(0);
    };

    ViewModelCollection<TriggerClauseViewModel>& Clauses() { return m_vClauses; }
    const ViewModelCollection<TriggerClauseViewModel>& Clauses() const { return m_vClauses; }

private:
    bool MergeClauses(
        TriggerSummaryViewModel::TriggerClauseViewModel& pClause1,
        TriggerSummaryViewModel::TriggerClauseViewModel& pClause2,
        gsl::index nIndex1, gsl::index nIndex2);
    bool MergeClauses(TriggerSummaryViewModel::TriggerClauseViewModel& pClause,
        TriggerSummaryViewModel::TriggerClauseViewModel& pDiscardClause,
        gsl::index nDiscardIndex, TriggerClauseViewModel::TriggerClauseType nNewType, const std::wstring& sNewOperation);

    ViewModelCollection<TriggerClauseViewModel> m_vClauses;
};

} // namespace viewmodels
} // namespace ui
} // namespace ra

#endif !RA_UI_TRIGGERSUMMARYVIEWMODEL_H
