#include "ViewModelCollection.hh"

namespace ra {
namespace ui {

ViewModelBase& ViewModelCollectionBase::AddItem(std::unique_ptr<ViewModelBase> vmViewModel)
{
    assert(!IsFrozen());

    // add the item to the end of the array with a temporary index so we know to notify in UpdateIndices
    // note that we don't StartWatching here, no sense until the NotifyTarget has been notified that the item exists
    auto& pItem = m_vItems.emplace_back(*this, -1, std::move(vmViewModel));
    ++m_nSize;

    if (m_nUpdateCount == 0)
        UpdateIndices();

    return pItem.ViewModel();
}

void ViewModelCollectionBase::RemoveAt(gsl::index nIndex)
{
    if (nIndex >= 0 && nIndex < ra::to_signed(m_nSize))
    {
        assert(!IsFrozen());

        // move the item to the end of the array - we'll clean it up in UpdateIndices
        if (nIndex != ra::to_signed(m_nSize) - 1)
            MoveItemInternal(nIndex, m_nSize - 1);

        // store the old index so we can notify properly
        auto& pItem = m_vItems.at(m_nSize - 1);

        // stop watching the item immediately
        if (IsWatching())
            pItem.StopWatching();

        // delete the item
        pItem.AttachViewModel(nullptr);

        // update the size
        --m_nSize;

        if (m_nUpdateCount == 0)
            UpdateIndices();
    }
}

void ViewModelCollectionBase::MoveItem(gsl::index nIndex, gsl::index nNewIndex)
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

void ViewModelCollectionBase::MoveItemInternal(gsl::index nIndex, gsl::index nNewIndex)
{
    auto pItemToMove = m_vItems.at(nIndex).DetachViewModel();
    const auto nOldIndex = m_vItems.at(nIndex).Index();

    if (nNewIndex < nIndex)
    {
        for (auto nSwapIndex = nIndex; nSwapIndex > nNewIndex; --nSwapIndex)
        {
            auto pItemToSwap = m_vItems.at(nSwapIndex - 1).DetachViewModel();
            m_vItems.at(nSwapIndex).AttachViewModel(pItemToSwap);
            m_vItems.at(nSwapIndex).SetIndex(m_vItems.at(nSwapIndex - 1).Index());
        }
    }
    else
    {
        for (auto nSwapIndex = nIndex; nSwapIndex < nNewIndex; ++nSwapIndex)
        {
            auto pItemToSwap = m_vItems.at(nSwapIndex + 1).DetachViewModel();
            m_vItems.at(nSwapIndex).AttachViewModel(pItemToSwap);
            m_vItems.at(nSwapIndex).SetIndex(m_vItems.at(nSwapIndex + 1).Index());
        }
    }

    m_vItems.at(nNewIndex).AttachViewModel(pItemToMove);
    m_vItems.at(nNewIndex).SetIndex(nOldIndex);
}

size_t ViewModelCollectionBase::ShiftItemsUp(const BoolModelProperty& pProperty)
{
    size_t nMatches = 0;

    BeginUpdate();

    gsl::index nInsertAt = -1;
    for (gsl::index nIndex = 0; nIndex < ra::to_signed(m_nSize); ++nIndex)
    {
        const auto& pItem = m_vItems.at(nIndex).ViewModel();
        if (!pItem.GetValue(pProperty))
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

size_t ViewModelCollectionBase::ShiftItemsDown(const BoolModelProperty& pProperty)
{
    size_t nMatches = 0;

    BeginUpdate();

    gsl::index nInsertAt = -1;
    for (gsl::index nIndex = ra::to_signed(m_nSize) - 1; nIndex >= 0; --nIndex)
    {
        const auto& pItem = m_vItems.at(nIndex).ViewModel();
        if (!pItem.GetValue(pProperty))
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

void ViewModelCollectionBase::Reverse()
{
    BeginUpdate();

    gsl::index nIndexFront = 0;
    gsl::index nIndexBack = m_nSize - 1;

    while (nIndexFront < nIndexBack)
    {
        auto pItemFront = m_vItems.at(nIndexFront).DetachViewModel();
        auto pItemBack = m_vItems.at(nIndexBack).DetachViewModel();
        m_vItems.at(nIndexFront).AttachViewModel(pItemBack);
        m_vItems.at(nIndexBack).AttachViewModel(pItemFront);

        const auto nOldIndexFront = m_vItems.at(nIndexFront).Index();
        const auto nOldIndexBack = m_vItems.at(nIndexBack).Index();
        m_vItems.at(nIndexFront).SetIndex(nOldIndexBack);
        m_vItems.at(nIndexBack).SetIndex(nOldIndexFront);

        ++nIndexFront;
        --nIndexBack;
    }

    EndUpdate();
}

void ViewModelCollectionBase::StartWatching() noexcept
{
    for (auto& pItem : m_vItems)
        pItem.StartWatching();
}

void ViewModelCollectionBase::StopWatching() noexcept
{
    for (auto& pItem : m_vItems)
        pItem.StopWatching();
}

void ViewModelCollectionBase::OnViewModelBoolValueChanged(gsl::index nIndex,
                                                          const BoolModelProperty::ChangeArgs& args)
{
    // create a copy of the list of pointers in case it's modified by one of the callbacks
    NotifyTargetSet vNotifyTargets(m_vNotifyTargets);
    for (NotifyTarget* target : vNotifyTargets)
    {
        Expects(target != nullptr);
        target->OnViewModelBoolValueChanged(nIndex, args);
    }
}

void ViewModelCollectionBase::OnViewModelStringValueChanged(gsl::index nIndex,
                                                            const StringModelProperty::ChangeArgs& args)
{
    // create a copy of the list of pointers in case it's modified by one of the callbacks
    NotifyTargetSet vNotifyTargets(m_vNotifyTargets);
    for (NotifyTarget* target : vNotifyTargets)
    {
        Expects(target != nullptr);
        target->OnViewModelStringValueChanged(nIndex, args);
    }
}

void ViewModelCollectionBase::OnViewModelIntValueChanged(gsl::index nIndex,
                                                         const IntModelProperty::ChangeArgs& args)
{
    // create a copy of the list of pointers in case it's modified by one of the callbacks
    NotifyTargetSet vNotifyTargets(m_vNotifyTargets);
    for (NotifyTarget* target : vNotifyTargets)
    {
        Expects(target != nullptr);
        target->OnViewModelIntValueChanged(nIndex, args);
    }
}

void ViewModelCollectionBase::BeginUpdate()
{
    if (m_nUpdateCount++ == 0)
    {
        // create a copy of the list of pointers in case it's modified by one of the callbacks
        NotifyTargetSet vNotifyTargets(m_vNotifyTargets);
        for (NotifyTarget* target : vNotifyTargets)
        {
            Expects(target != nullptr);
            target->OnBeginViewModelCollectionUpdate();
        }
    }
}

void ViewModelCollectionBase::EndUpdate()
{
    if (m_nUpdateCount == 1)
    {
        UpdateIndices();

        // don't actually decrement the update count until after the indices have been fixed
        --m_nUpdateCount;

        // create a copy of the list of pointers in case it's modified by one of the callbacks
        NotifyTargetSet vNotifyTargets(m_vNotifyTargets);
        for (NotifyTarget* target : vNotifyTargets)
        {
            Expects(target != nullptr);
            target->OnEndViewModelCollectionUpdate();
        }
    }
    else if (m_nUpdateCount > 0)
    {
        --m_nUpdateCount;
    }
}

void ViewModelCollectionBase::UpdateIndices()
{
    // first pass, deal with removed items
    if (m_vItems.size() > m_nSize)
    {
        const auto nDeletedIndices = m_vItems.size() - m_nSize;
        std::vector<gsl::index> vDeletedIndices;
        vDeletedIndices.reserve(nDeletedIndices);
        for (gsl::index nIndex = m_vItems.size() - 1; nIndex >= ra::to_signed(m_vItems.size() - nDeletedIndices); --nIndex)
        {
            auto& pItem = m_vItems.at(nIndex);
            vDeletedIndices.push_back(pItem.Index());
        }

        m_vItems.erase(m_vItems.begin() + m_nSize, m_vItems.end());

        // remove the later indices first
        std::sort(vDeletedIndices.rbegin(), vDeletedIndices.rend());

        for (auto& pItem : m_vItems)
        {
            gsl::index nIndex = pItem.Index();
            for (auto nDeletedIndex : vDeletedIndices)
            {
                if (nDeletedIndex < nIndex)
                    --nIndex;
            }

            pItem.SetIndex(nIndex);
        }

        if (IsWatching())
        {
            // create a copy of the list of pointers in case it's modified by one of the callbacks
            NotifyTargetSet vNotifyTargets(m_vNotifyTargets);
            for (auto nDeletedIndex : vDeletedIndices)
            {
                for (NotifyTarget* target : vNotifyTargets)
                {
                    Expects(target != nullptr);
                    target->OnViewModelRemoved(nDeletedIndex);
                }

                if (vNotifyTargets.size() != m_vNotifyTargets.size())
                    vNotifyTargets = m_vNotifyTargets;
            }
        }
    }

    // second pass, deal with new items
    gsl::index nIndex = 0;
    for (auto& pItem : m_vItems)
    {
        if (pItem.Index() == -1)
        {
            for (auto& pIter : m_vItems)
            {
                if (pIter.Index() >= nIndex)
                    pIter.SetIndex(pIter.Index() + 1);
            }

            pItem.SetIndex(nIndex);

            if (IsWatching())
            {
                pItem.StartWatching();

                // create a copy of the list of pointers in case it's modified by one of the callbacks
                NotifyTargetSet vNotifyTargets(m_vNotifyTargets);
                for (NotifyTarget* target : vNotifyTargets)
                {
                    Expects(target != nullptr);
                    target->OnViewModelAdded(nIndex);
                }
            }
        }

        ++nIndex;
    }

    // third pass, deal with moved items
    nIndex = 0;
    for (auto& pItem : m_vItems)
    {
        if (pItem.Index() != nIndex)
        {
            pItem.SetIndex(nIndex);

            if (IsWatching())
            {
                // create a copy of the list of pointers in case it's modified by one of the callbacks
                NotifyTargetSet vNotifyTargets(m_vNotifyTargets);
                for (NotifyTarget* target : vNotifyTargets)
                {
                    Expects(target != nullptr);
                    target->OnViewModelChanged(nIndex);
                }
            }
        }

        ++nIndex;
    }
}

} // namespace ui
} // namespace ra
