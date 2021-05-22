#include "ModelCollectionBase.hh"

namespace ra {
namespace data {

ModelBase& ModelCollectionBase::AddItem(std::unique_ptr<ModelBase> vmViewModel)
{
    assert(!IsFrozen());

    // insert the new item at the end of the array (but before any deleted items)
    // add the item with a temporary index so we know to notify in UpdateIndices.
    // note that we don't call StartWatching here - there's no sense doing that
    // until the NotifyTarget has been notified that the item exists.
    vmViewModel->m_nCollectionIndex = -1;
    auto& pItem = *m_vItems.emplace(m_vItems.begin() + m_nSize, std::move(vmViewModel));
    ++m_nSize;

    if (m_nUpdateCount == 0)
        UpdateIndices();

    return *pItem;
}

void ModelCollectionBase::RemoveAt(gsl::index nIndex)
{
    if (nIndex >= 0 && nIndex < ra::to_signed(m_nSize))
    {
        assert(!IsFrozen());

        // move the item to the end of the array - we'll clean it up in UpdateIndices
        if (nIndex != ra::to_signed(m_nSize) - 1)
            MoveItemInternal(nIndex, m_nSize - 1);

        // store the old index so we can notify properly
        auto& pItem = *m_vItems.at(m_nSize - 1);

        // stop watching the item immediately
        if (IsWatching())
            StopWatching(pItem);

        // update the size
        --m_nSize;

        if (m_nUpdateCount == 0)
            UpdateIndices();
    }
}

void ModelCollectionBase::Clear()
{
    assert(!IsFrozen());

    if (m_nSize == 0)
        return;

    BeginUpdate();

    StopWatching();
    m_nSize = 0;

    EndUpdate();
}

void ModelCollectionBase::MoveItem(gsl::index nIndex, gsl::index nNewIndex)
{
    // nothing to do
    if (nIndex == nNewIndex)
        return;

    // validate source index
    if (nIndex < 0 || nIndex >= ra::to_signed(m_nSize))
        return;

    // validate target index
    if (nNewIndex < 0 || nNewIndex >= ra::to_signed(m_nSize))
        return;

    assert(!IsFrozen());

    MoveItemInternal(nIndex, nNewIndex);

    if (m_nUpdateCount == 0)
        UpdateIndices();
}

void ModelCollectionBase::MoveItemInternal(gsl::index nIndex, gsl::index nNewIndex)
{
    if (nIndex == nNewIndex)
        return;

    std::unique_ptr<ModelBase> pOldItem = std::move(m_vItems.at(nIndex));

    if (nNewIndex < nIndex)
    {
        for (auto nSwapIndex = nIndex; nSwapIndex > nNewIndex; --nSwapIndex)
            m_vItems.at(nSwapIndex) = std::move(m_vItems.at(nSwapIndex - 1));
    }
    else
    {
        for (auto nSwapIndex = nIndex; nSwapIndex < nNewIndex; ++nSwapIndex)
            m_vItems.at(nSwapIndex) = std::move(m_vItems.at(nSwapIndex + 1));
    }

    m_vItems.at(nNewIndex) = std::move(pOldItem);
}

size_t ModelCollectionBase::ShiftItemsUp(const BoolModelProperty& pProperty)
{
    size_t nMatches = 0;

    BeginUpdate();

    gsl::index nInsertAt = -1;
    for (gsl::index nIndex = 0; nIndex < ra::to_signed(m_nSize); ++nIndex)
    {
        if (!GetItemValue(nIndex, pProperty))
            continue;

        ++nMatches;

        if (nInsertAt == -1)
        {
            if (nIndex == 0)
            {
                nInsertAt = 1;
                continue;
            }

            nInsertAt = nIndex - 1;
        }

        MoveItemInternal(nIndex, nInsertAt++);
    }

    EndUpdate();

    return nMatches;
}

size_t ModelCollectionBase::ShiftItemsDown(const BoolModelProperty& pProperty)
{
    size_t nMatches = 0;

    BeginUpdate();

    gsl::index nInsertAt = -1;
    for (gsl::index nIndex = ra::to_signed(m_nSize) - 1; nIndex >= 0; --nIndex)
    {
        if (!GetItemValue(nIndex, pProperty))
            continue;

        ++nMatches;

        if (nInsertAt == -1)
        {
            if (nIndex == ra::to_signed(m_nSize) - 1)
            {
                nInsertAt = ra::to_signed(m_nSize) - 2;
                continue;
            }

            nInsertAt = nIndex + 1;
        }

        MoveItemInternal(nIndex, nInsertAt--);
    }

    EndUpdate();

    return nMatches;
}

void ModelCollectionBase::Reverse()
{
    BeginUpdate();

    gsl::index nIndexFront = 0;
    gsl::index nIndexBack = m_nSize - 1;

    while (nIndexFront < nIndexBack)
    {
        m_vItems.at(nIndexFront).swap(m_vItems.at(nIndexBack));

        ++nIndexFront;
        --nIndexBack;
    }

    EndUpdate();
}

void ModelCollectionBase::StartWatching() noexcept
{
    gsl::index nIndex = 0;
    for (auto& pItem : m_vItems)
        StartWatching(*pItem, nIndex++);
}

void ModelCollectionBase::StartWatching(ModelBase& pModel, gsl::index nIndex) noexcept
{
    pModel.m_nCollectionIndex = nIndex;
    pModel.m_pCollection = this;
}

void ModelCollectionBase::StopWatching() noexcept
{
    for (auto& pItem : m_vItems)
        StopWatching(*pItem);
}

void ModelCollectionBase::StopWatching(ModelBase& pModel) noexcept
{
    pModel.m_pCollection = nullptr;
}

void ModelCollectionBase::BeginUpdate()
{
    if (m_nUpdateCount++ == 0)
        OnBeginUpdate();
}

void ModelCollectionBase::EndUpdate()
{
    if (m_nUpdateCount == 1)
    {
        UpdateIndices();

        // don't actually decrement the update count until after the indices have been fixed
        --m_nUpdateCount;

        OnEndUpdate();
    }
    else if (m_nUpdateCount > 0)
    {
        --m_nUpdateCount;
    }
}

void ModelCollectionBase::UpdateIndices()
{
    const bool bWatching = IsWatching();

    // first pass, deal with removed items
    if (m_vItems.size() > m_nSize)
    {
        // identify the deleted items
        std::vector<gsl::index> vDeletedIndices;
        const auto nDeletedIndices = m_vItems.size() - m_nSize;
        vDeletedIndices.reserve(nDeletedIndices);

        for (gsl::index nIndex = m_vItems.size() - 1; nIndex >= gsl::narrow_cast<gsl::index>(m_nSize); --nIndex)
        {
            auto& pModel = *m_vItems.at(nIndex);
            OnBeforeItemRemoved(pModel);
            vDeletedIndices.push_back(pModel.m_nCollectionIndex);
        }

        // use a reversed list so later indices are removed first when the callbacks are called
        std::sort(vDeletedIndices.rbegin(), vDeletedIndices.rend());

        // remove the deleted items from the collection
        m_vItems.erase(m_vItems.begin() + m_nSize, m_vItems.end());

        // update the indices of any items after the deleted items
        for (auto& pItem : m_vItems)
        {
            gsl::index nIndex = pItem->m_nCollectionIndex;
            if (nIndex > vDeletedIndices.back())
            {
                for (auto nDeletedIndex : vDeletedIndices)
                {
                    if (nDeletedIndex < nIndex)
                        --nIndex;
                }

                pItem->m_nCollectionIndex = nIndex;
            }
        }

        if (!vDeletedIndices.empty())
            OnItemsRemoved(vDeletedIndices);
    }

    // second pass, deal with new items
    std::vector<gsl::index> vNewIndices;
    gsl::index nIndex = 0;
    for (; nIndex < gsl::narrow_cast<gsl::index>(m_vItems.size()); ++nIndex)
    {
        auto& pItem = m_vItems.at(nIndex);
        if (pItem->m_nCollectionIndex == -1)
        {
            // update the indices of any items after the new item
            for (gsl::index nIndex2 = nIndex + 1; nIndex2 < gsl::narrow_cast<gsl::index>(m_vItems.size()); ++nIndex2)
            {
                auto& pItem2 = m_vItems.at(nIndex2);
                if (pItem2->m_nCollectionIndex >= nIndex)
                    pItem2->m_nCollectionIndex = pItem2->m_nCollectionIndex + 1;
            }

            pItem->m_nCollectionIndex = nIndex;

            if (bWatching)
                StartWatching(*pItem, nIndex);

            vNewIndices.push_back(nIndex);
        }
    }

    if (!vNewIndices.empty())
        OnItemsAdded(vNewIndices);

    // third pass, deal with moved items
    nIndex = 0;
    std::vector<gsl::index> vChangedIndices;
    for (auto& pItem : m_vItems)
    {
        if (pItem->m_nCollectionIndex != nIndex)
        {
            pItem->m_nCollectionIndex = nIndex;
            vChangedIndices.push_back(nIndex);
        }

        ++nIndex;
    }

    if (!vChangedIndices.empty())
        OnItemsChanged(vChangedIndices);
}

void ModelCollectionBase::NotifyModelValueChanged(gsl::index nIndex, const BoolModelProperty::ChangeArgs& args)
{
    // ignore events for items added while updates are suspended
    if (nIndex < 0)
        return;

    const auto nCollectionIndex = m_vItems.at(gsl::narrow_cast<size_t>(nIndex))->m_nCollectionIndex;
    if (IsUpdating())
    {
        // if updates are suspended, only raise events for items that haven't moved. OnItemsChanged events
        // will be raised for moved items when the update suspension is resumed.
        if (nIndex == nCollectionIndex)
            OnModelValueChanged(nIndex, args);
    }
    else
    {
        // raising property changed events for the wrong index causes synchronization errors for the GridBinding.
        // that should only happen when updates are suspended, so generate an error if we see it here.
        Expects(nIndex == nCollectionIndex);
        OnModelValueChanged(nIndex, args);
    }
}

void ModelCollectionBase::NotifyModelValueChanged(gsl::index nIndex, const StringModelProperty::ChangeArgs& args)
{
    // ignore events for items added while updates are suspended
    if (nIndex < 0)
        return;

    const auto nCollectionIndex = m_vItems.at(gsl::narrow_cast<size_t>(nIndex))->m_nCollectionIndex;
    if (IsUpdating())
    {
        // if updates are suspended, only raise events for items that haven't moved. OnItemsChanged events
        // will be raised for moved items when the update suspension is resumed.
        if (nIndex == nCollectionIndex)
            OnModelValueChanged(nIndex, args);
    }
    else
    {
        // raising property changed events for the wrong index causes synchronization errors for the GridBinding.
        // that should only happen when updates are suspended, so generate an error if we see it here.
        Expects(nIndex == nCollectionIndex);
        OnModelValueChanged(nIndex, args);
    }
}

void ModelCollectionBase::NotifyModelValueChanged(gsl::index nIndex, const IntModelProperty::ChangeArgs& args)
{
    // ignore events for items added while updates are suspended
    if (nIndex < 0)
        return;

    const auto nCollectionIndex = m_vItems.at(gsl::narrow_cast<size_t>(nIndex))->m_nCollectionIndex;
    if (IsUpdating())
    {
        // if updates are suspended, only raise events for items that haven't moved. OnItemsChanged events
        // will be raised for moved items when the update suspension is resumed.
        if (nIndex == nCollectionIndex)
            OnModelValueChanged(nIndex, args);
    }
    else
    {
        // raising property changed events for the wrong index causes synchronization errors for the GridBinding.
        // that should only happen when updates are suspended, so generate an error if we see it here.
        Expects(nIndex == nCollectionIndex);
        OnModelValueChanged(nIndex, args);
    }
}

} // namespace data
} // namespace ra
