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
const IntModelProperty AssetListViewModel::FilterCategoryProperty("AssetListViewModel", "FilterCategory", ra::etoi(ra::data::models::AssetCategory::Core));
const IntModelProperty AssetListViewModel::EnsureVisibleAssetIndexProperty("AssetListViewModel", "EnsureVisibleAssetIndex", -1);

AssetListViewModel::AssetListViewModel() noexcept
{
    SetWindowTitle(L"Assets List");

    m_vStates.Add(ra::etoi(ra::data::models::AssetState::Inactive), L"Inactive");
    m_vStates.Add(ra::etoi(ra::data::models::AssetState::Waiting), L"Waiting");
    m_vStates.Add(ra::etoi(ra::data::models::AssetState::Active), L"Active");
    m_vStates.Add(ra::etoi(ra::data::models::AssetState::Paused), L"Paused");
    m_vStates.Add(ra::etoi(ra::data::models::AssetState::Primed), L"Primed");
    m_vStates.Add(ra::etoi(ra::data::models::AssetState::Triggered), L"Triggered");
    m_vStates.Add(ra::etoi(ra::data::models::AssetState::Disabled), L"Disabled");

    m_vCategories.Add(ra::etoi(ra::data::models::AssetCategory::Core), L"Core");
    m_vCategories.Add(ra::etoi(ra::data::models::AssetCategory::Unofficial), L"Unofficial");
    m_vCategories.Add(ra::etoi(ra::data::models::AssetCategory::Local), L"Local");

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

static bool IsTalliedAchievement(const ra::data::models::AchievementModel& pAchievement)
{
    return (pAchievement.GetCategory() == ra::data::models::AssetCategory::Core);
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
        if (pAchievement != nullptr && IsTalliedAchievement(*pAchievement))
        {
            auto nPoints = GetTotalPoints();
            nPoints += args.tNewValue - args.tOldValue;
            SetValue(TotalPointsProperty, nPoints);
        }
    }
    else if (args.Property == ra::data::models::AssetModelBase::CategoryProperty)
    {
        UpdateTotals();

        const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
        const auto* pAsset = pGameContext.Assets().GetItemAt(nIndex);
        if (pAsset != nullptr)
            AddOrRemoveFilteredItem(*pAsset);
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
    {
        UpdateTotals();

        const auto* pAsset = pGameContext.Assets().GetItemAt(nIndex);
        if (pAsset != nullptr)
            AddOrRemoveFilteredItem(*pAsset);
    }
}

void AssetListViewModel::OnDataModelRemoved(_UNUSED gsl::index nIndex)
{
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
    if (!pGameContext.Assets().IsUpdating())
    {
        UpdateTotals();

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
    }
}

void AssetListViewModel::OnDataModelChanged(_UNUSED gsl::index nIndex)
{
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
    if (!pGameContext.Assets().IsUpdating())
    {
        UpdateTotals();

        const auto* pAsset = pGameContext.Assets().GetItemAt(nIndex);
        if (pAsset != nullptr)
            AddOrRemoveFilteredItem(*pAsset);
    }
}

void AssetListViewModel::OnEndDataModelCollectionUpdate()
{
    UpdateTotals();
    ApplyFilter();
}

void AssetListViewModel::UpdateTotals()
{
    int nAchievementCount = 0;
    int nTotalPoints = 0;

    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
    for (gsl::index nIndex = 0; nIndex < gsl::narrow_cast<gsl::index>(pGameContext.Assets().Count()); ++nIndex)
    {
        auto* pAchievement = dynamic_cast<const ra::data::models::AchievementModel*>(pGameContext.Assets().GetItemAt(nIndex));
        if (pAchievement != nullptr && IsTalliedAchievement(*pAchievement))
        {
            ++nAchievementCount;
            nTotalPoints += pAchievement->GetPoints();
        }
    }

    SetValue(AchievementCountProperty, nAchievementCount);
    SetValue(TotalPointsProperty, nTotalPoints);
}

void AssetListViewModel::OnValueChanged(const IntModelProperty::ChangeArgs& args)
{
    if (args.Property == FilterCategoryProperty)
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

    UpdateButtons();
}

bool AssetListViewModel::MatchesFilter(const ra::data::models::AssetModelBase& pAsset)
{
    if (pAsset.GetCategory() != GetFilterCategory())
        return false;

    if (pAsset.GetType() == ra::data::models::AssetType::LocalBadges)
        return false;

    return true;
}

void AssetListViewModel::AddOrRemoveFilteredItem(const ra::data::models::AssetModelBase& pAsset)
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
            UpdateButtons();
        }
    }
    else
    {
        if (nIndex >= 0)
        {
            const bool bIsSelected = m_vFilteredAssets.GetItemValue(nIndex, AssetSummaryViewModel::IsSelectedProperty);

            m_vFilteredAssets.RemoveAt(nIndex);
            UpdateButtons();
        }
    }
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
    bool bHasSelection = false;
    bool bHasModified = false;
    bool bHasUnpublished = false;
    bool bHasUnofficial = false;

    const bool bGameLoaded = (GetGameId() != 0);
    if (!bGameLoaded)
    {
        SetValue(CanCreateProperty, false);
        SetValue(CanActivateProperty, false);
        SetValue(ActivateButtonTextProperty, ActivateButtonTextProperty.GetDefaultValue());
    }
    else
    {
        SetValue(CanCreateProperty, true);
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

                    switch (pItem->GetState())
                    {
                        case ra::data::models::AssetState::Inactive:
                        case ra::data::models::AssetState::Triggered:
                            bHasInactiveSelection = true;
                            break;
                        default:
                            bHasActiveSelection = true;
                            break;
                    }

                    switch (pItem->GetChanges())
                    {
                        case ra::data::models::AssetChanges::Modified:
                        case ra::data::models::AssetChanges::New:
                            bHasModifiedSelection = true;
                            break;
                        case ra::data::models::AssetChanges::Unpublished:
                            bHasUnpublishedSelection = true;
                            break;
                        default:
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
        SetValue(CanCloneProperty, true);
    }
    else if (bHasActiveSelection)
    {
        SetValue(ActivateButtonTextProperty, L"De&activate");
        SetValue(CanCloneProperty, true);
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
            SetValue(ResetButtonTextProperty, L"&Reset");
        else
            SetValue(ResetButtonTextProperty, ResetButtonTextProperty.GetDefaultValue());

        SetValue(CanResetProperty, true);
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

void AssetListViewModel::ActivateSelected()
{
    std::vector<ra::data::models::AssetModelBase*> vSelectedAssets;
    GetSelectedAssets(vSelectedAssets);

    if (GetActivateButtonText().at(0) == 'D') // Deactivate
    {
        for (auto* vmItem : vSelectedAssets)
        {
            if (vmItem)
                vmItem->Deactivate();
        }
    }
    else // Activate
    {
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
    for (const auto* pCondition = pCondSet->conditions; pCondition != nullptr; pCondition = pCondition->next)
    {
        if (pCondition->type == RC_CONDITION_RESET_NEXT_IF)
            return L"ResetNextIf is pre-release functionality";
        if (pCondition->type == RC_CONDITION_TRIGGER)
            return L"Trigger is pre-release functionality";
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
    vmAssetUpload.ShowResults();
}

void AssetListViewModel::ResetSelected()
{
    if (!CanReset())
        return;

    std::vector<const AssetSummaryViewModel*> vSelectedAssets;
    for (gsl::index nIndex = 0; nIndex < gsl::narrow_cast<gsl::index>(m_vFilteredAssets.Count()); ++nIndex)
    {
        const auto* pItem = m_vFilteredAssets.GetItemAt(nIndex);
        if (pItem != nullptr && pItem->IsSelected())
            vSelectedAssets.push_back(pItem);
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

    auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::context::GameContext>();
    pGameContext.Assets().BeginUpdate();

    std::vector<ra::data::models::AssetModelBase*> vAssetsToReset;
    if (vSelectedAssets.empty())
    {
        // reset all - first remove any "new" items
        for (gsl::index nIndex = gsl::narrow_cast<gsl::index>(pGameContext.Assets().Count()) - 1; nIndex >= 0; --nIndex)
        {
            auto* pAsset = pGameContext.Assets().GetItemAt(nIndex);
            if (pAsset != nullptr && pAsset->GetChanges() == ra::data::models::AssetChanges::New)
                pGameContext.Assets().RemoveAt(nIndex);
        }

        // when resetting all, always read the file to pick up new items
        pGameContext.Assets().ReloadAssets(vAssetsToReset);
    }
    else
    {
        // reset selection, remove any "new" items and get the AssetViewModel for the others
        for (auto* pItem : vSelectedAssets)
        {
            const auto nType = pItem->GetType();
            const auto nId = ra::to_unsigned(pItem->GetId());
            for (gsl::index nIndex = gsl::narrow_cast<gsl::index>(pGameContext.Assets().Count()) - 1; nIndex >= 0; --nIndex)
            {
                auto* pAsset = pGameContext.Assets().GetItemAt(nIndex);
                if (pAsset->GetID() == nId && pAsset->GetType() == nType)
                {
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
            pGameContext.Assets().ReloadAssets(vAssetsToReset);
    }

    pGameContext.Assets().EndUpdate();
}

void AssetListViewModel::RevertSelected()
{
    if (!CanRevert())
        return;

    std::vector<ra::data::models::AssetModelBase*> vAssetsToReset;
    GetSelectedAssets(vAssetsToReset);

    bool bHasLocal = false;
    for (const auto* pAsset : vAssetsToReset)
    {
        if (pAsset != nullptr && pAsset->GetCategory() == ra::data::models::AssetCategory::Local)
        {
            bHasLocal = true;
            break;
        }
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

        if (bHasLocal)
            sWarningMessage += L"\n\n";
    }
    else // Delete
    {
        vmMessageBox.SetHeader(L"Permanently delete?");

        Expects(bHasLocal); // message is shared with text appended to Revert message
    }

    if (bHasLocal)
        sWarningMessage += L"Assets not available on the server will be permanently deleted.";
    vmMessageBox.SetMessage(sWarningMessage);

    if (vmMessageBox.ShowModal() == DialogResult::No)
        return;

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
    auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::context::GameContext>();
    const auto& pUserContext = ra::services::ServiceLocator::GetMutable<ra::data::context::UserContext>();

    FilteredAssets().BeginUpdate();

    SetFilterCategory(ra::data::models::AssetCategory::Local);

    auto& vmAchievement = pGameContext.Assets().NewAchievement();
    vmAchievement.SetCategory(ra::data::models::AssetCategory::Local);
    vmAchievement.SetPoints(0);
    vmAchievement.SetAuthor(ra::Widen(pUserContext.GetUsername()));
    vmAchievement.UpdateServerCheckpoint();
    vmAchievement.SetNew();

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

    // assert: only Core assets are tallied, so we shouldn't need to call UpdateTotals

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

    auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::context::GameContext>();

    FilteredAssets().BeginUpdate();

    SetFilterCategory(ra::data::models::AssetCategory::Local);

    // add the cloned items
    std::vector<int> vNewIDs;
    for (const auto* pAsset : vSelectedAssets)
    {
        const auto& pSourceAchievement = dynamic_cast<const ra::data::models::AchievementModel*>(pAsset);
        auto& vmAchievement = pGameContext.Assets().NewAchievement();
        vmAchievement.SetCategory(ra::data::models::AssetCategory::Local);
        vmAchievement.SetName(pSourceAchievement->GetName() + L" (copy)");
        vmAchievement.SetDescription(pSourceAchievement->GetDescription());
        vmAchievement.SetBadge(pSourceAchievement->GetBadge());
        vmAchievement.SetPoints(pSourceAchievement->GetPoints());
        vmAchievement.SetTrigger(pSourceAchievement->GetTrigger());
        vmAchievement.UpdateServerCheckpoint();
        vmAchievement.SetNew();

        vNewIDs.push_back(vmAchievement.GetID());
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

    // assert: only Core assets are tallied, so we shouldn't need to call UpdateTotals

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
