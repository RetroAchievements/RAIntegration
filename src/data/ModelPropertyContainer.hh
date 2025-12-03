#ifndef RA_DATA_MODEL_PROPERTY_CONTAINER_H
#define RA_DATA_MODEL_PROPERTY_CONTAINER_H
#pragma once

#include "data\ModelProperty.hh"

#include <mutex>

namespace ra {
namespace data {

class ModelPropertyContainer
{
public:
    GSL_SUPPRESS_F6 ModelPropertyContainer() = default;

#ifdef DEBUG
    virtual ~ModelPropertyContainer() noexcept {
        m_bDestructed = true;
    }

    bool m_bDestructed = false;
#else
    virtual ~ModelPropertyContainer() noexcept = default;
#endif

    ModelPropertyContainer(const ModelPropertyContainer&) = delete;
    ModelPropertyContainer& operator=(const ModelPropertyContainer&) = delete;
    GSL_SUPPRESS_F6 ModelPropertyContainer(ModelPropertyContainer&&) = delete;
    GSL_SUPPRESS_F6 ModelPropertyContainer& operator=(ModelPropertyContainer&&) = delete;

    /// <summary>
    /// Gets the value associated to the requested boolean property.
    /// </summary>
    /// <param name="pProperty">The property to query.</param>
    /// <returns>The current value of the property for this object.</returns>
    bool GetValue(const BoolModelProperty& pProperty) const
    {
        const int* pValue = FindValue(pProperty.GetKey());
        return pValue ? (*pValue != 0) : pProperty.GetDefaultValue();
    }

    /// <summary>
    /// Sets the specified boolean property to the specified value.
    /// </summary>
    /// <param name="pProperty">The property to set.</param>
    /// <param name="bValue">The value to set.</param>
    void SetValue(const BoolModelProperty& pProperty, bool bValue);

    /// <summary>
    /// Called when a boolean value changes.
    /// </summary>
    /// <param name="args">Information about the change.</param>
    virtual void OnValueChanged(const BoolModelProperty::ChangeArgs& args) noexcept(false);

    /// <summary>
    /// Gets the value associated to the requested string property.
    /// </summary>
    /// <param name="pProperty">The property to query.</param>
    /// <returns>The current value of the property for this object.</returns>
    const std::wstring& GetValue(const StringModelProperty& pProperty) const
    {
        const int* pValue = FindValue(pProperty.GetKey());
        if (pValue == nullptr)
            return pProperty.GetDefaultValue();

        return GetString(*pValue);
    }

    /// <summary>
    /// Sets the specified string property to the specified value.
    /// </summary>
    /// <param name="pProperty">The property to set.</param>
    /// <param name="sValue">The value to set.</param>
    void SetValue(const StringModelProperty& pProperty, const std::wstring& sValue);

    /// <summary>
    /// Called when a string value changes.
    /// </summary>
    /// <param name="args">Information about the change.</param>
    virtual void OnValueChanged(const StringModelProperty::ChangeArgs& args) noexcept(false);

    /// <summary>
    /// Gets the value associated to the requested integer property.
    /// </summary>
    /// <param name="pProperty">The property to query.</param>
    /// <returns>The current value of the property for this object.</returns>
    int GetValue(const IntModelProperty& pProperty) const
    {
        const int* pValue = FindValue(pProperty.GetKey());
        return pValue ? *pValue : pProperty.GetDefaultValue();
    }

    /// <summary>
    /// Sets the specified integer property to the specified value.
    /// </summary>
    /// <param name="pProperty">The property to set.</param>
    /// <param name="nValue">The value to set.</param>
    void SetValue(const IntModelProperty& pProperty, int nValue);

    /// <summary>
    /// Called when a integer value changes.
    /// </summary>
    /// <param name="args">Information about the change.</param>
    virtual void OnValueChanged(const IntModelProperty::ChangeArgs& args) noexcept(false);

private:
    typedef struct ModelPropertyValue
    {
        ModelPropertyValue(int nKey, int nValue) noexcept
            : nKey(nKey), nValue(nValue)
        {
        }

        int nKey;
        int nValue;
    } ModelPropertyValue;
    std::vector<ModelPropertyValue> m_vValues;

    typedef struct ModelPropertyStrings
    {
        static constexpr size_t ChunkCount = 4;

        std::wstring sStrings[ChunkCount];
        std::unique_ptr<struct ModelPropertyStrings> pNext;
    } ModelPropertyStrings;
    std::unique_ptr<ModelPropertyStrings> m_pStrings;

#ifdef _DEBUG
    /// <summary>
    /// Complete list of values as strings for viewing in the debugger
    /// </summary>
    std::map<std::string, std::wstring> m_mDebugValues;
#endif

    static std::wstring s_sEmpty;

    static int CompareModelPropertyKey(const ModelPropertyValue& left, int nKey) noexcept;
    const int* FindValue(int nKey) const;
    const std::wstring& GetString(int nIndex) const noexcept;
    int LoadIntoEmptyStringSlot(const std::wstring& sValue);

protected:
    mutable std::mutex m_mMutex;
};

} // namespace data
} // namespace ra

#endif RA_DATA_MODEL_PROPERTY_CONTAINER_H
