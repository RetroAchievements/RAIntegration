#ifndef RA_UI_MODEL_PROPERTY_H
#define RA_UI_MODEL_PROPERTY_H
#pragma once

#include "ra_fwd.h"

namespace ra {
namespace ui {

class ModelPropertyBase
{
public:
    GSL_SUPPRESS_F6 virtual ~ModelPropertyBase() noexcept;
    ModelPropertyBase(const ModelPropertyBase&) noexcept = delete;
    ModelPropertyBase& operator=(const ModelPropertyBase&) noexcept = delete;
    ModelPropertyBase(ModelPropertyBase&&) noexcept = delete;
    ModelPropertyBase& operator=(ModelPropertyBase&&) noexcept = delete;

    /// <summary>
    /// Gets the unique identifier of the property.
    /// </summary>
    int GetKey() const noexcept { return m_nKey; }

    /// <summary>
    /// Gets the name of the type that owns the property.
    /// </summary>
    const char* GetTypeName() const noexcept { return m_sTypeName; }

    /// <summary>
    /// Gets the name of the property.
    /// </summary>
    const char* GetPropertyName() const noexcept { return m_sPropertyName; }

    /// <summary>
    /// Gets the property for specified unique identifier.
    /// </summary>
    /// <returns>Associated property, <c>nullptr</c> if not found.</returns>
    static const ModelPropertyBase* GetPropertyForKey(int nKey);

    _NODISCARD inline constexpr auto operator==(_In_ const ModelPropertyBase& that) const noexcept
    {
        return m_nKey == that.m_nKey;
    }

    _NODISCARD inline constexpr auto operator!=(_In_ const ModelPropertyBase& that) const noexcept
    {
        return m_nKey != that.m_nKey;
    }

    _NODISCARD inline constexpr auto // for sorting if needed
        operator<(_In_ const ModelPropertyBase& that) const noexcept
    {
        return m_nKey < that.m_nKey;
    }

protected:
    explicit ModelPropertyBase(_In_ const char* const sTypeName, _In_ const char* const sPropertyName);

    explicit ModelPropertyBase(_In_ int nKey) noexcept { m_nKey = nKey; } // for binary search

private:
    int m_nKey{};
    const char* m_sTypeName{};
    const char* m_sPropertyName{};

    static std::vector<ModelPropertyBase*> s_vProperties;
    static int s_nNextKey;
};

template<class T>
class ModelProperty : public ModelPropertyBase
{
public:
    explicit ModelProperty(const char* sTypeName, const char* sPropertyName, T tDefaultValue) :
        ModelPropertyBase(sTypeName, sPropertyName),
        m_tDefaultValue(tDefaultValue)
    {}

    /// <summary>
    /// Gets the default value for the property.
    /// </summary>
    const T& GetDefaultValue() const noexcept { return m_tDefaultValue; }

    /// <summary>
    /// Sets the default value for the property.
    /// </summary>
    /// <remarks>Affects all existing instances where a value has not been set, not just new instances.</remarks>
    void SetDefaultValue(const T& tValue) noexcept(std::is_nothrow_copy_assignable_v<T>) { m_tDefaultValue = tValue; }

    using ValueMap = std::unordered_map<int, T>;

    struct ChangeArgs
    {
        /// <summary>
        /// The property that changed.
        /// </summary>
        const ModelProperty& Property;

        /// <summary>
        /// The old value.
        /// </summary>
        const T& tOldValue;

        /// <summary>
        /// The new value.
        /// </summary>
        const T& tNewValue;
    };

private:
    T m_tDefaultValue;
};

using BoolModelProperty = ModelProperty<bool>;
using StringModelProperty = ModelProperty<std::wstring>;
using IntModelProperty = ModelProperty<int>;

} // namespace ui
} // namespace ra

#endif RA_UI_MODEL_PROPERTY_H
