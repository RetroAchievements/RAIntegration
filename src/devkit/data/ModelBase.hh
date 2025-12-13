#ifndef RA_DATA_MODEL_BASE_H
#define RA_DATA_MODEL_BASE_H
#pragma once

#include "ModelPropertyContainer.hh"

namespace ra {
namespace data {

class ModelCollectionBase;

class ModelBase : protected ModelPropertyContainer
{
protected:
    GSL_SUPPRESS_F6 ModelBase() = default;

    void OnValueChanged(const BoolModelProperty::ChangeArgs& args) override;
    void OnValueChanged(const StringModelProperty::ChangeArgs& args) override;
    void OnValueChanged(const IntModelProperty::ChangeArgs& args) override;

private:
    // allow ModelCollectionBase to call GetValue and SetValue directly, as well as manage the m_pCollection fields
    friend class ModelCollectionBase;

    ModelCollectionBase* m_pCollection = nullptr;
    gsl::index m_nCollectionIndex = 0;
};

} // namespace data
} // namespace ra

#endif RA_DATA_MODEL_BASE_H
