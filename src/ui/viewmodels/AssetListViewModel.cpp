#include "AssetListViewModel.hh"

#include "data\context\GameContext.hh"
#include "data\context\UserContext.hh"

#include "services\AchievementRuntime.hh"
#include "services\IConfiguration.hh"
#include "services\ILocalStorage.hh"
#include "services\IThreadPool.hh"
#include "services\ServiceLocator.hh"

#include "ui\viewmodels\AssetUploadViewModel.hh"
#include "ui\viewmodels\MessageBoxViewModel.hh"
#include "ui\viewmodels\OverlayManager.hh"
#include "ui\viewmodels\WindowManager.hh"

#include "Exports.hh"
#include "RA_BuildVer.h"
#include "RA_Log.h"

namespace ra {
namespace ui {
namespace viewmodels {

const IntModelProperty AssetListViewModel::GameIdProperty("AssetListViewModel", "GameId", 0);
const IntModelProperty AssetListViewModel::AchievementCountProperty("AssetListViewModel", "AchievementCount", 0);
const IntModelProperty AssetListViewModel::TotalPointsProperty("AssetListViewModel", "TotalPoints", 0);
const BoolModelProperty AssetListViewModel::IsProcessingActiveProperty("AssetListViewModel", "IsProcessingActive", true);
const BoolModelProperty AssetListViewModel::KeepActiveProperty("AssetListViewModel", "KeepActive", false);
const StringModelProperty AssetListViewModel::ActivateButtonTextProperty("AssetListViewModel", "ActivateButtonText", L"&Activate All");
const StringModelProperty AssetListViewModel::SaveButtonTextProperty("AssetListViewModel", "SaveButtonText", L"&Save All");
const StringModelProperty AssetListViewModel::ResetButtonTextProperty("AssetListViewModel", "ResetButtonText", L"&Reset All");
const StringModelProperty AssetListViewModel::RevertButtonTextProperty("AssetListViewModel", "RevertButtonText", L"Re&vert All");
const BoolModelProperty AssetListViewModel::CanActivateProperty("AssetListViewModel", "CanActivate", false);
const BoolModelProperty AssetListViewModel::CanSaveProperty("AssetListViewModel", "CanSave", false);
const BoolModelProperty AssetListViewModel::CanResetProperty("AssetListViewModel", "CanReset", false);
const BoolModelProperty AssetListViewModel::CanRevertProperty("AssetListViewModel", "CanRevert", false);
const BoolModelProperty AssetListViewModel::CanCreateProperty("AssetListViewModel", "CanCreate", false);
const BoolModelProperty AssetListViewModel::CanCloneProperty("AssetListViewModel", "CanClone", false);
const IntModelProperty AssetListViewModel::FilterCategoryProperty("AssetListViewModel", "FilterCategory", ra::etoi(AssetListViewModel::FilterCategory::Core));
const IntModelProperty AssetListViewModel::SpecialFilterProperty("AssetListViewModel", "SpecialFilter", ra::etoi(AssetListViewModel::SpecialFilter::All));
const IntModelProperty AssetListViewModel::EnsureVisibleAssetIndexProperty("AssetListViewModel", "EnsureVisibleAssetIndex", -1);

AssetListViewModel::AssetListViewModel() noexcept
{
    SetWindowTitle(L"Achievements List");

    m_vStates.Add(ra::etoi(ra::data::models::AssetState::Inactive), L"Inactive");
    m_vStates.Add(ra::etoi(ra::data::models::AssetState::Waiting), L"Waiting");
    m_vStates.Add(ra::etoi(ra::data::models::AssetState::Active), L"Active");
    m_vStates.Add(ra::etoi(ra::data::models::AssetState::Paused), L"Paused");
    m_vStates.Add(ra::etoi(ra::data::models::AssetState::Primed), L"Primed");
    m_vStates.Add(ra::etoi(ra::data::models::AssetState::Triggered), L"Triggered");
    m_vStates.Add(ra::etoi(ra::data::models::AssetState::Disabled), L"Disabled");

    m_vCategories.Add(ra::etoi(FilterCategory::Core), L"Core");
    m_vCategories.Add(ra::etoi(FilterCategory::Unofficial), L"Unofficial");
    m_vCategories.Add(ra::etoi(FilterCategory::Local), L"Local");
    m_vCategories.Add(ra::etoi(FilterCategory::All), L"All");

    m_vSpecialFilters.Add(ra::etoi(SpecialFilter::All), L"All");
    m_vSpecialFilters.Add(ra::etoi(SpecialFilter::Active), L"Active");
    m_vSpecialFilters.Add(ra::etoi(SpecialFilter::Inactive), L"Inactive");
    m_vSpecialFilters.Add(ra::etoi(SpecialFilter::Modified), L"Modified");
    m_vSpecialFilters.Add(ra::etoi(SpecialFilter::Unpublished), L"Unpublished");
    m_vSpecialFilters.Add(ra::etoi(SpecialFilter::Authored), L"Authored");

    m_vChanges.Add(ra::etoi(ra::data::models::AssetChanges::None), L"");
    m_vChanges.Add(ra::etoi(ra::data::models::AssetChanges::Modified), L"Modified");
    m_vChanges.Add(ra::etoi(ra::data::models::AssetChanges::Unpublished), L"Unpublished");
    m_vChanges.Add(ra::etoi(ra::data::models::AssetChanges::New), L"New");
    m_vChanges.Add(ra::etoi(ra::data::models::AssetChanges::Deleted), L"Deleted");

    // have to use separate monitor for capturing selection changes in filtered list
    // so EndUpdate for filtered list doesn't trigger another call to ApplyFilter.
    m_pFilteredListMonitor.m_pOwner = this;
    m_vFilteredAssets.AddNotifyTarget(m_pFilteredListMonitor);
}

AssetListViewModel::~AssetListViewModel()
{
    m_bNeedToUpdateButtons.store(false);
}

void AssetListViewModel::InitializeNotifyTargets()
{
    auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::context::GameContext>();
    pGameContext.AddNotifyTarget(*this);
    pGameContext.Assets().AddNotifyTarget(*this);
}

void AssetListViewModel::OnActiveGameChanged()
{
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
    SetGameId(pGameContext.GameId());
}

void AssetListViewModel::OnDataModelStringValueChanged(gsl::index nIndex, const StringModelProperty::ChangeArgs& args)
{
    // sync this property through to the summary view model
    if (args.Property == ra::data::models::AssetModelBase::NameProperty)
    {
        const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
        const auto* pItem = pGameContext.Assets().GetItemAt(nIndex);
        if (pItem != nullptr)
        {
            const auto nFilteredIndex = GetFilteredAssetIndex(*pItem);
            if (nFilteredIndex != -1)
                m_vFilteredAssets.SetItemValue(nFilteredIndex, AssetSummaryViewModel::LabelProperty, args.tNewValue);
        }
    }
}

void AssetListViewModel::OnDataModelIntValueChanged(gsl::index nIndex, const IntModelProperty::ChangeArgs& args)
{
    if (args.Property == ra::data::models::AchievementModel::PointsProperty)
    {
        const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
        const auto* pAsset = pGameContext.Assets().GetItemAt(nIndex);
        const auto* pAchievement = dynamic_cast<const ra::data::models::AchievementModel*>(pAsset);
        if (pAchievement != nullptr && GetFilteredAssetIndex(*pAsset) >= 0)
        {
            auto nPoints = GetTotalPoints();
            nPoints += args.tNewValue - args.tOldValue;
            SetValue(TotalPointsProperty, nPoints);
        }
    }
    else if (args.Property == ra::data::models::AssetModelBase::StateProperty)
    {
        if (GetSpecialFilter() != SpecialFilter::All)
            AddOrRemoveFilteredItem(nIndex);

        const auto nNewState = ra::itoe<ra::data::models::AssetState>(args.tNewValue);
        if (!ra::data::models::AssetModelBase::IsActive(nNewState))
        {
            // when an active achievement is deactivated, the challenge indicator needs
            // to be hidden as no event will be raised
            const auto nOldState = ra::itoe<ra::data::models::AssetState>(args.tOldValue);
            if (ra::data::models::AssetModelBase::IsActive(nOldState))
            {
                const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
                const auto* pAsset = pGameContext.Assets().GetItemAt(nIndex);
                if (pAsset->GetType() == ra::data::models::AssetType::Achievement)
                {
                    auto& pOverlayManager = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>();
                    pOverlayManager.RemoveChallengeIndicator(pAsset->GetID());
                }
            }
        }

        if (nNewState == ra::data::models::AssetState::Triggered && KeepActive())
        {
            // if KeepActive is selected, set a Triggered achievement back to Waiting
            auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::context::GameContext>();
            auto* pAsset = pGameContext.Assets().GetItemAt(nIndex);
            pAsset->SetState(ra::data::models::AssetState::Waiting);

            // SetState is re-entrant - we don't want to do any further processing with the previous new value
            return;
        }
    }

    // these properties potentially affect visibility
    if (args.Property == ra::data::models::AssetModelBase::CategoryProperty)
    {
        const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
        const auto* pAsset = pGameContext.Assets().GetItemAt(nIndex);
        if (pAsset != nullptr)
        {
            RA_LOG_INFO("%s %u category changed from %d to %d", ra::data::models::AssetModelBase::GetAssetTypeString(pAsset->GetType()), pAsset->GetID(), args.tOldValue, args.tNewValue);
        }

        UpdateTotals();

        if (GetFilterCategory() != FilterCategory::All)
            AddOrRemoveFilteredItem(nIndex);
    }
    else if (args.Property == ra::data::models::AssetModelBase::StateProperty ||
             args.Property == ra::data::models::AssetModelBase::ChangesProperty)
    {
        if (GetSpecialFilter() != SpecialFilter::All)
            AddOrRemoveFilteredItem(nIndex);
    }

    // sync these properties through to the summary view model
    if (args.Property == ra::data::models::AssetModelBase::ChangesProperty ||
        args.Property == ra::data::models::AchievementModel::PointsProperty ||
        args.Property == ra::data::models::AssetModelBase::StateProperty ||
        args.Property == ra::data::models::AssetModelBase::CategoryProperty)
    {
        const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
        const auto* pAsset = pGameContext.Assets().GetItemAt(nIndex);
        if (pAsset != nullptr)
        {
            const auto nFilteredIndex = GetFilteredAssetIndex(*pAsset);
            if (nFilteredIndex != -1)
            {
                m_vFilteredAssets.SetItemValue(nFilteredIndex, args.Property, args.tNewValue);

                if (m_vFilteredAssets.GetItemValue(nFilteredIndex, AssetSummaryViewModel::IsSelectedProperty))
                    UpdateButtons();
            }
        }
    }
    else if (args.Property == ra::data::models::AssetModelBase::IDProperty)
    {
        const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
        const auto* pAsset = pGameContext.Assets().GetItemAt(nIndex);
        if (pAsset != nullptr)
        {
            gsl::index nFilteredIndex = -1;
            const auto nType = pAsset->GetType();

            RA_LOG_INFO("%s %u ID changed from %d to %d", ra::data::models::AssetModelBase::GetAssetTypeString(pAsset->GetType()), pAsset->GetID(), args.tOldValue, args.tNewValue);

            // have to find the filtered item using the old ID
            for (gsl::index nScanIndex = 0; nScanIndex < gsl::narrow_cast<gsl::index>(m_vFilteredAssets.Count()); ++nScanIndex)
            {
                auto* pItem = m_vFilteredAssets.GetItemAt(nScanIndex);
                if (pItem != nullptr && pItem->GetId() == args.tOldValue && pItem->GetType() == nType)
                {
                    nFilteredIndex = nScanIndex;
                    break;
                }
            }

            if (nFilteredIndex != -1)
                m_vFilteredAssets.SetItemValue(nFilteredIndex, AssetSummaryViewModel::IdProperty, args.tNewValue);
        }
    }
}

void AssetListViewModel::OnDataModelAdded(_UNUSED gsl::index nIndex)
{
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
    if (!pGameContext.Assets().IsUpdating())
        AddOrRemoveFilteredItem(nIndex);
}

void AssetListViewModel::OnDataModelRemoved(_UNUSED gsl::index nIndex)
{
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
    if (!pGameContext.Assets().IsUpdating())
    {
        // the item has already been removed from the vAssets collection, and all we know about it
        // is the index where it was located. scan through the vFilteredAssets collection and remove 
        // any that no longer exist in the vAssets collection.
        for (gsl::index nFilteredIndex = 0; nFilteredIndex < gsl::narrow_cast<gsl::index>(m_vFilteredAssets.Count()); ++nFilteredIndex)
        {
            auto* pItem = m_vFilteredAssets.GetItemAt(nFilteredIndex);
            if (pItem != nullptr)
            {
                if (!pGameContext.Assets().FindAsset(pItem->GetType(), pItem->GetId()))
                {
                    m_vFilteredAssets.RemoveAt(nFilteredIndex);
                    UpdateButtons();
                    break;
                }
            }
        }

        UpdateTotals();
    }
}

void AssetListViewModel::OnDataModelChanged(_UNUSED gsl::index nIndex)
{
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
    if (!pGameContext.Assets().IsUpdating())
    {
        const auto* pAsset = pGameContext.Assets().GetItemAt(nIndex);
        if (pAsset != nullptr)
            AddOrRemoveFilteredItem(*pAsset);

        UpdateTotals();
    }
}

void AssetListViewModel::OnEndDataModelCollectionUpdate()
{
    ApplyFilter();
}

void AssetListViewModel::UpdateTotals()
{
    int nAchievementCount = 0;
    int nTotalPoints = 0;

    for (gsl::index nIndex = 0; nIndex < gsl::narrow_cast<gsl::index>(m_vFilteredAssets.Count()); ++nIndex)
    {
        auto* pAsset = m_vFilteredAssets.GetItemAt(nIndex);
        if (pAsset && pAsset->GetType() == ra::data::models::AssetType::Achievement)
        {
            ++nAchievementCount;
            nTotalPoints += pAsset->GetPoints();
        }
    }

    SetValue(AchievementCountProperty, nAchievementCount);
    SetValue(TotalPointsProperty, nTotalPoints);
}

void AssetListViewModel::OnValueChanged(const IntModelProperty::ChangeArgs& args)
{
    if (args.Property == FilterCategoryProperty || args.Property == SpecialFilterProperty)
        ApplyFilter();
    else if (args.Property == GameIdProperty)
        UpdateButtons();

    WindowViewModelBase::OnValueChanged(args);
}

void AssetListViewModel::OnValueChanged(const BoolModelProperty::ChangeArgs& args)
{
    if (args.Property == IsProcessingActiveProperty)
    {
        auto& pRuntime = ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>();
        pRuntime.SetPaused(!args.tNewValue);

        auto& pEmulatorContext = ra::services::ServiceLocator::GetMutable<ra::data::context::EmulatorContext>();
        pEmulatorContext.SetMemoryModified();
    }

    WindowViewModelBase::OnValueChanged(args);
}

void AssetListViewModel::ApplyFilter()
{
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
    m_vFilteredAssets.BeginUpdate();

    // first pass: remove any filtered items no longer in the source collection
    for (gsl::index nIndex = gsl::narrow_cast<gsl::index>(m_vFilteredAssets.Count()) -1; nIndex >= 0; --nIndex)
    {
        auto* pItem = m_vFilteredAssets.GetItemAt(nIndex);
        if (pItem != nullptr)
        {
            if (!pGameContext.Assets().FindAsset(pItem->GetType(), pItem->GetId()))
                m_vFilteredAssets.RemoveAt(nIndex);
        }
    }

    // second pass: update visibility of each item in the source collection
    for (gsl::index nIndex = 0; nIndex < gsl::narrow_cast<gsl::index>(pGameContext.Assets().Count()); ++nIndex)
    {
        const auto* pAsset = pGameContext.Assets().GetItemAt(nIndex);
        if (pAsset != nullptr)
            AddOrRemoveFilteredItem(*pAsset);
    }

    m_vFilteredAssets.EndUpdate();

    UpdateTotals();
    UpdateButtons();
}

void AssetListViewModel::EnsureAppearsInFilteredList(const ra::data::models::AssetModelBase& pAsset)
{
    // if the filter category is not all, ensure it matches the asset
    const auto nFilterCategory = GetFilterCategory();
    if (nFilterCategory != FilterCategory::All)
    {
        const auto nAssetCategory = ra::itoe<ra::data::models::AssetCategory>(ra::etoi(nFilterCategory));
        if (pAsset.GetCategory() != nAssetCategory)
            SetValue(FilterCategoryProperty, ra::etoi(pAsset.GetCategory()));
    }

    // if there's a special filter and it doesn't match, clear the special filter
    if (GetSpecialFilter() != SpecialFilter::All && !MatchesFilter(pAsset))
        SetSpecialFilter(ra::ui::viewmodels::AssetListViewModel::SpecialFilter::All);
}

bool AssetListViewModel::MatchesFilter(const ra::data::models::AssetModelBase& pAsset)
{
    const auto nFilterCategory = GetFilterCategory();
    if (nFilterCategory != FilterCategory::All)
    {
        const auto nAssetCategory = ra::itoe<ra::data::models::AssetCategory>(ra::etoi(nFilterCategory));
        if (pAsset.GetCategory() != nAssetCategory)
            return false;
    }

    if (pAsset.GetType() == ra::data::models::AssetType::LocalBadges)
        return false;

    switch (GetSpecialFilter())
    {
        default:
            break;

        case SpecialFilter::Active:
            if (!pAsset.IsActive())
                return false;
            break;

        case SpecialFilter::Inactive:
            if (pAsset.IsActive())
                return false;
            break;

        case SpecialFilter::Modified:
            if (!pAsset.IsModified())
                return false;
            break;

        case SpecialFilter::Unpublished:
            if (pAsset.GetChanges() != ra::data::models::AssetChanges::Unpublished)
                return false;
            break;

        case SpecialFilter::Authored:
            if (pAsset.GetAuthor() != ra::Widen(ra::services::ServiceLocator::Get<ra::data::context::UserContext>().GetUsername()))
                return false;
            break;
    }

    return true;
}

void AssetListViewModel::AddOrRemoveFilteredItem(gsl::index nAssetIndex)
{
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
    const auto* pAsset = pGameContext.Assets().GetItemAt(nAssetIndex);
    if (pAsset != nullptr)
    {
        if (AddOrRemoveFilteredItem(*pAsset))
        {
            UpdateTotals();
            UpdateButtons();
        }
    }
}

bool AssetListViewModel::AddOrRemoveFilteredItem(const ra::data::models::AssetModelBase& pAsset)
{
    const auto nIndex = GetFilteredAssetIndex(pAsset);

    if (MatchesFilter(pAsset))
    {
        if (nIndex < 0)
        {
            auto pSummary = std::make_unique<AssetSummaryViewModel>();
            pSummary->SetId(ra::to_signed(pAsset.GetID()));
            pSummary->SetLabel(pAsset.GetName());
            pSummary->SetType(pAsset.GetType());
            pSummary->SetCategory(pAsset.GetCategory());
            pSummary->SetChanges(pAsset.GetChanges());
            pSummary->SetState(pAsset.GetState());

            const auto* pAchievement = dynamic_cast<const ra::data::models::AchievementModel*>(&pAsset);
            if (pAchievement != nullptr)
                pSummary->SetPoints(pAchievement->GetPoints());

            m_vFilteredAssets.Append(std::move(pSummary));
            return true;
        }
    }
    else
    {
        if (nIndex >= 0)
        {
            const bool bIsSelected = m_vFilteredAssets.GetItemValue(nIndex, AssetSummaryViewModel::IsSelectedProperty);

            m_vFilteredAssets.RemoveAt(nIndex);
            return true;
        }
    }

    return false;
}

gsl::index AssetListViewModel::GetFilteredAssetIndex(const ra::data::models::AssetModelBase& pAsset) const
{
    const auto nId = ra::to_signed(pAsset.GetID());
    const auto nType = pAsset.GetType();

    for (gsl::index nIndex = 0; nIndex < gsl::narrow_cast<gsl::index>(m_vFilteredAssets.Count()); ++nIndex)
    {
        auto* pItem = m_vFilteredAssets.GetItemAt(nIndex);
        if (pItem != nullptr && pItem->GetId() == nId && pItem->GetType() == nType)
            return nIndex;
    }

    return -1;
}

void AssetListViewModel::OpenEditor(const AssetSummaryViewModel* pAsset)
{
    auto& pEmulatorContext = ra::services::ServiceLocator::GetMutable<ra::data::context::EmulatorContext>();
    if (!pEmulatorContext.WarnDisableHardcoreMode("edit assets"))
        return;

    ra::data::models::AssetModelBase* vmAsset = nullptr;

    if (pAsset)
    {
        auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::context::GameContext>();
        vmAsset = pGameContext.Assets().FindAsset(pAsset->GetType(), pAsset->GetId());
    }

    auto& pWindowManager = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::WindowManager>();
    pWindowManager.AssetEditor.LoadAsset(vmAsset);

    if (!pWindowManager.AssetEditor.IsVisible())
        pWindowManager.AssetEditor.Show();
}

bool AssetListViewModel::HasSelection(ra::data::models::AssetType nAssetType) const
{
    for (gsl::index nIndex = 0; nIndex < gsl::narrow_cast<gsl::index>(m_vFilteredAssets.Count()); ++nIndex)
    {
        const auto* pItem = m_vFilteredAssets.GetItemAt(nIndex);
        if (pItem != nullptr && pItem->IsSelected())
        {
            if (nAssetType == ra::data::models::AssetType::None || nAssetType == pItem->GetType())
                return true;
        }
    }

    return false;
}

void AssetListViewModel::FilteredListMonitor::OnViewModelBoolValueChanged(_UNUSED gsl::index nIndex, const BoolModelProperty::ChangeArgs& args)
{
    if (args.Property == AssetSummaryViewModel::IsSelectedProperty)
    {
        if (m_pOwner->FilteredAssets().IsUpdating())
            m_bUpdateButtonsPending = true;
        else
            m_pOwner->UpdateButtons();
    }
}

void AssetListViewModel::FilteredListMonitor::OnBeginViewModelCollectionUpdate() noexcept
{
    m_bUpdateButtonsPending = false;
}

void AssetListViewModel::FilteredListMonitor::OnEndViewModelCollectionUpdate()
{
    if (m_bUpdateButtonsPending)
    {
        m_bUpdateButtonsPending = false;
        m_pOwner->UpdateButtons();
    }
}

void AssetListViewModel::UpdateButtons()
{
    if (m_bNeedToUpdateButtons.exchange(true) == false)
    {
        // wait 300ms before updating the buttons in case more changes are coming
        ra::services::ServiceLocator::GetMutable<ra::services::IThreadPool>().ScheduleAsync(
            std::chrono::milliseconds(300), [this]()
        {
            if (m_bNeedToUpdateButtons.exchange(false) == true)
                DoUpdateButtons();
        });
    }
}

void AssetListViewModel::DoUpdateButtons()
{
    bool bHasCoreSelection = false;
    bool bHasUnofficialSelection = false;
    bool bHasLocalSelection = false;
    bool bHasActiveSelection = false;
    bool bHasInactiveSelection = false;
    bool bHasModifiedSelection = false;
    bool bHasUnpublishedSelection = false;
    bool bHasNonNewSelection = false;
    bool bHasSelection = false;
    bool bHasModified = false;
    bool bHasUnpublished = false;
    bool bHasUnofficial = false;
    const bool bHardcore = _RA_HardcoreModeIsActive();

    const bool bGameLoaded = (GetGameId() != 0);
    if (!bGameLoaded)
    {
        SetValue(CanCreateProperty, false);
        SetValue(CanActivateProperty, false);
        SetValue(ActivateButtonTextProperty, ActivateButtonTextProperty.GetDefaultValue());
    }
    else
    {
        SetValue(CanCreateProperty, !bHardcore);
        SetValue(CanActivateProperty, m_vFilteredAssets.Count() > 0);

        for (size_t i = 0; i < m_vFilteredAssets.Count(); ++i)
        {
            const auto* pItem = m_vFilteredAssets.GetItemAt(i);
            if (pItem != nullptr)
            {
                switch (pItem->GetChanges())
                {
                    case ra::data::models::AssetChanges::Modified:
                    case ra::data::models::AssetChanges::New:
                        bHasModified = true;
                        break;
                    case ra::data::models::AssetChanges::Unpublished:
                        bHasUnpublished = true;
                        break;
                    default:
                        break;
                }

                if (pItem->IsSelected())
                {
                    bHasSelection = true;

                    switch (pItem->GetCategory())
                    {
                        case ra::data::models::AssetCategory::Core:
                            bHasCoreSelection = true;
                            break;
                        case ra::data::models::AssetCategory::Unofficial:
                            bHasUnofficial = true;
                            bHasUnofficialSelection = true;
                            break;
                        case ra::data::models::AssetCategory::Local:
                            bHasLocalSelection = true;
                            break;
                        default:
                            break;
                    }

                    if (ra::data::models::AssetModelBase::IsActive(pItem->GetState()))
                        bHasActiveSelection = true;
                    else
                        bHasInactiveSelection = true;

                    switch (pItem->GetChanges())
                    {
                        case ra::data::models::AssetChanges::New:
                            bHasModifiedSelection = true;
                            break;
                        case ra::data::models::AssetChanges::Modified:
                            bHasNonNewSelection = true;
                            bHasModifiedSelection = true;
                            break;
                        case ra::data::models::AssetChanges::Unpublished:
                            bHasNonNewSelection = true;
                            bHasUnpublishedSelection = true;
                            break;
                        default:
                            bHasNonNewSelection = true;
                            break;
                    }
                }
                else
                {
                    switch (pItem->GetCategory())
                    {
                        case ra::data::models::AssetCategory::Unofficial:
                            bHasUnofficial = true;
                            break;
                        default:
                            break;
                    }
                }
            }
        }
    }

    if (bHasInactiveSelection)
    {
        SetValue(ActivateButtonTextProperty, L"&Activate");
        SetValue(CanCloneProperty, !bHardcore);
    }
    else if (bHasActiveSelection)
    {
        SetValue(ActivateButtonTextProperty, L"De&activate");
        SetValue(CanCloneProperty, !bHardcore);
    }
    else
    {
        Expects(!bHasSelection); // a selected item should be tallied as either active or inactive
        SetValue(ActivateButtonTextProperty, ActivateButtonTextProperty.GetDefaultValue());
        SetValue(CanCloneProperty, false);
    }

    if (bHasModifiedSelection)
    {
        SetValue(SaveButtonTextProperty, L"&Save");
        SetValue(CanSaveProperty, true);
    }
    else if (bHasUnpublishedSelection)
    {
        SetValue(SaveButtonTextProperty, L"Publi&sh");
        SetValue(CanSaveProperty, true);
    }
    else if (bHasUnofficialSelection)
    {
        SetValue(SaveButtonTextProperty, L"Pro&mote");
        SetValue(CanSaveProperty, true);
    }
    else if (bHasCoreSelection)
    {
        SetValue(SaveButtonTextProperty, L"De&mote");
        SetValue(CanSaveProperty, true);
    }
    else if (bHasSelection)
    {
        SetValue(SaveButtonTextProperty, L"&Save");
        SetValue(CanSaveProperty, false);
    }
    else if (bHasModified)
    {
        SetValue(SaveButtonTextProperty, SaveButtonTextProperty.GetDefaultValue());
        SetValue(CanSaveProperty, true);
    }
    else if (bHasUnpublished)
    {
        SetValue(SaveButtonTextProperty, L"Publi&sh All");
        SetValue(CanSaveProperty, true);
    }
    else if (bHasUnofficial)
    {
        SetValue(SaveButtonTextProperty, L"Pro&mote All");
        SetValue(CanSaveProperty, true);
    }
    else
    {
        SetValue(SaveButtonTextProperty, SaveButtonTextProperty.GetDefaultValue());
        SetValue(CanSaveProperty, false);
    }

    if (bGameLoaded)
    {
        if (bHasSelection)
        {
            SetValue(ResetButtonTextProperty, L"&Reset");
            SetValue(CanResetProperty, bHasNonNewSelection);
        }
        else
        {
            SetValue(ResetButtonTextProperty, ResetButtonTextProperty.GetDefaultValue());
            SetValue(CanResetProperty, true);
        }
    }
    else
    {
        SetValue(ResetButtonTextProperty, ResetButtonTextProperty.GetDefaultValue());
        SetValue(CanResetProperty, false);
    }

    if (bGameLoaded)
    {
        if (bHasCoreSelection || bHasUnofficialSelection)
            SetValue(RevertButtonTextProperty, L"Re&vert");
        else if (bHasLocalSelection)
            SetValue(RevertButtonTextProperty, L"&Delete");
        else
            SetValue(RevertButtonTextProperty, RevertButtonTextProperty.GetDefaultValue());

        SetValue(CanRevertProperty, true);
    }
    else
    {
        SetValue(RevertButtonTextProperty, RevertButtonTextProperty.GetDefaultValue());
        SetValue(CanRevertProperty, false);
    }
}

void AssetListViewModel::GetSelectedAssets(std::vector<ra::data::models::AssetModelBase*>& vSelectedAssets,
    std::function<bool(const ra::data::models::AssetModelBase&)> pFilterFunction)
{
    auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::context::GameContext>();

    for (size_t i = 0; i < m_vFilteredAssets.Count(); ++i)
    {
        const auto* pItem = m_vFilteredAssets.GetItemAt(i);
        if (pItem != nullptr && pItem->IsSelected())
        {
            auto* pAsset = pGameContext.Assets().FindAsset(pItem->GetType(), pItem->GetId());
            if (pAsset != nullptr)
            {
                if (!pFilterFunction || pFilterFunction(*pAsset))
                    vSelectedAssets.push_back(pAsset);
            }
        }
    }

    // no selected assets found, return all assets
    if (vSelectedAssets.empty())
    {
        for (size_t i = 0; i < m_vFilteredAssets.Count(); ++i)
        {
            const auto* pItem = m_vFilteredAssets.GetItemAt(i);
            if (pItem != nullptr)
            {
                auto* pAsset = pGameContext.Assets().FindAsset(pItem->GetType(), pItem->GetId());
                if (pAsset != nullptr)
                {
                    if (!pFilterFunction || pFilterFunction(*pAsset))
                        vSelectedAssets.push_back(pAsset);
                }
            }
        }
    }
}

bool AssetListViewModel::SelectionContainsInvalidAsset(const std::vector<ra::data::models::AssetModelBase*>& vSelectedAssets, _Out_ std::wstring& sErrorMessage) const
{
    auto& pWindowManager = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::WindowManager>();
    if (pWindowManager.AssetEditor.HasAssetValidationError())
    {
        const auto* vmInvalidAsset = pWindowManager.AssetEditor.GetAsset();
        for (const auto* vmItem : vSelectedAssets)
        {
            if (vmItem == vmInvalidAsset)
            {
                sErrorMessage = ra::StringPrintf(L"The following errors must be corrected:\n* %s",
                    pWindowManager.AssetEditor.GetAssetValidationError());

                return true;
            }
        }
    }

    sErrorMessage.clear();
    return false;
}

void AssetListViewModel::ActivateSelected()
{
    std::vector<ra::data::models::AssetModelBase*> vSelectedAssets;
    GetSelectedAssets(vSelectedAssets);

    if (GetActivateButtonText().at(0) == 'D') // Deactivate
    {
        bool bCoreAssetSelected = false;
        for (const auto* vmItem : vSelectedAssets)
        {
            if (vmItem && vmItem->GetCategory() == ra::data::models::AssetCategory::Core)
            {
                bCoreAssetSelected = true;
                break;
            }
        }

        if (bCoreAssetSelected)
        {
            auto& pEmulatorContext = ra::services::ServiceLocator::GetMutable<ra::data::context::EmulatorContext>();
            if (!pEmulatorContext.WarnDisableHardcoreMode("deactivate core achievements"))
                return;
        }

        for (auto* vmItem : vSelectedAssets)
        {
            if (vmItem)
                vmItem->Deactivate();
        }
    }
    else // Activate
    {
        std::wstring sErrorMessage;
        if (SelectionContainsInvalidAsset(vSelectedAssets, sErrorMessage))
        {
            ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(L"Unable to activate", sErrorMessage);
            return;
        }

        for (auto* vmItem : vSelectedAssets)
        {
            if (vmItem)
                vmItem->Activate();
        }
    }
}

void AssetListViewModel::SaveSelected()
{
    auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::context::GameContext>();
    std::vector<ra::data::models::AssetModelBase*> vSelectedAssets;

    const auto sSaveButtonText = GetSaveButtonText();
    if (sSaveButtonText.at(0) == '&') // "&Save" / "&Save All"
    {
        // save - find the selected items (allow empty to mean all)
        for (size_t i = 0; i < m_vFilteredAssets.Count(); ++i)
        {
            const auto* pItem = m_vFilteredAssets.GetItemAt(i);
            if (pItem != nullptr && pItem->IsSelected())
            {
                auto* pAsset = pGameContext.Assets().FindAsset(pItem->GetType(), pItem->GetId());
                if (pAsset != nullptr)
                    vSelectedAssets.push_back(pAsset);
            }
        }

        std::wstring sErrorMessage;
        if (SelectionContainsInvalidAsset(vSelectedAssets, sErrorMessage))
        {
            ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(L"Unable to save", sErrorMessage);
            return;
        }

        if (vSelectedAssets.empty())
        {
            RA_LOG_INFO("Saving all %u assets", pGameContext.Assets().Count());
        }
        else
        {
            std::string sMessage;
            for (const auto* pAsset : vSelectedAssets)
            {
                if (!sMessage.empty())
                    sMessage.push_back(',');
                sMessage.append(std::to_string(pAsset->GetID()));
            }
            RA_LOG_INFO("Saving %u assets: %s", vSelectedAssets.size(), sMessage);
        }
    }
    else if (sSaveButtonText.at(0) == 'P' && sSaveButtonText.at(1) == 'u') // "Publi&sh" / "Publi&sh All"
    {
        // publish - find the selected unpublished items
        GetSelectedAssets(vSelectedAssets, [](const ra::data::models::AssetModelBase& pModel)
        {
            return pModel.GetChanges() == ra::data::models::AssetChanges::Unpublished;
        });

        if (vSelectedAssets.empty())
            return;

        if (!ValidateAssetsForCore(vSelectedAssets, true))
            return;

        ra::ui::viewmodels::MessageBoxViewModel vmMessageBox;
        vmMessageBox.SetMessage(ra::StringPrintf(L"Are you sure you want to publish %d items?", vSelectedAssets.size()));
        vmMessageBox.SetButtons(ra::ui::viewmodels::MessageBoxViewModel::Buttons::YesNo);
        if (vmMessageBox.ShowModal(*this) != DialogResult::Yes)
            return;

        RA_LOG_INFO("Publishing %u items", vSelectedAssets.size());
        Publish(vSelectedAssets);
    }
    else if (sSaveButtonText.at(0) == 'P') // "Pro&mote" / "Pro&mote All"
    {
        // promote - find the selected unpublished items
        GetSelectedAssets(vSelectedAssets, [](const ra::data::models::AssetModelBase& pModel)
        {
            return (pModel.GetChanges() == ra::data::models::AssetChanges::None &&
                pModel.GetCategory() == ra::data::models::AssetCategory::Unofficial);
        });

        if (vSelectedAssets.empty())
            return;

        if (!ValidateAssetsForCore(vSelectedAssets, false))
            return;

        ra::ui::viewmodels::MessageBoxViewModel vmMessageBox;
        vmMessageBox.SetHeader(ra::StringPrintf(L"Are you sure you want to promote %d items to core?", vSelectedAssets.size()));
        vmMessageBox.SetMessage(L"Items in core are officially available for players to earn.");
        vmMessageBox.SetButtons(ra::ui::viewmodels::MessageBoxViewModel::Buttons::YesNo);
        if (vmMessageBox.ShowModal(*this) != DialogResult::Yes)
            return;

        for (auto* pAsset : vSelectedAssets)
            pAsset->SetCategory(ra::data::models::AssetCategory::Core);

        RA_LOG_INFO("Promoting %u items", vSelectedAssets.size());
        Publish(vSelectedAssets);
    }
    else if (sSaveButtonText.at(0) == 'D') // "De&mote"
    {
        // demote - find the selected published items
        GetSelectedAssets(vSelectedAssets, [](const ra::data::models::AssetModelBase& pModel)
        {
            return (pModel.GetChanges() == ra::data::models::AssetChanges::None &&
                pModel.GetCategory() == ra::data::models::AssetCategory::Core);
        });

        if (vSelectedAssets.empty())
            return;

        ra::ui::viewmodels::MessageBoxViewModel vmMessageBox;
        vmMessageBox.SetIcon(ra::ui::viewmodels::MessageBoxViewModel::Icon::Warning);
        vmMessageBox.SetHeader(ra::StringPrintf(L"Are you sure you want to demote %d items to unofficial?", vSelectedAssets.size()));
        vmMessageBox.SetMessage(L"Items in unofficial can no longer be earned by players.");
        vmMessageBox.SetButtons(ra::ui::viewmodels::MessageBoxViewModel::Buttons::YesNo);
        if (vmMessageBox.ShowModal(*this) != DialogResult::Yes)
            return;

        for (auto* pAsset : vSelectedAssets)
            pAsset->SetCategory(ra::data::models::AssetCategory::Unofficial);

        RA_LOG_INFO("Demoting %u items", vSelectedAssets.size());
        Publish(vSelectedAssets);
    }

    // update the local file
    pGameContext.Assets().SaveAssets(vSelectedAssets);
    UpdateButtons();
}

#if RA_INTEGRATION_VERSION_MINOR < 79
static std::wstring ValidateCondSet(const rc_condset_t* pCondSet)
{
    if (!pCondSet)
        return L"";

    for (const auto* pCondition = pCondSet->conditions; pCondition != nullptr; pCondition = pCondition->next)
    {
        if (pCondition->type == RC_CONDITION_RESET_NEXT_IF)
            return L"ResetNextIf is pre-release functionality";
        if (pCondition->type == RC_CONDITION_TRIGGER)
            return L"Trigger is pre-release functionality";
        if (pCondition->type == RC_CONDITION_SUB_HITS)
            return L"SubHits is pre-release functionality";
        if (pCondition->oper == RC_OPERATOR_MULT)
            return L"Multiplication is pre-release functionality";
        if (pCondition->oper == RC_OPERATOR_DIV)
            return L"Division is pre-release functionality";
        if (pCondition->oper == RC_OPERATOR_AND)
            return L"Bitwise And is pre-release functionality";
    }

    return L"";
}

static std::wstring ValidateTriggerLogic(const std::string& sTrigger)
{
    const auto nSize = rc_trigger_size(sTrigger.c_str());
    if (nSize < 0)
        return ra::StringPrintf(L"Parse Error %d: %s", nSize, rc_error_str(nSize));

    std::string sBuffer;
    sBuffer.resize(nSize);
    const auto* pTrigger = rc_parse_trigger(sBuffer.data(), sTrigger.c_str(), nullptr, 0);

    std::wstring sError = ValidateCondSet(pTrigger->requirement);
    if (sError.empty())
    {
        const auto* pAlt = pTrigger->alternative;
        while (pAlt != nullptr)
        {
            sError = ValidateCondSet(pAlt);
            if (!sError.empty())
                break;

            pAlt = pAlt->next;
        }
    }

    return sError;
}
#endif

void AssetListViewModel::ValidateAchievementForCore(_UNUSED std::wstring& sError, _UNUSED const ra::data::models::AchievementModel& pAchievement) const
{
#if RA_INTEGRATION_VERSION_MINOR < 79
    if (!ra::services::ServiceLocator::Get<ra::services::IConfiguration>().IsCustomHost())
    {
        std::wstring sTriggerError = ValidateTriggerLogic(pAchievement.GetTrigger());
        if (!sTriggerError.empty())
            sError.append(ra::StringPrintf(L"\n* %s: %s", pAchievement.GetName(), sTriggerError));
    }
#endif
}

bool AssetListViewModel::ValidateAssetsForCore(std::vector<ra::data::models::AssetModelBase*>& vAssets, bool bCoreOnly)
{
    std::wstring sError;

    for (const auto* pAsset : vAssets)
    {
        const auto* pAchievement = dynamic_cast<const ra::data::models::AchievementModel*>(pAsset);
        if (pAchievement != nullptr)
            ValidateAchievementForCore(sError, *pAchievement);
    }

    if (!sError.empty())
    {
        sError.insert(0, L"The following items could not be published:");
        ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(bCoreOnly ? L"Publish aborted." : L"Promote to core aborted.", sError);
        return false;
    }

    return true;
}

void AssetListViewModel::Publish(std::vector<ra::data::models::AssetModelBase*>& vAssets)
{
    AssetUploadViewModel vmAssetUpload;
    for (auto* pAsset : vAssets)
    {
        Expects(pAsset != nullptr);
        vmAssetUpload.QueueAsset(*pAsset);
    }

    vmAssetUpload.ShowModal(*this);

    // if anything failed, revert to pre-publish state
    // this assumes that publish can only be called on committed assets
    for (auto* pAsset : vAssets)
    {
        Expects(pAsset != nullptr);
        if (pAsset->GetChanges() != ra::data::models::AssetChanges::None)
            pAsset->RestoreLocalCheckpoint();
    }

    vmAssetUpload.ShowResults();
}

void AssetListViewModel::ResetSelected()
{
    if (!CanReset())
        return;

    bool bCoreAssetSelected = false;
    std::vector<const AssetSummaryViewModel*> vSelectedAssets;
    for (gsl::index nIndex = 0; nIndex < gsl::narrow_cast<gsl::index>(m_vFilteredAssets.Count()); ++nIndex)
    {
        const auto* pItem = m_vFilteredAssets.GetItemAt(nIndex);
        if (pItem != nullptr && pItem->IsSelected())
        {
            vSelectedAssets.push_back(pItem);

            bCoreAssetSelected |= (pItem->GetCategory() == ra::data::models::AssetCategory::Core);
        }
    }

    if (bCoreAssetSelected)
    {
        auto& pEmulatorContext = ra::services::ServiceLocator::GetMutable<ra::data::context::EmulatorContext>();
        if (!pEmulatorContext.WarnDisableHardcoreMode("reset core achievements"))
            return;
    }

    ra::ui::viewmodels::MessageBoxViewModel vmMessageBox;
    vmMessageBox.SetHeader(L"Reload from disk?");
    vmMessageBox.SetIcon(ra::ui::viewmodels::MessageBoxViewModel::Icon::Warning);
    vmMessageBox.SetButtons(ra::ui::viewmodels::MessageBoxViewModel::Buttons::YesNo);

    if (vSelectedAssets.empty())
        vmMessageBox.SetMessage(L"This will discard all unsaved changes and reset the assets to the last locally saved state.");
    else
        vmMessageBox.SetMessage(L"This will discard any changes to the selected assets and reset them to the last locally saved state.");

    if (vmMessageBox.ShowModal() == DialogResult::No)
        return;

    std::vector<ra::AchievementID> vActiveAchievementsBefore;

    auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::context::GameContext>();
    pGameContext.Assets().BeginUpdate();

    std::vector<ra::data::models::AssetModelBase*> vAssetsToReset;
    if (vSelectedAssets.empty())
    {
        RA_LOG_INFO("Resetting all assets");

        // reset all - first remove any "new" items
        for (gsl::index nIndex = gsl::narrow_cast<gsl::index>(pGameContext.Assets().Count()) - 1; nIndex >= 0; --nIndex)
        {
            auto* pAsset = pGameContext.Assets().GetItemAt(nIndex);
            if (pAsset != nullptr)
            {
                if (pAsset->GetType() == ra::data::models::AssetType::Achievement && pAsset->IsActive())
                    vActiveAchievementsBefore.push_back(pAsset->GetID());

                if (pAsset->GetChanges() == ra::data::models::AssetChanges::New)
                    pGameContext.Assets().RemoveAt(nIndex);
            }
        }

        // when resetting all, always read the file to pick up new items
        pGameContext.Assets().ReloadAssets(vAssetsToReset);
    }
    else
    {
        // reset selection, remove "new" items and get the AssetViewModel for the others
        for (auto* pItem : vSelectedAssets)
        {
            const auto nType = pItem->GetType();
            const auto nId = ra::to_unsigned(pItem->GetId());
            for (gsl::index nIndex = gsl::narrow_cast<gsl::index>(pGameContext.Assets().Count()) - 1; nIndex >= 0; --nIndex)
            {
                auto* pAsset = pGameContext.Assets().GetItemAt(nIndex);
                if (pAsset->GetID() == nId && pAsset->GetType() == nType)
                {
                    if (nType == ra::data::models::AssetType::Achievement && pAsset->IsActive())
                        vActiveAchievementsBefore.push_back(nId);

                    if (pAsset->GetChanges() == ra::data::models::AssetChanges::New)
                        pGameContext.Assets().RemoveAt(nIndex);
                    else
                        vAssetsToReset.push_back(pAsset);
                    break;
                }
            }
        }

        // if there were any non-new items, reload them from disk
        if (!vAssetsToReset.empty())
        {
            std::string sMessage;
            for (const auto* pAsset : vAssetsToReset)
            {
                if (!sMessage.empty())
                    sMessage.push_back(',');
                sMessage.append(std::to_string(pAsset->GetID()));
            }
            RA_LOG_INFO("Resetting %zu assets: %s", vAssetsToReset.size(), sMessage);

            pGameContext.Assets().ReloadAssets(vAssetsToReset);
        }
    }

    // if the asset that is loaded in the editor no longer exists, clear out the editor
    // do this before EndUpdate so the asset won't have been destroyed yet
    auto& pWindowManager = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::WindowManager>();
    const auto* pEditorAsset = pWindowManager.AssetEditor.GetAsset();
    if (pEditorAsset != nullptr && !pGameContext.Assets().FindAsset(pEditorAsset->GetType(), pEditorAsset->GetID()))
        pWindowManager.AssetEditor.LoadAsset(nullptr);

    pGameContext.Assets().EndUpdate();

    // if any achievements were deleted, disable their challenge indicators
    for (auto nId : vActiveAchievementsBefore)
    {
        if (!pGameContext.Assets().FindAchievement(nId))
        {
            // when an active achievement is deleted, the challenge indicator needs to be hidden as no event will be raised
            auto& pOverlayManager = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>();
            pOverlayManager.RemoveChallengeIndicator(nId);
        }
    }
}

void AssetListViewModel::RevertSelected()
{
    if (!CanRevert())
        return;

    std::vector<ra::data::models::AssetModelBase*> vAssetsToReset;
    GetSelectedAssets(vAssetsToReset);

    bool bLocalAssetSelected = false;
    bool bCoreAssetSelected = false;
    for (const auto* pAsset : vAssetsToReset)
    {
        if (pAsset == nullptr)
            continue;

        switch (pAsset->GetCategory())
        {
            case ra::data::models::AssetCategory::Local:
                bLocalAssetSelected = true;
                break;

            case ra::data::models::AssetCategory::Core:
                bCoreAssetSelected = true;
                break;
        }
    }

    if (bCoreAssetSelected)
    {
        auto& pEmulatorContext = ra::services::ServiceLocator::GetMutable<ra::data::context::EmulatorContext>();
        if (!pEmulatorContext.WarnDisableHardcoreMode("revert core achievements"))
            return;
    }

    ra::ui::viewmodels::MessageBoxViewModel vmMessageBox;
    vmMessageBox.SetIcon(ra::ui::viewmodels::MessageBoxViewModel::Icon::Warning);
    vmMessageBox.SetButtons(ra::ui::viewmodels::MessageBoxViewModel::Buttons::YesNo);

    std::wstring sWarningMessage;

    if (GetRevertButtonText().at(0) == 'R') // Revert
    {
        vmMessageBox.SetHeader(L"Revert from server?");

        if (vAssetsToReset.size() == m_vFilteredAssets.Count())
            sWarningMessage = L"This will discard all local changes and revert the assets to the last state retrieved from the server.";
        else
            sWarningMessage = L"This will discard any local changes to the selected assets and revert them to the last state retrieved from the server.";

        if (bLocalAssetSelected)
            sWarningMessage += L"\n\n";
    }
    else // Delete
    {
        vmMessageBox.SetHeader(L"Permanently delete?");

        Expects(bLocalAssetSelected); // message is shared with text appended to Revert message
    }

    if (bLocalAssetSelected)
        sWarningMessage += L"Assets not available on the server will be permanently deleted.";
    vmMessageBox.SetMessage(sWarningMessage);

    if (vmMessageBox.ShowModal() == DialogResult::No)
        return;

    std::string sMessage;
    for (const auto* pAsset : vAssetsToReset)
    {
        if (!sMessage.empty())
            sMessage.push_back(',');
        sMessage.append(std::to_string(pAsset->GetID()));
    }
    RA_LOG_INFO("Reverting %zu assets: %s", vAssetsToReset.size(), sMessage);

    auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::context::GameContext>();
    auto& pAssets = pGameContext.Assets();
    pAssets.BeginUpdate();

    for (auto* pAsset : vAssetsToReset)
    {
        Expects(pAsset != nullptr);
        if (pAsset->GetCategory() == ra::data::models::AssetCategory::Local)
        {
            // reverting a local item deletes it
            pAsset->SetDeleted();

            // when an active achievement is deleted, the challenge indicator needs to be hidden as no event will be raised
            if (pAsset->IsActive() && pAsset->GetType() == ra::data::models::AssetType::Achievement)
            {
                auto& pOverlayManager = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>();
                pOverlayManager.RemoveChallengeIndicator(pAsset->GetID());
            }

            // if the asset is open in the editor, select no asset
            auto& pWindowManager = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::WindowManager>();
            if (pWindowManager.AssetEditor.GetAsset() == pAsset)
                pWindowManager.AssetEditor.LoadAsset(nullptr);
        }
        else
        {
            // Assert: New items should always be in the Local category
            Expects(pAsset->GetChanges() != ra::data::models::AssetChanges::New);

            // reverting a non-local item restores the server state
            pAsset->RestoreServerCheckpoint();
        }
    }

    pAssets.EndUpdate();

    // update the local file
    pAssets.SaveAssets(vAssetsToReset);
    UpdateButtons();
}

void AssetListViewModel::CreateNew()
{
    if (!CanCreate())
        return;

    RA_LOG_INFO("Creating new achievement");

    auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::context::GameContext>();
    const auto& pUserContext = ra::services::ServiceLocator::GetMutable<ra::data::context::UserContext>();

    FilteredAssets().BeginUpdate();

    auto& vmAchievement = pGameContext.Assets().NewAchievement();
    vmAchievement.SetCategory(ra::data::models::AssetCategory::Local);
    vmAchievement.SetPoints(0);
    vmAchievement.SetAuthor(ra::Widen(pUserContext.GetUsername()));
    vmAchievement.UpdateServerCheckpoint();
    vmAchievement.SetNew();

    EnsureAppearsInFilteredList(vmAchievement);

    const auto nId = ra::to_signed(vmAchievement.GetID());

    // select the new viewmodel, and deselect everything else
    gsl::index nIndex = -1;
    for (gsl::index i = 0; i < ra::to_signed(m_vFilteredAssets.Count()); ++i)
    {
        auto* pItem = m_vFilteredAssets.GetItemAt(i);
        if (pItem != nullptr)
        {
            if (pItem->GetId() == nId && pItem->GetType() == ra::data::models::AssetType::Achievement)
            {
                pItem->SetSelected(true);
                nIndex = i;
            }
            else
            {
                pItem->SetSelected(false);
            }
        }
    }

    FilteredAssets().EndUpdate();

    UpdateTotals();

    SetValue(EnsureVisibleAssetIndexProperty, gsl::narrow_cast<int>(nIndex));

    auto* vmAsset = m_vFilteredAssets.GetItemAt(nIndex);
    if (vmAsset)
        OpenEditor(vmAsset);
}

void AssetListViewModel::CloneSelected()
{
    if (!CanClone())
        return;

    std::vector<ra::data::models::AssetModelBase*> vSelectedAssets;
    GetSelectedAssets(vSelectedAssets);
    if (vSelectedAssets.empty())
        return;

    std::wstring sErrorMessage;
    if (SelectionContainsInvalidAsset(vSelectedAssets, sErrorMessage))
    {
        ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(L"Unable to clone", sErrorMessage);
        return;
    }

    {
        std::string sMessage;
        for (const auto* pAsset : vSelectedAssets)
        {
            if (!sMessage.empty())
                sMessage.push_back(',');
            sMessage.append(std::to_string(pAsset->GetID()));
        }
        RA_LOG_INFO("Cloning %u assets: %s", vSelectedAssets.size(), sMessage);
    }

    auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::context::GameContext>();

    FilteredAssets().BeginUpdate();

    // add the cloned items
    std::vector<int> vNewIDs;
    for (const auto* pAsset : vSelectedAssets)
    {
        const auto* pSourceAchievement = dynamic_cast<const ra::data::models::AchievementModel*>(pAsset);
        if (pSourceAchievement != nullptr)
        {
            auto& vmAchievement = pGameContext.Assets().NewAchievement();
            vmAchievement.SetCategory(ra::data::models::AssetCategory::Local);
            vmAchievement.UpdateServerCheckpoint();

            vmAchievement.SetName(pSourceAchievement->GetName() + L" (copy)");
            vmAchievement.SetDescription(pSourceAchievement->GetDescription());
            vmAchievement.SetBadge(pSourceAchievement->GetBadge());
            vmAchievement.SetPoints(pSourceAchievement->GetPoints());
            vmAchievement.SetTrigger(pSourceAchievement->GetTrigger());
            vmAchievement.SetNew();

            EnsureAppearsInFilteredList(vmAchievement);

            vNewIDs.push_back(vmAchievement.GetID());
        }
    }

    // select the new items and deselect everything else
    gsl::index nIndex = -1;
    for (gsl::index i = 0; i < ra::to_signed(m_vFilteredAssets.Count()); ++i)
    {
        auto* pItem = m_vFilteredAssets.GetItemAt(i);
        if (pItem != nullptr)
        {
            if (std::find(vNewIDs.begin(), vNewIDs.end(), pItem->GetId()) != vNewIDs.end() &&
                pItem->GetType() == ra::data::models::AssetType::Achievement)
            {
                nIndex = i;
                pItem->SetSelected(true);
            }
            else
            {
                pItem->SetSelected(false);
            }
        }
    }

    FilteredAssets().EndUpdate();

    UpdateTotals();

    SetValue(EnsureVisibleAssetIndexProperty, gsl::narrow_cast<int>(m_vFilteredAssets.Count()) - 1);

    // if only a single item was cloned, open it
    if (vNewIDs.size() == 1)
    {
        auto* vmAsset = m_vFilteredAssets.GetItemAt(nIndex);
        if (vmAsset)
            OpenEditor(vmAsset);
    }
}

} // namespace viewmodels
} // namespace ui
} // namespace ra
