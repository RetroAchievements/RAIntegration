#ifndef RA_DATA_MODEL_PROPERTY_CONTAINER_H
#define RA_DATA_MODEL_PROPERTY_CONTAINER_H
#pragma once

#include "ra_fwd.h"

#include "ModelProperty.hh"

namespace ra {
namespace data {

class ModelPropertyContainer
{
public:
    GSL_SUPPRESS_F6 ModelPropertyContainer() = default;

#ifdef RA_UTEST
    virtual ~ModelPropertyContainer() noexcept {
        m_bDestructed = true;
    }

    bool m_bDestructed = false;
#else
    virtual ~ModelPropertyContainer() noexcept = default;
#endif

    ModelPropertyContainer(const ModelPropertyContainer&) = delete;
    ModelPropertyContainer& operator=(const ModelPropertyContainer&) = delete;
    GSL_SUPPRESS_F6 ModelPropertyContainer(ModelPropertyContainer&&) = default;
    GSL_SUPPRESS_F6 ModelPropertyContainer& operator=(ModelPropertyContainer&&) = default;

    /// <summary>
    /// Gets the value associated to the requested boolean property.
    /// </summary>
    /// <param name="pProperty">The property to query.</param>
    /// <returns>The current value of the property for this object.</returns>
    bool GetValue(const BoolModelProperty& pProperty) const
    {
        const IntModelProperty::ValueMap::const_iterator iter = m_mIntValues.find(pProperty.GetKey());
        return gsl::narrow_cast<bool>(iter != m_mIntValues.end() ? iter->second : pProperty.GetDefaultValue());
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
        const StringModelProperty::ValueMap::const_iterator iter = m_mStringValues.find(pProperty.GetKey());
        return (iter != m_mStringValues.end() ? iter->second : pProperty.GetDefaultValue());
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
        const IntModelProperty::ValueMap::const_iterator iter = m_mIntValues.find(pProperty.GetKey());
        return (iter != m_mIntValues.end() ? iter->second : pProperty.GetDefaultValue());
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
    StringModelProperty::ValueMap m_mStringValues;
    IntModelProperty::ValueMap m_mIntValues;

#ifdef _DEBUG
    /// <summary>
    /// Complete list of values as strings for viewing in the debugger
    /// </summary>
    std::map<std::string, std::wstring> m_mDebugValues;
#endif
};

} // namespace data
} // namespace ra

#endif RA_DATA_MODEL_PROPERTY_CONTAINER_H
