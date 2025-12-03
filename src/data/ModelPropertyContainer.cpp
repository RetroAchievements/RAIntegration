#include "ModelPropertyContainer.hh"

namespace ra {
namespace data {

std::wstring ModelPropertyContainer::s_sEmpty;

int ModelPropertyContainer::CompareModelPropertyKey(const ModelPropertyContainer::ModelPropertyValue& left, int nKey) noexcept
{
    return left.nKey < nKey;
}

const int* ModelPropertyContainer::FindValue(int nKey) const
{
    std::lock_guard<std::mutex> pLock(m_mMutex);

    const auto iter = std::lower_bound(m_vValues.begin(), m_vValues.end(), nKey, CompareModelPropertyKey);
    if (iter == m_vValues.end() || iter->nKey != nKey)
        return nullptr;

    return &iter->nValue;
}

void ModelPropertyContainer::SetValue(const BoolModelProperty& pProperty, bool bValue)
{
    const int nValue = bValue ? 1 : 0;
    {
        std::lock_guard<std::mutex> pLock(m_mMutex);

        auto iter = std::lower_bound(m_vValues.begin(), m_vValues.end(), pProperty.GetKey(), CompareModelPropertyKey);
        if (iter == m_vValues.end() || iter->nKey != pProperty.GetKey())
        {
            if (bValue == pProperty.GetDefaultValue())
                return;

            m_vValues.emplace(iter, pProperty.GetKey(), nValue);
        }
        else
        {
            if (nValue == iter->nValue)
                return;

            if (bValue == pProperty.GetDefaultValue())
                m_vValues.erase(iter);
            else
                iter->nValue = nValue;
        }

#ifdef _DEBUG
        m_mDebugValues.insert_or_assign(pProperty.GetPropertyName(), bValue ? L"true" : L"false");
#endif
    }

    const BoolModelProperty::ChangeArgs args{ pProperty, !bValue, bValue };
    OnValueChanged(args);
}

void ModelPropertyContainer::OnValueChanged(const BoolModelProperty::ChangeArgs&)
{
}

const std::wstring& ModelPropertyContainer::GetString(int nIndex) const noexcept
{
    if (nIndex < 0)
        return s_sEmpty;

    const ModelPropertyStrings* pStrings = m_pStrings.get();
    while (nIndex >= ModelPropertyStrings::ChunkCount && pStrings)
    {
        pStrings = pStrings->pNext.get();
        nIndex -= ModelPropertyStrings::ChunkCount;
    }

    if (!pStrings)
        return s_sEmpty;

    return gsl::at(pStrings->sStrings, nIndex);
}

int ModelPropertyContainer::LoadIntoEmptyStringSlot(const std::wstring& sValue)
{
    int nValue = 0;

    gsl::not_null<std::unique_ptr<ModelPropertyStrings>*> pPrevious = gsl::make_not_null(&m_pStrings);
    for (auto* pStrings = m_pStrings.get(); pStrings; pStrings = pStrings->pNext.get())
    {
        for (size_t i = 0; i < ModelPropertyStrings::ChunkCount; i++)
        {
            if (gsl::at(pStrings->sStrings, i).empty())
            {
                gsl::at(pStrings->sStrings, i) = sValue;
                return nValue;
            }

            ++nValue;
        }

        pPrevious = gsl::make_not_null(&pStrings->pNext);
    }

    // no empty slot found, allocate one
    *pPrevious = std::make_unique<ModelPropertyStrings>();
    (*pPrevious)->sStrings[0] = sValue;
    return nValue;
}

void ModelPropertyContainer::SetValue(const StringModelProperty& pProperty, const std::wstring& sValue)
{
    const std::wstring* pOldValue = nullptr;
    std::wstring sOldValue;

    {
        std::lock_guard<std::mutex> pLock(m_mMutex);

        auto iter = std::lower_bound(m_vValues.begin(), m_vValues.end(), pProperty.GetKey(), CompareModelPropertyKey);
        if (iter == m_vValues.end() || iter->nKey != pProperty.GetKey())
        {
            // no entry for property. if setting to default, do nothing
            if (sValue == pProperty.GetDefaultValue())
                return;

            // find a slot for the new value
            int nValue = 0;
            if (sValue.empty())
            {
                // negative value for empty string
                nValue = -1;
            }
            else
            {
                // find an empty slot (not in use)
                nValue = LoadIntoEmptyStringSlot(sValue);
            }

            m_vValues.emplace(iter, pProperty.GetKey(), nValue);
            pOldValue = &pProperty.GetDefaultValue();
        }
        else
        {
            pOldValue = &sOldValue;
            gsl::not_null<std::wstring*> sString = gsl::make_not_null(&sOldValue);

            if (iter->nValue == -1)
            {
                // negative nValue is empty string. if new value is also empty, do nothing
                if (sValue.empty())
                    return;

                iter->nValue = LoadIntoEmptyStringSlot(sValue);
            }
            else
            {
                // find the current value. if the new value is the same, do nothing
                ModelPropertyStrings* pStrings = m_pStrings.get();
                int nIndex = iter->nValue;
                while (nIndex >= ModelPropertyStrings::ChunkCount && pStrings)
                {
                    pStrings = pStrings->pNext.get();
                    nIndex -= ModelPropertyStrings::ChunkCount;
                }

                if (pStrings == nullptr)
                    return;

                sString = gsl::make_not_null(&gsl::at(pStrings->sStrings, nIndex));
                if (sValue == *sString)
                    return;

                // move the current value into a temporary variable for the ChangeArgs
                sString->swap(sOldValue);

                if (sValue == pProperty.GetDefaultValue())
                {
                    // setting value back to default. eliminate the value pair.
                    m_vValues.erase(iter);
                }
                else
                {
                    // put the new string in the slot.
                    *sString = sValue;
                }
            }
        }

#ifdef _DEBUG
        m_mDebugValues.insert_or_assign(pProperty.GetPropertyName(), sValue);
#endif
    }

    const StringModelProperty::ChangeArgs args{ pProperty, *pOldValue, sValue };
    OnValueChanged(args);
}

void ModelPropertyContainer::OnValueChanged(const StringModelProperty::ChangeArgs&)
{
}

void ModelPropertyContainer::SetValue(const IntModelProperty& pProperty, int nValue)
{
    int nOldValue = 0;
    {
        std::lock_guard<std::mutex> pLock(m_mMutex);

        auto iter = std::lower_bound(m_vValues.begin(), m_vValues.end(), pProperty.GetKey(), CompareModelPropertyKey);
        if (iter == m_vValues.end() || iter->nKey != pProperty.GetKey())
        {
            nOldValue = pProperty.GetDefaultValue();
            if (nValue == nOldValue)
                return;

            m_vValues.emplace(iter, pProperty.GetKey(), nValue);
        }
        else
        {
            nOldValue = iter->nValue;
            if (nValue == nOldValue)
                return;

            if (nValue == pProperty.GetDefaultValue())
                m_vValues.erase(iter);
            else
                iter->nValue = nValue;
        }

#ifdef _DEBUG
        m_mDebugValues.insert_or_assign(pProperty.GetPropertyName(), std::to_wstring(nValue));
#endif
    }

    const IntModelProperty::ChangeArgs args{ pProperty, nOldValue, nValue };
    OnValueChanged(args);
}

void ModelPropertyContainer::OnValueChanged(const IntModelProperty::ChangeArgs&)
{
}

} // namespace ui
} // namespace ra
