#include "ViewModelBase.hh"

namespace ra {
namespace ui {

void ViewModelBase::SetValue(const BoolModelProperty& pProperty, bool bValue)
{
    IntModelProperty::ValueMap::const_iterator iter = m_mIntValues.find(pProperty.GetKey());

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

        m_mIntValues[pProperty.GetKey()] = static_cast<int>(bValue);
    }

#ifdef _DEBUG
    m_mDebugValues[pProperty.GetPropertyName()] = bValue ? L"true" : L"false";
#endif

    if (!m_vNotifyTargets.empty())
    {
        BoolModelProperty::ChangeArgs args{ pProperty, !bValue, bValue };

		// create a copy of the list of pointers in case it's modified by one of the callbacks
		NotifyTargetSet vNotifyTargets(m_vNotifyTargets);
        for (NotifyTarget* target : vNotifyTargets)
            target->OnViewModelBoolValueChanged(args);
    }
}

void ViewModelBase::SetValue(const StringModelProperty& pProperty, const std::wstring& sValue)
{
    StringModelProperty::ValueMap::iterator iter = m_mStringValues.find(pProperty.GetKey());
    std::wstring sOldValue;
    const std::wstring* pOldValue;

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
        m_mStringValues[pProperty.GetKey()] = sValue;
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
    m_mDebugValues[pProperty.GetPropertyName()] = sValue;
#endif

    if (!m_vNotifyTargets.empty())
    {
        StringModelProperty::ChangeArgs args{ pProperty, *pOldValue, sValue };

		// create a copy of the list of pointers in case it's modified by one of the callbacks
        NotifyTargetSet vNotifyTargets(m_vNotifyTargets);
        for (NotifyTarget* target : vNotifyTargets)
            target->OnViewModelStringValueChanged(args);
    }
}

void ViewModelBase::SetValue(const IntModelProperty& pProperty, int nValue)
{
    IntModelProperty::ValueMap::iterator iter = m_mIntValues.find(pProperty.GetKey());
    int nOldValue;

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
		m_mIntValues[pProperty.GetKey()] = nValue;
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
    m_mDebugValues[pProperty.GetPropertyName()] = std::to_wstring(nValue);
#endif

    if (!m_vNotifyTargets.empty())
    {
        IntModelProperty::ChangeArgs args{ pProperty, nOldValue, nValue };

		// create a copy of the list of pointers in case it's modified by one of the callbacks
		NotifyTargetSet vNotifyTargets(m_vNotifyTargets);
        for (NotifyTarget* target : vNotifyTargets)
            target->OnViewModelIntValueChanged(args);
    }
}

} // namespace ui
} // namespace ra
