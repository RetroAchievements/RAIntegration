#include "DataModelBase.hh"

namespace ra {
namespace data {

const BoolModelProperty DataModelBase::IsModifiedProperty("DataModelBase", "IsModified", false);

void DataModelBase::OnValueChanged(const BoolModelProperty::ChangeArgs& args)
{
    if (m_pTransaction != nullptr && IsTransactional(args.Property))
    {
        m_pTransaction->ValueChanged(args);
        SetValue(IsModifiedProperty, m_pTransaction->IsModified());
    }

    if (!m_vNotifyTargets.empty())
    {
        // create a copy of the list of pointers in case it's modified by one of the callbacks
        NotifyTargetSet vNotifyTargets(m_vNotifyTargets);
        for (NotifyTarget* target : vNotifyTargets)
        {
            Expects(target != nullptr);
            target->OnDataModelBoolValueChanged(args);
        }
    }

    ModelBase::OnValueChanged(args);
}

void DataModelBase::Transaction::ValueChanged(const BoolModelProperty::ChangeArgs& args)
{
    const IntModelProperty::ValueMap::const_iterator iter = m_mOriginalIntValues.find(args.Property.GetKey());
    if (iter == m_mOriginalIntValues.end())
    {
        m_mOriginalIntValues.insert_or_assign(args.Property.GetKey(), static_cast<int>(args.tOldValue));

#ifdef _DEBUG
        m_mDebugOriginalValues.insert_or_assign(args.Property.GetPropertyName(), args.tOldValue ? L"true" : L"false");
#endif
    }
    else
    {
        m_mOriginalIntValues.erase(iter);
    }
}

void DataModelBase::OnValueChanged(const StringModelProperty::ChangeArgs& args)
{
    if (m_pTransaction != nullptr && IsTransactional(args.Property))
    {
        m_pTransaction->ValueChanged(args);
        SetValue(IsModifiedProperty, m_pTransaction->IsModified());
    }

    if (!m_vNotifyTargets.empty())
    {
        // create a copy of the list of pointers in case it's modified by one of the callbacks
        NotifyTargetSet vNotifyTargets(m_vNotifyTargets);
        for (NotifyTarget* target : vNotifyTargets)
        {
            Expects(target != nullptr);
            target->OnDataModelStringValueChanged(args);
        }
    }

    ModelBase::OnValueChanged(args);
}

void DataModelBase::Transaction::ValueChanged(const StringModelProperty::ChangeArgs& args)
{
    const StringModelProperty::ValueMap::const_iterator iter = m_mOriginalStringValues.find(args.Property.GetKey());
    if (iter == m_mOriginalStringValues.end())
    {
        m_mOriginalStringValues.insert_or_assign(args.Property.GetKey(), args.tOldValue);

#ifdef _DEBUG
        m_mDebugOriginalValues.insert_or_assign(args.Property.GetPropertyName(), args.tOldValue);
#endif
    }
    else if (args.tNewValue == iter->second)
    {
        m_mOriginalStringValues.erase(iter);
    }
}

void DataModelBase::OnValueChanged(const IntModelProperty::ChangeArgs& args)
{
    if (m_pTransaction != nullptr && IsTransactional(args.Property))
    {
        m_pTransaction->ValueChanged(args);
        SetValue(IsModifiedProperty, m_pTransaction->IsModified());
    }

    if (!m_vNotifyTargets.empty())
    {
        // create a copy of the list of pointers in case it's modified by one of the callbacks
        NotifyTargetSet vNotifyTargets(m_vNotifyTargets);
        for (NotifyTarget* target : vNotifyTargets)
        {
            Expects(target != nullptr);
            target->OnDataModelIntValueChanged(args);
        }
    }

    ModelBase::OnValueChanged(args);
}

void DataModelBase::Transaction::ValueChanged(const IntModelProperty::ChangeArgs& args)
{
    const IntModelProperty::ValueMap::const_iterator iter = m_mOriginalIntValues.find(args.Property.GetKey());
    if (iter == m_mOriginalIntValues.end())
    {
        m_mOriginalIntValues.insert_or_assign(args.Property.GetKey(), args.tOldValue);

#ifdef _DEBUG
        m_mDebugOriginalValues.insert_or_assign(args.Property.GetPropertyName(), std::to_wstring(args.tOldValue));
#endif
    }
    else if (args.tNewValue == iter->second)
    {
        m_mOriginalIntValues.erase(iter);
    }
}

void DataModelBase::BeginTransaction()
{
    auto pTransaction = std::make_unique<Transaction>();
    pTransaction->m_pNext = std::move(m_pTransaction);
    m_pTransaction = std::move(pTransaction);

    SetValue(IsModifiedProperty, false);
}

void DataModelBase::CommitTransaction()
{
    if (m_pTransaction != nullptr)
    {
        m_pTransaction->Commit(*this);

        DiscardTransaction();
    }
}

void DataModelBase::DiscardTransaction()
{
    if (m_pTransaction != nullptr)
    {
        m_pTransaction = std::move(m_pTransaction->m_pNext);
        if (m_pTransaction == nullptr)
            SetValue(IsModifiedProperty, false);
        else
            SetValue(IsModifiedProperty, m_pTransaction->IsModified());
    }
}

void DataModelBase::RevertTransaction()
{
    if (m_pTransaction != nullptr)
    {
        // revert first so any captured changes are in the transaction we're about to discard
        m_pTransaction->Revert(*this);

        // discard the current transaction
        std::unique_ptr<Transaction> pTransaction = std::move(m_pTransaction);
        m_pTransaction = std::move(pTransaction->m_pNext);

        if (m_pTransaction == nullptr)
            SetValue(IsModifiedProperty, false);
        else
            SetValue(IsModifiedProperty, m_pTransaction->IsModified());
    }
}

void DataModelBase::Transaction::Revert(DataModelBase& vmViewModel)
{
    // swap out the map while we process it to prevent re-entrant calls to ValueChanged from
    // modifying it while we're iterating
    IntModelProperty::ValueMap mOriginalIntValues;
    mOriginalIntValues.swap(m_mOriginalIntValues);
    for (const auto& pPair : mOriginalIntValues)
    {
        const auto* pProperty = ra::data::ModelPropertyBase::GetPropertyForKey(pPair.first);
        const IntModelProperty* pIntProperty = dynamic_cast<const IntModelProperty*>(pProperty);
        if (pIntProperty != nullptr)
        {
            vmViewModel.SetValue(*pIntProperty, pPair.second);
        }
        else
        {
            const BoolModelProperty* pBoolProperty = dynamic_cast<const BoolModelProperty*>(pProperty);
            if (pBoolProperty != nullptr)
            {
                vmViewModel.SetValue(*pBoolProperty, pPair.second);
            }
        }
    }

    StringModelProperty::ValueMap mOriginalStringValues;
    mOriginalStringValues.swap(m_mOriginalStringValues);
    for (const auto& pPair : mOriginalStringValues)
    {
        const auto* pProperty = ra::data::ModelPropertyBase::GetPropertyForKey(pPair.first);
        const StringModelProperty* pStringProperty = dynamic_cast<const StringModelProperty*>(pProperty);
        if (pStringProperty != nullptr)
        {
            vmViewModel.SetValue(*pStringProperty, pPair.second);
        }
    }
}

void DataModelBase::Transaction::Commit(const DataModelBase& vmViewModel)
{
    if (!m_pNext)
        return;

    for (const auto& pPair : m_mOriginalIntValues)
    {
        const auto pScan = m_pNext->m_mOriginalIntValues.find(pPair.first);
        if (pScan == m_pNext->m_mOriginalIntValues.end())
        {
            // field was not modified in parent, move the original value into the parent
            m_pNext->m_mOriginalIntValues.insert_or_assign(pPair.first, pPair.second);
        }
        else
        {
            // field was modified in parent - if it's been modified back, remove the modification tracker
            // otherwise, leave the previous original value in place.
            const auto* pProperty = ra::data::ModelPropertyBase::GetPropertyForKey(pPair.first);
            const IntModelProperty* pIntProperty = dynamic_cast<const IntModelProperty*>(pProperty);
            if (pIntProperty != nullptr)
            {
                if (vmViewModel.GetValue(*pIntProperty) == pScan->second)
                    m_pNext->m_mOriginalIntValues.erase(pScan);
            }
            else
            {
                const BoolModelProperty* pBoolProperty = dynamic_cast<const BoolModelProperty*>(pProperty);
                if (pBoolProperty != nullptr)
                {
                    if (vmViewModel.GetValue(*pBoolProperty) == gsl::narrow_cast<bool>(pScan->second))
                        m_pNext->m_mOriginalIntValues.erase(pScan);
                }
            }
        }
    }

    for (const auto& pPair : m_mOriginalStringValues)
    {
        const auto pScan = m_pNext->m_mOriginalStringValues.find(pPair.first);
        if (pScan == m_pNext->m_mOriginalStringValues.end())
        {
            // field was not modified in parent, move the original value into the parent
            m_pNext->m_mOriginalStringValues.insert_or_assign(pPair.first, pPair.second);
        }
        else
        {
            // field was modified in parent - if it's been modified back, remove the modification tracker
            // otherwise, leave the previous original value in place.
            const auto* pProperty = ra::data::ModelPropertyBase::GetPropertyForKey(pPair.first);
            const StringModelProperty* pStringProperty = dynamic_cast<const StringModelProperty*>(pProperty);
            if (pStringProperty != nullptr)
            {
                if (vmViewModel.GetValue(*pStringProperty) == pScan->second)
                    m_pNext->m_mOriginalStringValues.erase(pScan);
            }
        }
    }
}

} // namespace ui
} // namespace ra
