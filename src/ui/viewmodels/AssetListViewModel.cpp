#include "AssetListViewModel.hh"

#include "data\context\GameContext.hh"

#include "services\ILocalStorage.hh"
#include "services\IThreadPool.hh"
#include "services\ServiceLocator.hh"

#include "ui\viewmodels\MessageBoxViewModel.hh"
#include "ui\viewmodels\WindowManager.hh"

#include "Exports.hh"
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
const IntModelProperty AssetListViewModel::FilterCategoryProperty("AssetListViewModel", "FilterCategory", ra::etoi(AssetCategory::Core));
const IntModelProperty AssetListViewModel::EnsureVisibleAssetIndexProperty("AssetListViewModel", "EnsureVisibleAssetIndex", -1);

AssetListViewModel::AssetListViewModel() noexcept
{
    SetWindowTitle(L"Assets List");

    m_vStates.Add(ra::etoi(AssetState::Inactive), L"Inactive");
    m_vStates.Add(ra::etoi(AssetState::Waiting), L"Waiting");
    m_vStates.Add(ra::etoi(AssetState::Active), L"Active");
    m_vStates.Add(ra::etoi(AssetState::Paused), L"Paused");
    m_vStates.Add(ra::etoi(AssetState::Triggered), L"Triggered");

    m_vCategories.Add(ra::etoi(AssetCategory::Core), L"Core");
    m_vCategories.Add(ra::etoi(AssetCategory::Unofficial), L"Unofficial");
    m_vCategories.Add(ra::etoi(AssetCategory::Local), L"Local");

    m_vChanges.Add(ra::etoi(AssetChanges::None), L"");
    m_vChanges.Add(ra::etoi(AssetChanges::Modified), L"Modified");
    m_vChanges.Add(ra::etoi(AssetChanges::Unpublished), L"Unpublished");
    m_vChanges.Add(ra::etoi(AssetChanges::New), L"New");

    m_vAssets.AddNotifyTarget(*this);

    // have to use separate monitor for capturing selection changes in filtered list
    // so EndUpdate for filtered list doesn't trigger another call to ApplyFilter.
    m_pFilteredListMonitor.m_pOwner = this;
    m_vFilteredAssets.AddNotifyTarget(m_pFilteredListMonitor);
}

AssetListViewModel::~AssetListViewModel()
{
    m_bNeedToUpdateButtons.store(false);
}

static bool IsTalliedAchievement(const AchievementViewModel& pAchievement)
{
    return (pAchievement.GetCategory() == AssetCategory::Core);
}

void AssetListViewModel::OnViewModelStringValueChanged(gsl::index nIndex, const StringModelProperty::ChangeArgs& args)
{
    // sync this property through to the summary view model
    if (args.Property == AssetViewModelBase::NameProperty)
    {
        auto* pItem = m_vAssets.GetItemAt(nIndex);
        if (pItem != nullptr)
        {
            const auto nFilteredIndex = GetFilteredAssetIndex(*pItem);
            if (nFilteredIndex != -1)
                m_vFilteredAssets.SetItemValue(nFilteredIndex, AssetSummaryViewModel::LabelProperty, args.tNewValue);
        }
    }
}

