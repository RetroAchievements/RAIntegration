#include "TransactionalViewModelBase.hh"

namespace ra {
namespace ui {

const BoolModelProperty TransactionalViewModelBase::IsModifiedProperty("TransactionalViewModelBase", "IsModified", false);

void TransactionalViewModelBase::SetTransactionalValue(const BoolModelProperty& pProperty, bool bValue)
{
    if (m_pTransaction != nullptr)
    {
        const bool bCurrentValue = GetValue(pProperty);
        if (bValue != bCurrentValue)
        {
            m_pTransaction->SetValue(pProperty, bValue, bCurrentValue);
            SetValue(pProperty, bValue);
            SetValue(IsModifiedProperty, m_pTransaction->IsModified());
        }
    }
    else
    {
        SetValue(pProperty, bValue);
    }
}

void TransactionalViewModelBase::Transaction::SetValue(const BoolModelProperty& pProperty, bool bValue, bool bCurrentValue)
{
    const IntModelProperty::ValueMap::const_iterator iter = m_mOriginalIntValues.find(pProperty.GetKey());
    if (iter == m_mOriginalIntValues.end())
    {
        m_mOriginalIntValues.insert_or_assign(pProperty.GetKey(), static_cast<int>(bCurrentValue));

#ifdef _DEBUG
        m_mDebugOriginalValues.insert_or_assign(pProperty.GetPropertyName(), bCurrentValue ? L"true" : L"false");
#endif
    }
    else
    {
        m_mOriginalIntValues.erase(iter);
    }

    if (m_pNext != nullptr)
        m_pNext->SetValue(pProperty, bValue, bCurrentValue);
}

void TransactionalViewModelBase::SetTransactionalValue(const StringModelProperty& pProperty, const std::wstring& sValue)
{
    if (m_pTransaction != nullptr)
    {
        const std::wstring& sCurrentValue = GetValue(pProperty);
        if (sValue != sCurrentValue)
        {
            m_pTransaction->SetValue(pProperty, sValue, sCurrentValue);
            SetValue(pProperty, sValue);
            SetValue(IsModifiedProperty, m_pTransaction->IsModified());
        }
    }
    else
    {
        SetValue(pProperty, sValue);
    }
}

void TransactionalViewModelBase::Transaction::SetValue(const StringModelProperty& pProperty, const std::wstring& sValue, const std::wstring& sCurrentValue)
{
    const StringModelProperty::ValueMap::const_iterator iter = m_mOriginalStringValues.find(pProperty.GetKey());
    if (iter == m_mOriginalStringValues.end())
    {
        m_mOriginalStringValues.insert_or_assign(pProperty.GetKey(), sCurrentValue);

#ifdef _DEBUG
        m_mDebugOriginalValues.insert_or_assign(pProperty.GetPropertyName(), sCurrentValue);
#endif
    }
    else if (sValue == iter->second)
    {
        m_mOriginalStringValues.erase(iter);
    }

    if (m_pNext != nullptr)
        m_pNext->SetValue(pProperty, sValue, sCurrentValue);
}

void TransactionalViewModelBase::SetTransactionalValue(const IntModelProperty& pProperty, int nValue)
{
    if (m_pTransaction != nullptr)
    {
        const int nCurrentValue = GetValue(pProperty);
        if (nValue != nCurrentValue)
        {
            m_pTransaction->SetValue(pProperty, nValue, nCurrentValue);
            SetValue(pProperty, nValue);
            SetValue(IsModifiedProperty, m_pTransaction->IsModified());
        }
    }
    else
    {
        SetValue(pProperty, nValue);
    }
}

void TransactionalViewModelBase::Transaction::SetValue(const IntModelProperty& pProperty, int nValue, int nCurrentValue)
{
    const IntModelProperty::ValueMap::const_iterator iter = m_mOriginalIntValues.find(pProperty.GetKey());
    if (iter == m_mOriginalIntValues.end())
    {
        m_mOriginalIntValues.insert_or_assign(pProperty.GetKey(), nCurrentValue);

#ifdef _DEBUG
        m_mDebugOriginalValues.insert_or_assign(pProperty.GetPropertyName(), std::to_wstring(nCurrentValue));
#endif
    }
    else if (nValue == iter->second)
    {
        m_mOriginalIntValues.erase(iter);
    }

    if (m_pNext != nullptr)
        m_pNext->SetValue(pProperty, nValue, nCurrentValue);
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
        m_pTransaction->Revert(*this);

        DiscardTransaction();
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
            if (m_pNext)
                m_pNext->SetValue(*pIntProperty, pPair.second, vmViewModel.GetValue(*pIntProperty));

            vmViewModel.SetValue(*pIntProperty, pPair.second);
        }
        else
        {
            const BoolModelProperty* pBoolProperty = dynamic_cast<const BoolModelProperty*>(pProperty);
            if (pBoolProperty != nullptr)
            {
                if (m_pNext)
                    m_pNext->SetValue(*pBoolProperty, pPair.second, vmViewModel.GetValue(*pBoolProperty));

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
            if (m_pNext)
                m_pNext->SetValue(*pStringProperty, pPair.second, vmViewModel.GetValue(*pStringProperty));

            vmViewModel.SetValue(*pStringProperty, pPair.second);
        }
    }
}

} // namespace ui
} // namespace ra
