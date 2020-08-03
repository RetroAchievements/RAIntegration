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

    if (m_pNext != nullptr)
        m_pNext->ValueChanged(args);
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

    if (m_pNext != nullptr)
        m_pNext->ValueChanged(args);
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

    if (m_pNext != nullptr)
        m_pNext->ValueChanged(args);
}

void TransactionalViewModelBase::BeginTransaction()
{
    auto pTransaction = std::make_shared<Transaction>();
    pTransaction->m_pNext = m_pTransaction;
    m_pTransaction = std::move(pTransaction);

    SetValue(IsModifiedProperty, false);
}

void TransactionalViewModelBase::CommitTransaction()
{
    // ASSERT: Transaction::SetValue propogates the change up the chain, so the parent
    // transaction already has the value.
    if (m_pTransaction != nullptr)
        DiscardTransaction();
}

void TransactionalViewModelBase::DiscardTransaction()
{
    m_pTransaction = std::move(m_pTransaction->m_pNext);
    if (m_pTransaction == nullptr)
        SetValue(IsModifiedProperty, false);
    else
        SetValue(IsModifiedProperty, m_pTransaction->IsModified());
}

void TransactionalViewModelBase::RevertTransaction()
{
    if (m_pTransaction != nullptr)
    {
        std::shared_ptr<Transaction> pTransaction = m_pTransaction;
        DiscardTransaction();

        pTransaction->Revert(*this);
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

} // namespace ui
} // namespace ra
