#include "DataModelBase.hh"

namespace ra {
namespace data {

const BoolModelProperty DataModelBase::IsModifiedProperty("DataModelBase", "IsModified", false);

int DataModelBase::Transaction::CompareModelPropertyKey(const DataModelBase::Transaction::ModelPropertyValue& left, int nKey) noexcept
{
    return left.nKey < nKey;
}

const DataModelBase::Transaction::ModelPropertyValue* DataModelBase::Transaction::FindValue(int nKey) const
{
    const auto iter = std::lower_bound(m_vOriginalValues.begin(), m_vOriginalValues.end(), nKey, CompareModelPropertyKey);
    if (iter == m_vOriginalValues.end() || iter->nKey != nKey)
        return nullptr;

    return &(*iter);
}

void DataModelBase::OnValueChanged(const BoolModelProperty::ChangeArgs& args)
{
    if (&args != m_pEndUpdateChangeArgs)
    {
        if (m_pTransaction != nullptr && IsTransactional(args.Property))
        {
            m_pTransaction->ValueChanged(args);
            SetValue(IsModifiedProperty, m_pTransaction->IsModified());
        }

        if (m_pUpdateTransaction)
        {
            for (auto& pChange : m_pUpdateTransaction->m_vDelayedBoolChanges)
            {
                if (pChange.pProperty == &args.Property)
                {
                    pChange.tNewValue = args.tNewValue;
                    return;
                }
            }

            m_pUpdateTransaction->m_vDelayedBoolChanges.push_back({ &args.Property, args.tOldValue, args.tNewValue });
            return;
        }
    }

    if (m_vNotifyTargets.LockIfNotEmpty())
    {
        for (auto& target : m_vNotifyTargets.Targets())
            target.OnDataModelBoolValueChanged(args);

        m_vNotifyTargets.Unlock();
    }

    ModelBase::OnValueChanged(args);
}

void DataModelBase::Transaction::ValueChanged(const BoolModelProperty::ChangeArgs& args)
{
    auto iter = std::lower_bound(m_vOriginalValues.begin(), m_vOriginalValues.end(), args.Property.GetKey(), CompareModelPropertyKey);
    if (iter == m_vOriginalValues.end() || iter->nKey != args.Property.GetKey())
    {
        ModelPropertyValue value;
        value.nKey = args.Property.GetKey();
        value.nValue = args.tOldValue ? 1 : 0;
#ifdef _DEBUG
        value.sValue = args.tOldValue ? L"true" : L"false";
        value.pPropertyName = args.Property.GetPropertyName();
#endif
        m_vOriginalValues.insert(iter, std::move(value));
    }
    else if (args.tNewValue == (iter->nValue != 0))
    {
        m_vOriginalValues.erase(iter);
    }
}

void DataModelBase::OnValueChanged(const StringModelProperty::ChangeArgs& args)
{
    if (&args != m_pEndUpdateChangeArgs)
    {
        if (m_pTransaction != nullptr && IsTransactional(args.Property))
        {
            m_pTransaction->ValueChanged(args);
            SetValue(IsModifiedProperty, m_pTransaction->IsModified());
        }

        if (m_pUpdateTransaction)
        {
            for (auto& pChange : m_pUpdateTransaction->m_vDelayedStringChanges)
            {
                if (pChange.pProperty == &args.Property)
                {
                    pChange.tNewValue = args.tNewValue;
                    return;
                }
            }

            m_pUpdateTransaction->m_vDelayedStringChanges.push_back({ &args.Property, args.tOldValue, args.tNewValue });
            return;
        }
    }

    if (m_vNotifyTargets.LockIfNotEmpty())
    {
        for (auto& target : m_vNotifyTargets.Targets())
            target.OnDataModelStringValueChanged(args);

        m_vNotifyTargets.Unlock();
    }

    ModelBase::OnValueChanged(args);
}

void DataModelBase::Transaction::ValueChanged(const StringModelProperty::ChangeArgs& args)
{
    auto iter = std::lower_bound(m_vOriginalValues.begin(), m_vOriginalValues.end(), args.Property.GetKey(), CompareModelPropertyKey);
    if (iter == m_vOriginalValues.end() || iter->nKey != args.Property.GetKey())
    {
        ModelPropertyValue value;
        value.nKey = args.Property.GetKey();
        value.nValue = 0;
        value.sValue = args.tOldValue;
#ifdef _DEBUG
        value.pPropertyName = args.Property.GetPropertyName();
#endif
        m_vOriginalValues.insert(iter, std::move(value));
    }
    else if (args.tNewValue == iter->sValue)
    {
        m_vOriginalValues.erase(iter);
    }
}

void DataModelBase::OnValueChanged(const IntModelProperty::ChangeArgs& args)
{
    if (&args != m_pEndUpdateChangeArgs)
    {
        if (m_pTransaction != nullptr && IsTransactional(args.Property))
        {
            m_pTransaction->ValueChanged(args);
            SetValue(IsModifiedProperty, m_pTransaction->IsModified());
        }

        if (m_pUpdateTransaction)
        {
            for (auto& pChange : m_pUpdateTransaction->m_vDelayedIntChanges)
            {
                if (pChange.pProperty == &args.Property)
                {
                    pChange.tNewValue = args.tNewValue;
                    return;
                }
            }

            m_pUpdateTransaction->m_vDelayedIntChanges.push_back({ &args.Property, args.tOldValue, args.tNewValue });
            return;
        }
    }

    if (m_vNotifyTargets.LockIfNotEmpty())
    {
        for (auto& target : m_vNotifyTargets.Targets())
            target.OnDataModelIntValueChanged(args);

        m_vNotifyTargets.Unlock();
    }

    ModelBase::OnValueChanged(args);
}

void DataModelBase::Transaction::ValueChanged(const IntModelProperty::ChangeArgs& args)
{
    auto iter = std::lower_bound(m_vOriginalValues.begin(), m_vOriginalValues.end(), args.Property.GetKey(), CompareModelPropertyKey);
    if (iter == m_vOriginalValues.end() || iter->nKey != args.Property.GetKey())
    {
        ModelPropertyValue value;
        value.nKey = args.Property.GetKey();
        value.nValue = args.tOldValue;
        value.sValue.clear();
#ifdef _DEBUG
        value.pPropertyName = args.Property.GetPropertyName();
#endif
        m_vOriginalValues.insert(iter, std::move(value));
    }
    else if (args.tNewValue == iter->nValue)
    {
        m_vOriginalValues.erase(iter);
    }
}

void DataModelBase::BeginUpdate()
{
    if (m_pUpdateTransaction == nullptr)
        m_pUpdateTransaction = std::make_unique<UpdateTransaction>();

    ++m_pUpdateTransaction->m_nUpdateCount;
}

void DataModelBase::EndUpdate()
{
    if (m_pUpdateTransaction && --m_pUpdateTransaction->m_nUpdateCount == 0)
    {
        std::unique_ptr<UpdateTransaction> pUpdateTransaction = std::move(m_pUpdateTransaction);

        // m_pEndUpdateChangeArgs is set to each ChangeArgs as we call OnValueChanged so it doesn't try to
        // apply the change to the transaction - the transaction was updated the first time it was called

        for (const auto& pChange : pUpdateTransaction->m_vDelayedIntChanges)
        {
            if (pChange.tNewValue != pChange.tOldValue)
            {
                const IntModelProperty::ChangeArgs args{ *pChange.pProperty, pChange.tOldValue, pChange.tNewValue };
                m_pEndUpdateChangeArgs = &args;
                OnValueChanged(args);
            }
        }

        for (const auto& pChange : pUpdateTransaction->m_vDelayedBoolChanges)
        {
            if (pChange.tNewValue != pChange.tOldValue)
            {
                const BoolModelProperty::ChangeArgs args{ *pChange.pProperty, pChange.tOldValue, pChange.tNewValue };
                m_pEndUpdateChangeArgs = &args;
                OnValueChanged(args);
            }
        }

        for (const auto& pChange : pUpdateTransaction->m_vDelayedStringChanges)
        {
            if (pChange.tNewValue != pChange.tOldValue)
            {
                const StringModelProperty::ChangeArgs args{ *pChange.pProperty, pChange.tOldValue, pChange.tNewValue };
                m_pEndUpdateChangeArgs = &args;
                OnValueChanged(args);
            }
        }

        m_pEndUpdateChangeArgs = nullptr;
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

void DataModelBase::Transaction::Revert(DataModelBase& pModel)
{
    // swap out the map while we process it to prevent re-entrant calls to ValueChanged from
    // modifying it while we're iterating
    std::vector<ModelPropertyValue> vOriginalValues;
    vOriginalValues.swap(m_vOriginalValues);
    for (const auto& pValue : vOriginalValues)
    {
        const auto* pProperty = ra::data::ModelPropertyBase::GetPropertyForKey(pValue.nKey);

        const IntModelProperty* pIntProperty = dynamic_cast<const IntModelProperty*>(pProperty);
        if (pIntProperty != nullptr)
        {
            pModel.SetValue(*pIntProperty, pValue.nValue);
        }
        else
        {
            const StringModelProperty* pStringProperty = dynamic_cast<const StringModelProperty*>(pProperty);
            if (pStringProperty != nullptr)
            {
                pModel.SetValue(*pStringProperty, pValue.sValue);
            }
            else
            {
                const BoolModelProperty* pBoolProperty = dynamic_cast<const BoolModelProperty*>(pProperty);
                if (pBoolProperty != nullptr)
                    pModel.SetValue(*pBoolProperty, (pValue.nValue != 0));
            }
        }
    }
}

void DataModelBase::Transaction::Commit(const DataModelBase& pModel)
{
    if (!m_pNext)
        return;

    auto& pNextOriginalValues = m_pNext->m_vOriginalValues;
    for (const auto& pValue : m_vOriginalValues)
    {
        const auto iter = std::lower_bound(pNextOriginalValues.begin(), pNextOriginalValues.end(), pValue.nKey, CompareModelPropertyKey);
        if (iter == pNextOriginalValues.end() || iter->nKey != pValue.nKey)
        {
            // field was not modified in parent, move the original value into the parent
            ModelPropertyValue value;
            value.nKey = pValue.nKey;
            value.nValue = pValue.nValue;
            value.sValue = pValue.sValue;
#ifdef _DEBUG
            value.pPropertyName = pValue.pPropertyName;
#endif
            pNextOriginalValues.insert(iter, std::move(value));
        }
        else
        {
            // field was modified in parent - if it's been modified back, remove the modification tracker
            // otherwise, leave the previous original value in place.
            const auto* pProperty = ra::data::ModelPropertyBase::GetPropertyForKey(pValue.nKey);

            const IntModelProperty* pIntProperty = dynamic_cast<const IntModelProperty*>(pProperty);
            if (pIntProperty != nullptr)
            {
                if (pModel.GetValue(*pIntProperty) == iter->nValue)
                    pNextOriginalValues.erase(iter);
            }
            else
            {
                const StringModelProperty* pStringProperty = dynamic_cast<const StringModelProperty*>(pProperty);
                if (pStringProperty != nullptr)
                {
                    if (pModel.GetValue(*pStringProperty) == iter->sValue)
                        pNextOriginalValues.erase(iter);
                }
                else
                {
                    const BoolModelProperty* pBoolProperty = dynamic_cast<const BoolModelProperty*>(pProperty);
                    if (pBoolProperty != nullptr)
                    {
                        if (pModel.GetValue(*pBoolProperty) == (iter->nValue != 0))
                            pNextOriginalValues.erase(iter);
                    }
                }
            }
        }
    }
}

} // namespace data
} // namespace ra
