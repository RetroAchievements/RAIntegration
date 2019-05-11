#include "ui\ViewModelBase.hh"

#include "ra_fwd.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ra {
namespace ui {
namespace tests {

TEST_CLASS(ViewModelBase_Tests)
{
    class ViewModelHarness : public ViewModelBase
    {
    public:
        ViewModelHarness() noexcept(std::is_nothrow_default_constructible_v<ViewModelBase>) = default;

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

    class NotifyTargetHarness : public ViewModelBase::NotifyTarget
    {
    public:
        void AssertNotChanged()
        {
            Assert::AreEqual(std::string(), m_sLastPropertyChanged);

            m_sLastPropertyChanged.clear();
        }

        void OnViewModelBoolValueChanged(const BoolModelProperty::ChangeArgs& args) noexcept override
        {
            GSL_SUPPRESS_F6 m_sLastPropertyChanged = args.Property.GetPropertyName();

            m_bOldValue = args.tOldValue;
            m_bNewValue = args.tNewValue;
        }

        void AssertBoolChanged(const BoolModelProperty& pProperty, bool bOldValue, bool bNewValue)
        {
            Assert::AreEqual(pProperty.GetPropertyName(), m_sLastPropertyChanged.c_str());
            Assert::AreEqual(bOldValue, m_bOldValue);
            Assert::AreEqual(bNewValue, m_bNewValue);

            m_sLastPropertyChanged.clear();
        }
        
        void OnViewModelStringValueChanged(const StringModelProperty::ChangeArgs& args) noexcept override
        {
           GSL_SUPPRESS_F6 m_sLastPropertyChanged = args.Property.GetPropertyName();

           GSL_SUPPRESS_F6 m_sOldValue = args.tOldValue;
           GSL_SUPPRESS_F6 m_sNewValue = args.tNewValue;
        }

        void AssertStringChanged(const StringModelProperty& pProperty, const std::wstring& sOldValue, const std::wstring& sNewValue)
        {
            Assert::AreEqual(pProperty.GetPropertyName(), m_sLastPropertyChanged.c_str());
            Assert::AreEqual(sOldValue, m_sOldValue);
            Assert::AreEqual(sNewValue, m_sNewValue);

            m_sLastPropertyChanged.clear();
        }

        void OnViewModelIntValueChanged(const IntModelProperty::ChangeArgs& args) noexcept override
        {
            GSL_SUPPRESS_F6 m_sLastPropertyChanged = args.Property.GetPropertyName();

            m_nOldValue = args.tOldValue;
            m_nNewValue = args.tNewValue;
        }

        void AssertIntChanged(const IntModelProperty& pProperty, int nOldValue, int nNewValue)
        {
            Assert::AreEqual(pProperty.GetPropertyName(), m_sLastPropertyChanged.c_str());
            Assert::AreEqual(nOldValue, m_nOldValue);
            Assert::AreEqual(nNewValue, m_nNewValue);

            m_sLastPropertyChanged.clear();
        }

    private:
        std::string m_sLastPropertyChanged;
        std::wstring m_sOldValue, m_sNewValue;
        int m_nOldValue{}, m_nNewValue{};
        bool m_bOldValue{}, m_bNewValue{};
    };

public:
    TEST_METHOD(TestStringProperty)
    {
        ViewModelHarness vmViewModel;
        Assert::AreEqual(std::wstring(), vmViewModel.GetString());

        vmViewModel.SetString(L"Test");
        Assert::AreEqual(std::wstring(L"Test"), vmViewModel.GetString());

        NotifyTargetHarness oNotify;
        vmViewModel.AddNotifyTarget(oNotify);
        oNotify.AssertNotChanged();

        vmViewModel.SetString(L"Test2");
        oNotify.AssertStringChanged(vmViewModel.StringProperty, L"Test", L"Test2");

        vmViewModel.SetString(L"Test2");
        oNotify.AssertNotChanged();

        vmViewModel.SetString(vmViewModel.StringProperty.GetDefaultValue());
        oNotify.AssertStringChanged(vmViewModel.StringProperty, L"Test2", L"");

        vmViewModel.RemoveNotifyTarget(oNotify);
        vmViewModel.SetString(L"Test3");
        oNotify.AssertNotChanged();
    }

    TEST_METHOD(TestIntProperty)
    {
        ViewModelHarness vmViewModel;
        Assert::AreEqual(0, vmViewModel.GetInt());

        vmViewModel.SetInt(32);
        Assert::AreEqual(32, vmViewModel.GetInt());

        NotifyTargetHarness oNotify;
        vmViewModel.AddNotifyTarget(oNotify);
        oNotify.AssertNotChanged();

        vmViewModel.SetInt(64);
        oNotify.AssertIntChanged(vmViewModel.IntProperty, 32, 64);

        vmViewModel.SetInt(64);
        oNotify.AssertNotChanged();

        vmViewModel.SetInt(vmViewModel.IntProperty.GetDefaultValue());
        oNotify.AssertIntChanged(vmViewModel.IntProperty, 64, 0);

        vmViewModel.RemoveNotifyTarget(oNotify);
        vmViewModel.SetInt(96);
        oNotify.AssertNotChanged();
    }

    TEST_METHOD(TestBoolProperty)
    {
        ViewModelHarness vmViewModel;
        Assert::AreEqual(false, vmViewModel.GetBool());

        vmViewModel.SetBool(true);
        Assert::AreEqual(true, vmViewModel.GetBool());

        NotifyTargetHarness oNotify;
        vmViewModel.AddNotifyTarget(oNotify);
        oNotify.AssertNotChanged();

        vmViewModel.SetBool(false);
        oNotify.AssertBoolChanged(vmViewModel.BoolProperty, true, false);

        vmViewModel.SetBool(false);
        oNotify.AssertNotChanged();

        vmViewModel.RemoveNotifyTarget(oNotify);
        vmViewModel.SetBool(true);
        oNotify.AssertNotChanged();
    }
};

} // namespace tests
} // namespace ui
} // namespace ra
