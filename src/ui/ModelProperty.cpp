#include "ModelProperty.hh"

#include <algorithm>

namespace ra {
namespace ui {

int ModelPropertyBase::s_nNextKey = 1;
std::vector<ModelPropertyBase*> ModelPropertyBase::s_vProperties;

ModelPropertyBase::ModelPropertyBase(const char* sTypeName, const char* sPropertyName) noexcept
{
    m_sTypeName = sTypeName;
    m_sPropertyName = sPropertyName;

    // ASSERT: ModelProperties are static variables and constructed when the DLL is loaded.
    // We cannot reasonably use a mutex because it cannot be locked during static initialization in WinXP.
    m_nKey = s_nNextKey++;
    s_vProperties.push_back(this);
}

ModelPropertyBase::~ModelPropertyBase() noexcept
{
    auto iter = std::find(s_vProperties.begin(), s_vProperties.end(), this);
    if (iter != s_vProperties.end())
        s_vProperties.erase(iter);
}

const ModelPropertyBase* ModelPropertyBase::GetPropertyForKey(int nKey)
{
    ModelPropertyBase search(nKey);
    auto iter = std::lower_bound(s_vProperties.begin(), s_vProperties.end(), &search, [](const ModelPropertyBase* left, const ModelPropertyBase* right)
    {
        return left->GetKey() < right->GetKey();
    });

    return (iter == s_vProperties.end() ? nullptr : *iter);
}

} // namespace ui
} // namespace ra
