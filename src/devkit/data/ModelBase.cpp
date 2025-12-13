#include "ModelBase.hh"

#include "ModelCollectionBase.hh"

namespace ra {
namespace data {

void ModelBase::OnValueChanged(const BoolModelProperty::ChangeArgs& args)
{
    if (m_pCollection)
        m_pCollection->NotifyModelValueChanged(m_nCollectionIndex, args);

    ModelPropertyContainer::OnValueChanged(args);
}

void ModelBase::OnValueChanged(const StringModelProperty::ChangeArgs& args)
{
    if (m_pCollection)
        m_pCollection->NotifyModelValueChanged(m_nCollectionIndex, args);

    ModelPropertyContainer::OnValueChanged(args);
}

void ModelBase::OnValueChanged(const IntModelProperty::ChangeArgs& args)
{
    if (m_pCollection)
        m_pCollection->NotifyModelValueChanged(m_nCollectionIndex, args);

    ModelPropertyContainer::OnValueChanged(args);
}

} // namespace ui
} // namespace ra
