#ifndef RA_UI_MODEL_PROPERTY_H
#define RA_UI_MODEL_PROPERTY_H
#pragma once

#ifndef PCH_H
#include <map>
#include <mutex> // thread(memory)
#include <set>
#include <string>
#endif /* !PCH_H */

namespace ra {
namespace ui {

class ModelPropertyBase
{
public:
    /// <summary>
    /// Gets the unique identifier of the property.
    /// </summary>
    int GetKey() const { return m_nKey; }

    /// <summary>
    /// Gets the name of the type that owns the property.
    /// </summary>
    const char* GetTypeName() const { return m_sTypeName; }

    /// <summary>
    /// Gets the name of the property.
    /// </summary>
    const char* GetPropertyName() const { return m_sPropertyName; }

    /// <summary>
    /// Gets the property for specified unique identifier.
    /// </summary>
    /// <returns>Associated property, <c>null</c> if not found.</returns>
    static const ModelPropertyBase* GetPropertyForKey(int nKey);

    bool operator==(const ModelPropertyBase& that) const { return m_nKey == that.m_nKey; }
    bool operator!=(const ModelPropertyBase& that) const { return m_nKey != that.m_nKey; }

protected:
    explicit ModelPropertyBase(const char* sTypeName, const char* sPropertyName) noexcept;
    virtual ~ModelPropertyBase() noexcept;

    explicit ModelPropertyBase(int nKey) noexcept { m_nKey = nKey; } // for binary search

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
    explicit ModelProperty(const char* sTypeName, const char* sPropertyName, T tDefaultValue) noexcept
        : ModelPropertyBase(sTypeName, sPropertyName), m_tDefaultValue(tDefaultValue)
    {
    }

    /// <summary>
    /// Gets the default value for the property.
    /// </summary>
    const T& GetDefaultValue() const { return m_tDefaultValue; }

    /// <summary>
    /// Sets the default value for the property.
    /// </summary>
    /// <remarks>Affects all existing instances where a value has not been set, not just new instances.</remarks>
    void SetDefaultValue(const T& tValue) { m_tDefaultValue = tValue; }

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