void AssetListViewModel::OnViewModelIntValueChanged(gsl::index nIndex, const IntModelProperty::ChangeArgs& args)
{
    if (args.Property == AchievementViewModel::PointsProperty)
    {
        auto* pAchievement = dynamic_cast<const AchievementViewModel*>(m_vAssets.GetItemAt(nIndex));
        if (pAchievement != nullptr && IsTalliedAchievement(*pAchievement))
        {
            auto nPoints = GetTotalPoints();
            nPoints += args.tNewValue - args.tOldValue;
            SetValue(TotalPointsProperty, nPoints);
        }
    }
    else if (args.Property == AssetViewModelBase::CategoryProperty)
    {
        UpdateTotals();

        auto* pItem = m_vAssets.GetItemAt(nIndex);
        if (pItem != nullptr)
            AddOrRemoveFilteredItem(*pItem);
    }

    // sync these properties through to the summary view model
    if (args.Property == AssetViewModelBase::ChangesProperty ||
        args.Property == AchievementViewModel::PointsProperty ||
        args.Property == AssetViewModelBase::StateProperty ||
        args.Property == AssetViewModelBase::CategoryProperty)
    {
        auto* pItem = m_vAssets.GetItemAt(nIndex);
        if (pItem != nullptr)
        {
            const auto nFilteredIndex = GetFilteredAssetIndex(*pItem);
            if (nFilteredIndex != -1)
            {
                m_vFilteredAssets.SetItemValue(nFilteredIndex, args.Property, args.tNewValue);

                if (m_vFilteredAssets.GetItemValue(nFilteredIndex, AssetSummaryViewModel::IsSelectedProperty))
                    UpdateButtons();
            }
        }
    }
    else if (args.Property == AssetViewModelBase::IDProperty)
    {
        auto* pAsset = m_vAssets.GetItemAt(nIndex);
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

void AssetListViewModel::OnViewModelAdded(_UNUSED gsl::index nIndex)
{
    if (!m_vAssets.IsUpdating())
    {
        UpdateTotals();

        auto* pItem = m_vAssets.GetItemAt(nIndex);
        if (pItem != nullptr)
            AddOrRemoveFilteredItem(*pItem);
    }
}

void AssetListViewModel::OnViewModelRemoved(_UNUSED gsl::index nIndex)
{
    if (!m_vAssets.IsUpdating())
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
                if (!FindAsset(pItem->GetType(), pItem->GetId()))
                {
                    m_vFilteredAssets.RemoveAt(nFilteredIndex);
                    UpdateButtons();
                    break;
                }
            }
        }
    }
}

void AssetListViewModel::OnViewModelChanged(_UNUSED gsl::index nIndex)
{
    if (!m_vAssets.IsUpdating())
    {
        UpdateTotals();

        auto* pItem = m_vAssets.GetItemAt(nIndex);
        if (pItem != nullptr)
            AddOrRemoveFilteredItem(*pItem);
    }
}

void AssetListViewModel::OnEndViewModelCollectionUpdate()
{
    UpdateTotals();
    ApplyFilter();
}

