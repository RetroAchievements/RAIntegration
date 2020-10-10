#include "TransactionalViewModelBase.hh"

namespace ra {
namespace ui {

const BoolModelProperty TransactionalViewModelBase::IsModifiedProperty("TransactionalViewModelBase", "IsModified", false);

void TransactionalViewModelBase::OnValueChanged(const BoolModelProperty::ChangeArgs& args)
{
    if (m_pTransaction != nullptr && IsTransactional(args.Property))
    {
        m_pTransaction->ValueChanged(args);
        SetValue(IsModifiedProperty, m_pTransaction->IsModified());
    }

    ViewModelBase::OnValueChanged(args);
}

void TransactionalViewModelBase::Transaction::ValueChanged(const BoolModelProperty::ChangeArgs& args)
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

void TransactionalViewModelBase::OnValueChanged(const StringModelProperty::ChangeArgs& args)
{
    if (m_pTransaction != nullptr && IsTransactional(args.Property))
    {
        m_pTransaction->ValueChanged(args);
        SetValue(IsModifiedProperty, m_pTransaction->IsModified());
    }

    ViewModelBase::OnValueChanged(args);
}

void TransactionalViewModelBase::Transaction::ValueChanged(const StringModelProperty::ChangeArgs& args)
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

void TransactionalViewModelBase::OnValueChanged(const IntModelProperty::ChangeArgs& args)
{
    if (m_pTransaction != nullptr && IsTransactional(args.Property))
    {
        m_pTransaction->ValueChanged(args);
        SetValue(IsModifiedProperty, m_pTransaction->IsModified());
    }

    ViewModelBase::OnValueChanged(args);
}

void TransactionalViewModelBase::Transaction::ValueChanged(const IntModelProperty::ChangeArgs& args)
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

void TransactionalViewModelBase::BeginTransaction()
{
    auto pTransaction = std::make_unique<Transaction>();
    pTransaction->m_pNext = std::move(m_pTransaction);
    m_pTransaction = std::move(pTransaction);

    SetValue(IsModifiedProperty, false);
}

void TransactionalViewModelBase::CommitTransaction()
{
    if (m_pTransaction != nullptr)
    {
        m_pTransaction->Commit(*this);

        DiscardTransaction();
    }
}

void TransactionalViewModelBase::DiscardTransaction()
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

void TransactionalViewModelBase::RevertTransaction()
{
    if (m_pTransaction != nullptr)
    {
        std::unique_ptr<Transaction> pTransaction = std::move(m_pTransaction);
        m_pTransaction = std::move(pTransaction->m_pNext);

        pTransaction->Revert(*this);

        if (m_pTransaction == nullptr)
            SetValue(IsModifiedProperty, false);
        else
            SetValue(IsModifiedProperty, m_pTransaction->IsModified());
    }
}

void TransactionalViewModelBase::Transaction::Revert(TransactionalViewModelBase& vmViewModel)
{
    for (const auto& pPair : m_mOriginalIntValues)
    {
        const ModelPropertyBase* pProperty = ModelPropertyBase::GetPropertyForKey(pPair.first);
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

    for (const auto& pPair : m_mOriginalStringValues)
    {
        const ModelPropertyBase* pProperty = ModelPropertyBase::GetPropertyForKey(pPair.first);
        const StringModelProperty* pStringProperty = dynamic_cast<const StringModelProperty*>(pProperty);
        if (pStringProperty != nullptr)
        {
            vmViewModel.SetValue(*pStringProperty, pPair.second);
        }
    }
}

void TransactionalViewModelBase::Transaction::Commit(const TransactionalViewModelBase& vmViewModel)
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
            const ModelPropertyBase* pProperty = ModelPropertyBase::GetPropertyForKey(pPair.first);
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
            const ModelPropertyBase* pProperty = ModelPropertyBase::GetPropertyForKey(pPair.first);
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
