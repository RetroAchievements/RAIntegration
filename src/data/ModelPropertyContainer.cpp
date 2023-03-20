#include "ModelPropertyContainer.hh"

namespace ra {
namespace data {

void ModelPropertyContainer::SetValue(const BoolModelProperty& pProperty, bool bValue)
{
    const IntModelProperty::ValueMap::const_iterator iter = m_mIntValues.find(pProperty.GetKey());

    if (bValue == pProperty.GetDefaultValue())
    {
        if (iter == m_mIntValues.end())
            return;

        m_mIntValues.erase(iter);
    }
    else
    {
        if (iter != m_mIntValues.end())
            return;

        m_mIntValues.insert_or_assign(pProperty.GetKey(), static_cast<int>(bValue));
    }

#ifdef _DEBUG
    m_mDebugValues.insert_or_assign(pProperty.GetPropertyName(), bValue ? L"true" : L"false");
#endif

    const BoolModelProperty::ChangeArgs args{ pProperty, !bValue, bValue };
    OnValueChanged(args);
}

void ModelPropertyContainer::OnValueChanged(const BoolModelProperty::ChangeArgs&)
{
}

void ModelPropertyContainer::SetValue(const StringModelProperty& pProperty, const std::wstring& sValue)
{
    const StringModelProperty::ValueMap::iterator iter = m_mStringValues.find(pProperty.GetKey());
    std::wstring sOldValue;
    const std::wstring* pOldValue{};

    if (sValue == pProperty.GetDefaultValue())
    {
        if (iter == m_mStringValues.end())
        {
            // value didn't change
            return;
        }

        // changed to default value, remove from map
        sOldValue = iter->second;
        pOldValue = &sOldValue;
        m_mStringValues.erase(iter);
    }
    else if (iter == m_mStringValues.end())
    {
        // not in map, add it
        pOldValue = &(pProperty.GetDefaultValue());
        m_mStringValues.insert_or_assign(pProperty.GetKey(), sValue);
    }
    else if (iter->second != sValue)
    {
        // update map
        sOldValue = iter->second;
        pOldValue = &sOldValue;
        iter->second = sValue;
    }
    else
    {
        // value didn't change
        return;
    }

#ifdef _DEBUG
    m_mDebugValues.insert_or_assign(pProperty.GetPropertyName(), sValue);
#endif

    const StringModelProperty::ChangeArgs args{ pProperty, *pOldValue, sValue };
    OnValueChanged(args);
}

void ModelPropertyContainer::OnValueChanged(const StringModelProperty::ChangeArgs&)
{
}

void ModelPropertyContainer::SetValue(const IntModelProperty& pProperty, int nValue)
{
    const IntModelProperty::ValueMap::iterator iter = m_mIntValues.find(pProperty.GetKey());
    int nOldValue{};

    if (nValue == pProperty.GetDefaultValue())
    {
        // already default, do nothing
        if (iter == m_mIntValues.end())
            return;

        // changed to default value, remove from map
        nOldValue = iter->second;
        m_mIntValues.erase(iter);
    }
    else if (iter == m_mIntValues.end())
    {
        // not in map, add it
        m_mIntValues.insert_or_assign(pProperty.GetKey(), nValue);
        nOldValue = pProperty.GetDefaultValue();
    }
    else if (iter->second != nValue)
    {
        // update map
        nOldValue = iter->second;
        iter->second = nValue;
    }
    else
    {
        // value didn't change
        return;
    }

#ifdef _DEBUG
    m_mDebugValues.insert_or_assign(pProperty.GetPropertyName(), std::to_wstring(nValue));
#endif

    const IntModelProperty::ChangeArgs args{ pProperty, nOldValue, nValue };
    OnValueChanged(args);
}

void ModelPropertyContainer::OnValueChanged(const IntModelProperty::ChangeArgs&)
{
}

} // namespace ui
} // namespace ra