void AssetListViewModel::UpdateTotals()
{
    int nAchievementCount = 0;
    int nTotalPoints = 0;

    for (gsl::index nIndex = 0; nIndex < gsl::narrow_cast<gsl::index>(m_vAssets.Count()); ++nIndex)
    {
        auto* pAchievement = dynamic_cast<const AchievementViewModel*>(m_vAssets.GetItemAt(nIndex));
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

void AssetListViewModel::ApplyFilter()
{
    m_vFilteredAssets.BeginUpdate();

    // first pass: remove any filtered items no longer in the source collection
    for (gsl::index nIndex = gsl::narrow_cast<gsl::index>(m_vFilteredAssets.Count()) -1; nIndex >= 0; --nIndex)
    {
        auto* pItem = m_vFilteredAssets.GetItemAt(nIndex);
        if (pItem != nullptr)
        {
            if (!FindAsset(pItem->GetType(), pItem->GetId()))
                m_vFilteredAssets.RemoveAt(nIndex);
        }
    }

    // second pass: update visibility of each item in the source collection
    for (gsl::index nIndex = 0; nIndex < gsl::narrow_cast<gsl::index>(m_vAssets.Count()); ++nIndex)
    {
        auto* pItem = m_vAssets.GetItemAt(nIndex);
        if (pItem != nullptr)
            AddOrRemoveFilteredItem(*pItem);
    }

    m_vFilteredAssets.EndUpdate();

    UpdateButtons();
}

bool AssetListViewModel::MatchesFilter(const AssetViewModelBase& pAsset)
{
    if (pAsset.GetCategory() != GetFilterCategory())
        return false;

    return true;
}

void AssetListViewModel::AddOrRemoveFilteredItem(const AssetViewModelBase& pAsset)
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

            const auto* pAchievement = dynamic_cast<const AchievementViewModel*>(&pAsset);
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

gsl::index AssetListViewModel::GetFilteredAssetIndex(const AssetViewModelBase& pAsset) const
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

AssetViewModelBase* AssetListViewModel::FindAsset(AssetType nType, ra::AchievementID nId)
{
    for (gsl::index nIndex = 0; nIndex < gsl::narrow_cast<gsl::index>(m_vAssets.Count()); ++nIndex)
    {
        auto* pItem = m_vAssets.GetItemAt(nIndex);
        if (pItem != nullptr && pItem->GetID() == nId && pItem->GetType() == nType)
            return pItem;
    }

    return nullptr;
}

const AssetViewModelBase* AssetListViewModel::FindAsset(AssetType nType, ra::AchievementID nId) const
{
    for (gsl::index nIndex = 0; nIndex < gsl::narrow_cast<gsl::index>(m_vAssets.Count()); ++nIndex)
    {
        auto* pItem = m_vAssets.GetItemAt(nIndex);
        if (pItem != nullptr && pItem->GetID() == nId && pItem->GetType() == nType)
            return pItem;
    }

    return nullptr;
}

void AssetListViewModel::OpenEditor(const AssetSummaryViewModel* pAsset)
{
    AssetViewModelBase* vmAsset = nullptr;

    if (pAsset)
        vmAsset = FindAsset(pAsset->GetType(), pAsset->GetId());

    auto& pWindowManager = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::WindowManager>();
    pWindowManager.AssetEditor.LoadAsset(vmAsset);

    if (!pWindowManager.AssetEditor.IsVisible())
        pWindowManager.AssetEditor.Show();
}

bool AssetListViewModel::HasSelection(AssetType nAssetType) const
{
    for (gsl::index nIndex = 0; nIndex < gsl::narrow_cast<gsl::index>(m_vFilteredAssets.Count()); ++nIndex)
    {
        const auto* pItem = m_vFilteredAssets.GetItemAt(nIndex);
        if (pItem != nullptr && pItem->IsSelected())
        {
            if (nAssetType == AssetType::None || nAssetType == pItem->GetType())
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
                    case AssetChanges::Modified:
                    case AssetChanges::New:
                        bHasModified = true;
                        break;
                    case AssetChanges::Unpublished:
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
                        case AssetCategory::Core:
                            bHasCoreSelection = true;
                            break;
                        case AssetCategory::Unofficial:
                            bHasUnofficialSelection = true;
                            break;
                        case AssetCategory::Local:
                            bHasLocalSelection = true;
                            break;
                        default:
                            break;
                    }

                    switch (pItem->GetState())
                    {
                        case AssetState::Inactive:
                        case AssetState::Triggered:
                            bHasInactiveSelection = true;
                            break;
                        default:
                            bHasActiveSelection = true;
                            break;
                    }

                    switch (pItem->GetChanges())
                    {
                        case AssetChanges::Modified:
                        case AssetChanges::New:
                            bHasModifiedSelection = true;
                            break;
                        case AssetChanges::Unpublished:
                            bHasUnpublishedSelection = true;
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
        SetValue(SaveButtonTextProperty, L"&Publish All");
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

void AssetListViewModel::GetSelectedAssets(std::vector<AssetViewModelBase*>& vSelectedAssets)
{
    for (size_t i = 0; i < m_vFilteredAssets.Count(); ++i)
    {
        const auto* pItem = m_vFilteredAssets.GetItemAt(i);
        if (pItem != nullptr && pItem->IsSelected())
        {
            auto* vmItem = FindAsset(pItem->GetType(), pItem->GetId());
            if (vmItem != nullptr)
                vSelectedAssets.push_back(vmItem);
        }
    }

    // no selected assets found, return all assets
    if (vSelectedAssets.size() == 0)
    {
        for (size_t i = 0; i < m_vFilteredAssets.Count(); ++i)
        {
            const auto* pItem = m_vFilteredAssets.GetItemAt(i);
            if (pItem != nullptr)
            {
                auto* vmItem = FindAsset(pItem->GetType(), pItem->GetId());
                if (vmItem != nullptr)
                    vSelectedAssets.push_back(vmItem);
            }
        }
    }
}

void AssetListViewModel::ActivateSelected()
{
    std::vector<AssetViewModelBase*> vSelectedAssets;
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
    const bool bHasSelection = HasSelection(AssetType::Achievement);
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();

    auto& pLocalStorage = ra::services::ServiceLocator::GetMutable<ra::services::ILocalStorage>();
    auto pData = pLocalStorage.WriteText(ra::services::StorageItemType::UserAchievements, std::to_wstring(pGameContext.GameId()));
    if (pData == nullptr)
    {
        RA_LOG_ERR("Failed to create user assets file");
        return;
    }

    pData->WriteLine(_RA_IntegrationVersion()); // version used to create the file
    pData->WriteLine(pGameContext.GameTitle());

    for (gsl::index nIndex = 0; nIndex < gsl::narrow_cast<gsl::index>(m_vAssets.Count()); ++nIndex)
    {
        auto* pItem = m_vAssets.GetItemAt(nIndex);
        if (pItem == nullptr)
            continue;

        switch (pItem->GetChanges())
        {
            case AssetChanges::None:
                // non-modified committed items don't need to be serialized
                // ASSERT: unmodified local items should report as Unpublished
                continue;

            case AssetChanges::Unpublished:
                // always write unpublished changes
                break;

            default:
                // if the item is modified, check to see if it's selected
                bool bSelected = !bHasSelection;
                if (bHasSelection)
                {
                    const auto nFilteredIndex = GetFilteredAssetIndex(*pItem);
                    if (nFilteredIndex >= 0 && m_vFilteredAssets.GetItemAt(nFilteredIndex)->IsSelected())
                        bSelected = true;
                }

                // if it's selected, commit the changes before flushing
                if (bSelected)
                {
                    pItem->UpdateLocalCheckpoint();

                    // reverted back to committed state, no need to serialize
                    if (pItem->GetChanges() == AssetChanges::None)
                        continue;
                }
                else if (pItem->GetCategory() != AssetCategory::Local)
                {
                    // if there's no unpublished changes and the item is not selected, ignore it
                    if (!pItem->HasUnpublishedChanges())
                        continue;
                }
                else if (pItem->GetChanges() == AssetChanges::New)
                {
                    // unselected new item has no local state to maintain
                    continue;
                }
                break;
        }

        // serialize the item
        pData->Write(std::to_string(pItem->GetID()));
        pItem->Serialize(*pData);
        pData->WriteLine();
    }

    RA_LOG_INFO("Wrote user assets file");
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

    m_vAssets.BeginUpdate();

    std::vector<AssetViewModelBase*> vAssetsToReset;
    if (vSelectedAssets.empty())
    {
        // reset all - first remove any "new" items
        for (gsl::index nIndex = gsl::narrow_cast<gsl::index>(m_vAssets.Count()) - 1; nIndex >= 0; --nIndex)
        {
            auto* pAsset = m_vAssets.GetItemAt(nIndex);
            if (pAsset != nullptr && pAsset->GetChanges() == AssetChanges::New)
                m_vAssets.RemoveAt(nIndex);
        }

        // when resetting all, always read the file to pick up new items
        MergeLocalAssets(vAssetsToReset, true);
    }
    else
    {
        // reset selection, remove any "new" items and get the AssetViewModel for the others
        for (auto* pItem : vSelectedAssets)
        {
            const auto nType = pItem->GetType();
            const auto nId = ra::to_unsigned(pItem->GetId());
            for (gsl::index nIndex = gsl::narrow_cast<gsl::index>(m_vAssets.Count()) - 1; nIndex >= 0; --nIndex)
            {
                auto* pAsset = m_vAssets.GetItemAt(nIndex);
                if (pAsset->GetID() == nId && pAsset->GetType() == nType)
                {
                    if (pAsset->GetChanges() == AssetChanges::New)
                        m_vAssets.RemoveAt(nIndex);
                    else
                        vAssetsToReset.push_back(pAsset);
                    break;
                }
            }
        }

        // if there were any non-new items, reload them from disk
        if (!vAssetsToReset.empty())
            MergeLocalAssets(vAssetsToReset, false);
    }

    m_vAssets.EndUpdate();
}

// NOTE: destroys vAssetsToMerge as it processes the file
void AssetListViewModel::MergeLocalAssets(std::vector<AssetViewModelBase*>& vAssetsToMerge, bool bFullMerge)
{
    auto& pLocalStorage = ra::services::ServiceLocator::GetMutable<ra::services::ILocalStorage>();
    auto pData = pLocalStorage.ReadText(ra::services::StorageItemType::UserAchievements, std::to_wstring(GetGameId()));
    if (pData == nullptr)
        return;

    std::string sLine;
    pData->GetLine(sLine); // version used to create the file
    pData->GetLine(sLine); // game title

    m_vAssets.BeginUpdate();

    std::vector<AssetViewModelBase*> vUnnumberedAssets;
    while (pData->GetLine(sLine))
    {
        AssetType nType = AssetType::None;
        unsigned nId = 0;

        ra::Tokenizer pTokenizer(sLine);
        switch (pTokenizer.PeekChar())
        {
            case '0': case '1': case '2': case '3': case '4':
            case '5': case '6': case '7': case '8': case '9':
                // achievements start with a number (no prefix)
                nType = AssetType::Achievement;
                nId = pTokenizer.ReadNumber();
                break;
        }

        if (nType == AssetType::None)
            continue;
        if (!pTokenizer.Consume(':'))
            continue;

        // find the asset to merge
        AssetViewModelBase* pAsset = nullptr;
        if (nId != 0)
        {
            if (nId >= m_nNextLocalId)
                m_nNextLocalId = nId + 1;

            for (auto pIter = vAssetsToMerge.begin(); pIter != vAssetsToMerge.end(); ++pIter)
            {
                auto* pAssetToReset = *pIter;
                if (pAssetToReset->GetID() == nId && pAssetToReset->GetType() == nType)
                {
                    pAsset = pAssetToReset;
                    vAssetsToMerge.erase(pIter);
                    pAsset->RestoreServerCheckpoint();
                    break;
                }
            }

            if (!pAsset && bFullMerge && nId < FirstLocalId)
                pAsset = FindAsset(nType, nId);
        }

        if (pAsset)
        {
            // merge the data from the file
            pAsset->Deserialize(pTokenizer);
            pAsset->UpdateLocalCheckpoint();
        }
        else if (bFullMerge)
        {
            // non-existant item, only process it if nothing was selected
            switch (nType)
            {
                case AssetType::Achievement:
                {
                    auto vmAchievement = std::make_unique<ra::ui::viewmodels::AchievementViewModel>();
                    vmAchievement->SetID(nId);
                    vmAchievement->SetCategory(ra::ui::viewmodels::AssetCategory::Local);
                    vmAchievement->CreateServerCheckpoint();

                    pAsset = &m_vAssets.Append(std::move(vmAchievement));
                    break;
                }
            }

            if (pAsset)
            {
                pAsset->Deserialize(pTokenizer);
                pAsset->CreateLocalCheckpoint();

                if (nId == 0)
                    vUnnumberedAssets.push_back(pAsset);
            }
        }
    }

    // assign IDs for any assets where one was not available
    for (auto* pAsset : vUnnumberedAssets)
        pAsset->SetID(m_nNextLocalId++);

    // any items still in the source list no longer exist in the file. if the item is on the server,
    // just reset to the server state. otherwise, delete the item.
    for (auto* pAsset : vAssetsToMerge)
    {
        if (pAsset->GetCategory() == AssetCategory::Local)
        {
            for (gsl::index nIndex = gsl::narrow_cast<gsl::index>(m_vAssets.Count()) - 1; nIndex >= 0; --nIndex)
            {
                if (m_vAssets.GetItemAt(nIndex) == pAsset)
                {
                    m_vAssets.RemoveAt(nIndex);
                    break;
                }
            }
        }
        else
        {
            pAsset->RestoreServerCheckpoint();
        }
    }

    m_vAssets.EndUpdate();
}

void AssetListViewModel::MergeLocalAssets()
{
    std::vector<AssetViewModelBase*> vAssetsToMerge;
    MergeLocalAssets(vAssetsToMerge, true);
}

void AssetListViewModel::RevertSelected()
{
    if (!CanRevert())
        return;
}

void AssetListViewModel::CreateNew()
{
    auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::context::GameContext>();
    const auto& pAchievement = pGameContext.NewAchievement(Achievement::Category::Local);

    auto vmAchievement = std::make_unique<ra::ui::viewmodels::AchievementViewModel>();
    vmAchievement->SetID(pAchievement.ID());
    vmAchievement->SetCategory(AssetCategory::Local);
    vmAchievement->SetPoints(0);
    vmAchievement->CreateServerCheckpoint();
    vmAchievement->CreateLocalCheckpoint();
    vmAchievement->SetNew();

    const auto nId = ra::to_signed(vmAchievement->GetID());

    FilteredAssets().BeginUpdate();

    SetFilterCategory(AssetCategory::Local);

    Assets().Append(std::move(vmAchievement));

    // select the new viewmodel, and deselect everything else
    gsl::index nIndex = -1;
    for (gsl::index i = 0; i < ra::to_signed(m_vFilteredAssets.Count()); ++i)
    {
        auto* pItem = m_vFilteredAssets.GetItemAt(i);
        if (pItem != nullptr)
        {
            if (pItem->GetId() == nId && pItem->GetType() == AssetType::Achievement)
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

    std::vector<AssetViewModelBase*> vSelectedAssets;
    GetSelectedAssets(vSelectedAssets);
    if (vSelectedAssets.empty())
        return;

    auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::context::GameContext>();

    FilteredAssets().BeginUpdate();

    SetFilterCategory(AssetCategory::Local);

    // add the cloned items
    std::vector<int> vNewIDs;
    for (const auto* pAsset : vSelectedAssets)
    {
        const auto& pSourceAchievement = dynamic_cast<const AchievementViewModel*>(pAsset);

        const auto& pAchievement = pGameContext.NewAchievement(Achievement::Category::Local);
        vNewIDs.push_back(gsl::narrow_cast<int>(pAchievement.ID()));

        auto vmAchievement = std::make_unique<ra::ui::viewmodels::AchievementViewModel>();
        vmAchievement->SetID(pAchievement.ID());
        vmAchievement->SetCategory(AssetCategory::Local);
        vmAchievement->SetName(pSourceAchievement->GetName() + L" (copy)");
        vmAchievement->SetDescription(pSourceAchievement->GetDescription());
        vmAchievement->SetBadge(pSourceAchievement->GetBadge());
        vmAchievement->SetPoints(pSourceAchievement->GetPoints());
        vmAchievement->SetTrigger(pSourceAchievement->GetTrigger());
        vmAchievement->CreateServerCheckpoint();
        vmAchievement->CreateLocalCheckpoint();
        vmAchievement->SetNew();

        Assets().Append(std::move(vmAchievement));
    }

    // select the new items and deselect everything else
    gsl::index nIndex = -1;
    for (gsl::index i = 0; i < ra::to_signed(m_vFilteredAssets.Count()); ++i)
    {
        auto* pItem = m_vFilteredAssets.GetItemAt(i);
        if (pItem != nullptr)
        {
            if (std::find(vNewIDs.begin(), vNewIDs.end(), pItem->GetId()) != vNewIDs.end() &&
                pItem->GetType() == AssetType::Achievement)
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

void AssetListViewModel::DoFrame()
{
    for (gsl::index nIndex = 0; nIndex < gsl::narrow_cast<gsl::index>(m_vAssets.Count()); ++nIndex)
    {
        auto* pItem = m_vAssets.GetItemAt(nIndex);
        if (pItem != nullptr)
            pItem->DoFrame();
    }
}

} // namespace viewmodels
} // namespace ui
} // namespace ra
