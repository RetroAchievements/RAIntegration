#include "data\ModelPropertyContainer.hh"

#include "ra_fwd.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ra {
namespace data {
namespace tests {

TEST_CLASS(ModelPropertyContainer_Tests)
{
    class ModelPropertyContainerHarness : public ModelPropertyContainer
    {
    public:
        StringModelProperty StringProperty{ "ViewModelHarness", "String", L"" };
        const std::wstring& GetString() const { return GetValue(StringProperty); }
        void SetString(const std::wstring& sValue) { SetValue(StringProperty, sValue); }

        IntModelProperty IntProperty{ "ViewModelHarness", "Int", 0 };
        int GetInt() const { return GetValue(IntProperty); }
        void SetInt(int nValue) { SetValue(IntProperty, nValue); }

        BoolModelProperty BoolProperty{ "ViewModelHarness", "Bool", false };
        bool GetBool() const { return GetValue(BoolProperty); }
        void SetBool(bool bValue) { SetValue(BoolProperty, bValue); }
    };

public:
    TEST_METHOD(TestStringProperty)
    {
        ModelPropertyContainerHarness vmViewModel;
        Assert::AreEqual(std::wstring(), vmViewModel.GetString());

        vmViewModel.SetString(L"Test");
        Assert::AreEqual(std::wstring(L"Test"), vmViewModel.GetString());
    }

    TEST_METHOD(TestIntProperty)
    {
        ModelPropertyContainerHarness vmViewModel;
        Assert::AreEqual(0, vmViewModel.GetInt());

        vmViewModel.SetInt(32);
        Assert::AreEqual(32, vmViewModel.GetInt());
    }

    TEST_METHOD(TestBoolProperty)
    {
        ModelPropertyContainerHarness vmViewModel;
        Assert::AreEqual(false, vmViewModel.GetBool());

        vmViewModel.SetBool(true);
        Assert::AreEqual(true, vmViewModel.GetBool());
    }
};

} // namespace tests
} // namespace ui
} // namespace ra
