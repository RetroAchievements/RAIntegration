#include "ui\ViewModelCollection.hh"

#include "ra_fwd.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ra {
namespace ui {
namespace tests {

class TestViewModel : public ViewModelBase
{
public:
    TestViewModel() noexcept {}
    TestViewModel(int nInt, const std::wstring& sString) noexcept
    {
        SetInt(nInt);
        SetString(sString);
    }

    TestViewModel(const TestViewModel&) = delete;
    TestViewModel& operator=(const TestViewModel&) = delete;
    TestViewModel(TestViewModel&&)
        noexcept(std::is_nothrow_move_constructible_v<ViewModelBase>) = default;

    TestViewModel& operator=(TestViewModel&&) noexcept = default;


    static StringModelProperty StringProperty;
    const std::wstring& GetString() const { return GetValue(StringProperty); }
    void SetString(const std::wstring& sValue) { SetValue(StringProperty, sValue); }

    static IntModelProperty IntProperty;
    int GetInt() const { return GetValue(IntProperty); }
    void SetInt(int nValue) { SetValue(IntProperty, nValue); }

    static BoolModelProperty BoolProperty;
    bool GetBool() const { return GetValue(BoolProperty); }
    void SetBool(bool bValue) { SetValue(BoolProperty, bValue); }
};

StringModelProperty TestViewModel::StringProperty("ViewModelHarness", "String", L"");
IntModelProperty TestViewModel::IntProperty("ViewModelHarness", "Int", 0);
BoolModelProperty TestViewModel::BoolProperty("ViewModelHarness", "Bool", false);

TEST_CLASS(ViewModelCollection_Tests)
{
    class NotifyTargetHarness : public ViewModelCollectionBase::NotifyTarget
    {
    public:
        void AssertNotChanged()
        {
            Assert::AreEqual(std::string(), m_sLastPropertyChanged);

            m_sLastPropertyChanged.clear();
        }

        void OnViewModelBoolValueChanged(gsl::index nIndex, const BoolModelProperty::ChangeArgs& args) noexcept override
        {
            GSL_SUPPRESS_F6 m_sLastPropertyChanged = args.Property.GetPropertyName();

            m_nChangeIndex = nIndex;
            m_bOldValue = args.tOldValue;
            m_bNewValue = args.tNewValue;
        }

        void AssertBoolChanged(const BoolModelProperty& pProperty, gsl::index nIndex, bool bOldValue, bool bNewValue)
        {
            Assert::AreEqual(pProperty.GetPropertyName(), m_sLastPropertyChanged.c_str());
            Assert::AreEqual(nIndex, m_nChangeIndex);
            Assert::AreEqual(bOldValue, m_bOldValue);
            Assert::AreEqual(bNewValue, m_bNewValue);

            m_sLastPropertyChanged.clear();
        }
        
        void OnViewModelStringValueChanged(gsl::index nIndex, const StringModelProperty::ChangeArgs& args) noexcept override
        {
           GSL_SUPPRESS_F6 m_sLastPropertyChanged = args.Property.GetPropertyName();

           m_nChangeIndex = nIndex;
           GSL_SUPPRESS_F6 m_sOldValue = args.tOldValue;
           GSL_SUPPRESS_F6 m_sNewValue = args.tNewValue;
        }

        void AssertStringChanged(const StringModelProperty& pProperty, gsl::index nIndex, const std::wstring& sOldValue, const std::wstring& sNewValue)
        {
            Assert::AreEqual(pProperty.GetPropertyName(), m_sLastPropertyChanged.c_str());
            Assert::AreEqual(nIndex, m_nChangeIndex);
            Assert::AreEqual(sOldValue, m_sOldValue);
            Assert::AreEqual(sNewValue, m_sNewValue);

            m_sLastPropertyChanged.clear();
        }

        void OnViewModelIntValueChanged(gsl::index nIndex, const IntModelProperty::ChangeArgs& args) noexcept override
        {
            GSL_SUPPRESS_F6 m_sLastPropertyChanged = args.Property.GetPropertyName();

            m_nChangeIndex = nIndex;
            m_nOldValue = args.tOldValue;
            m_nNewValue = args.tNewValue;
        }

        void AssertIntChanged(const IntModelProperty& pProperty, gsl::index nIndex, int nOldValue, int nNewValue)
        {
            Assert::AreEqual(pProperty.GetPropertyName(), m_sLastPropertyChanged.c_str());
            Assert::AreEqual(nIndex, m_nChangeIndex);
            Assert::AreEqual(nOldValue, m_nOldValue);
            Assert::AreEqual(nNewValue, m_nNewValue);

            m_sLastPropertyChanged.clear();
        }

        void OnViewModelAdded(gsl::index nIndex) noexcept override
        {
            m_nChangeIndex = nIndex;
            m_sLastPropertyChanged = "~ADDED~";
        }

        void AssertItemAdded(gsl::index nIndex)
        {
            Assert::AreEqual(nIndex, m_nChangeIndex);
            Assert::AreEqual(std::string("~ADDED~"), m_sLastPropertyChanged);

            m_sLastPropertyChanged.clear();
        }

        void OnViewModelRemoved(gsl::index nIndex) noexcept override
        {
            m_nChangeIndex = nIndex;
            m_sLastPropertyChanged = "~REMOVED~";
        }

        void AssertItemRemoved(gsl::index nIndex)
        {
            Assert::AreEqual(nIndex, m_nChangeIndex);
            Assert::AreEqual(std::string("~REMOVED~"), m_sLastPropertyChanged);

            m_sLastPropertyChanged.clear();
        }

    private:
        std::string m_sLastPropertyChanged;
        std::wstring m_sOldValue, m_sNewValue;
        int m_nOldValue{}, m_nNewValue{};
        bool m_bOldValue{}, m_bNewValue{};
        gsl::index m_nChangeIndex = -1;
    };

public:
    TEST_METHOD(TestAddWithoutSubscription)
    {
        ViewModelCollection<TestViewModel> vmCollection;
        auto& pItem1 = vmCollection.Add();
        pItem1.SetInt(1);
        pItem1.SetString(L"Test1");
        Assert::AreEqual(1, pItem1.GetInt());
        Assert::AreEqual(std::wstring(L"Test1"), pItem1.GetString());

        auto& pItem2 = vmCollection.Add(2, L"Test2");
        Assert::AreEqual(2, pItem2.GetInt());
        Assert::AreEqual(std::wstring(L"Test2"), pItem2.GetString());

        Assert::AreEqual(2U, vmCollection.Count());

        auto* pItem1b = vmCollection.GetItemAt(0);
        Assert::IsTrue(pItem1b == &pItem1);

        const auto* pItem2b = vmCollection.GetItemAt(1);
        Assert::IsTrue(pItem2b == &pItem2);
    }

    TEST_METHOD(TestAddRemoveWithSubscription)
    {
        ViewModelCollection<TestViewModel> vmCollection;
        NotifyTargetHarness oNotify;
        vmCollection.AddNotifyTarget(oNotify);

        vmCollection.Add();
        oNotify.AssertItemAdded(0);

        vmCollection.Add(2, L"Test2");
        oNotify.AssertItemAdded(1);

        auto& pItem3 = vmCollection.Add(3, L"Test3");
        oNotify.AssertItemAdded(2);

        Assert::AreEqual(3U, vmCollection.Count());

        vmCollection.RemoveAt(1);
        oNotify.AssertItemRemoved(1);

        auto* pItem3b = vmCollection.GetItemAt(1);
        Assert::IsTrue(pItem3b == &pItem3);

        auto* pItem3c = vmCollection.GetItemAt(2);
        Assert::IsNull(pItem3c);
    }

    TEST_METHOD(TestGetItemValue)
    {
        ViewModelCollection<TestViewModel> vmCollection;
        auto& pItem1 = vmCollection.Add(1, L"Test1");
        pItem1.SetBool(false);

        auto& pItem2 = vmCollection.Add(2, L"Test2");
        pItem2.SetBool(true);

        Assert::AreEqual(1, vmCollection.GetItemValue(0, TestViewModel::IntProperty));
        Assert::AreEqual(std::wstring(L"Test1"), vmCollection.GetItemValue(0, TestViewModel::StringProperty));
        Assert::AreEqual(false, vmCollection.GetItemValue(0, TestViewModel::BoolProperty));
        Assert::AreEqual(2, vmCollection.GetItemValue(1, TestViewModel::IntProperty));
        Assert::AreEqual(std::wstring(L"Test2"), vmCollection.GetItemValue(1, TestViewModel::StringProperty));
        Assert::AreEqual(true, vmCollection.GetItemValue(1, TestViewModel::BoolProperty));

        vmCollection.RemoveAt(0);

        Assert::AreEqual(2, vmCollection.GetItemValue(0, TestViewModel::IntProperty));
        Assert::AreEqual(std::wstring(L"Test2"), vmCollection.GetItemValue(0, TestViewModel::StringProperty));
        Assert::AreEqual(true, vmCollection.GetItemValue(0, TestViewModel::BoolProperty));

        // invalid index, return defaults
        Assert::AreEqual(TestViewModel::IntProperty.GetDefaultValue(), vmCollection.GetItemValue(1, TestViewModel::IntProperty));
        Assert::AreEqual(TestViewModel::StringProperty.GetDefaultValue(), vmCollection.GetItemValue(1, TestViewModel::StringProperty));
        Assert::AreEqual(TestViewModel::BoolProperty.GetDefaultValue(), vmCollection.GetItemValue(1, TestViewModel::BoolProperty));
    }

    TEST_METHOD(TestStringPropertyNotification)
    {
        ViewModelCollection<TestViewModel> vmCollection;
        auto& pItem1 = vmCollection.Add(1, L"Test1");
        auto& pItem2 = vmCollection.Add(2, L"Test2");

        NotifyTargetHarness oNotify;
        vmCollection.AddNotifyTarget(oNotify);

        pItem1.SetString(L"Test1a");
        oNotify.AssertStringChanged(TestViewModel::StringProperty, 0, L"Test1", L"Test1a");

        pItem2.SetString(L"Test2a");
        oNotify.AssertStringChanged(TestViewModel::StringProperty, 1, L"Test2", L"Test2a");

        vmCollection.RemoveNotifyTarget(oNotify);

        pItem1.SetString(L"Test1a");
        oNotify.AssertNotChanged();

        pItem2.SetString(L"Test2a");
        oNotify.AssertNotChanged();
    }

    TEST_METHOD(TestIntPropertyNotification)
    {
        ViewModelCollection<TestViewModel> vmCollection;
        auto& pItem1 = vmCollection.Add(1, L"Test1");
        auto& pItem2 = vmCollection.Add(2, L"Test2");

        NotifyTargetHarness oNotify;
        vmCollection.AddNotifyTarget(oNotify);

        pItem1.SetInt(4);
        oNotify.AssertIntChanged(TestViewModel::IntProperty, 0, 1, 4);

        pItem2.SetInt(5);
        oNotify.AssertIntChanged(TestViewModel::IntProperty, 1, 2, 5);

        vmCollection.RemoveNotifyTarget(oNotify);

        pItem1.SetInt(6);
        oNotify.AssertNotChanged();

        pItem2.SetInt(7);
        oNotify.AssertNotChanged();
    }

    TEST_METHOD(TestBoolPropertyNotification)
    {
        ViewModelCollection<TestViewModel> vmCollection;
        auto& pItem1 = vmCollection.Add(1, L"Test1");
        auto& pItem2 = vmCollection.Add(2, L"Test2");

        NotifyTargetHarness oNotify;
        vmCollection.AddNotifyTarget(oNotify);

        pItem1.SetBool(true);
        oNotify.AssertBoolChanged(TestViewModel::BoolProperty, 0, false, true);

        pItem2.SetBool(true);
        oNotify.AssertBoolChanged(TestViewModel::BoolProperty, 1, false, true);

        vmCollection.RemoveNotifyTarget(oNotify);

        pItem1.SetBool(false);
        oNotify.AssertNotChanged();

        pItem2.SetBool(false);
        oNotify.AssertNotChanged();
    }

    TEST_METHOD(TestFreeze)
    {
        ViewModelCollection<TestViewModel> vmCollection;
        auto& pItem1 = vmCollection.Add(1, L"Test1");

        NotifyTargetHarness oNotify;
        vmCollection.AddNotifyTarget(oNotify);

        pItem1.SetString(L"Test1a");
        oNotify.AssertStringChanged(TestViewModel::StringProperty, 0, L"Test1", L"Test1a");
        pItem1.SetInt(4);
        oNotify.AssertIntChanged(TestViewModel::IntProperty, 0, 1, 4);
        pItem1.SetBool(true);
        oNotify.AssertBoolChanged(TestViewModel::BoolProperty, 0, false, true);

        Assert::IsFalse(vmCollection.IsFrozen());
        vmCollection.Freeze();
        Assert::IsTrue(vmCollection.IsFrozen());

        // should not watch/notify changes after being frozen
        pItem1.SetString(L"Test1b");
        oNotify.AssertNotChanged();
        pItem1.SetInt(6);
        oNotify.AssertNotChanged();
        pItem1.SetBool(false);
        oNotify.AssertNotChanged();

        // collection cannot be modified after being frozen - if it could, we'd expect no notifications from new item
        // auto& pItem2 = vmCollection.Add(2, L"Test2");
    }

    TEST_METHOD(TestAddRemovePropertyNotification)
    {
        ViewModelCollection<TestViewModel> vmCollection;
        NotifyTargetHarness oNotify;
        vmCollection.AddNotifyTarget(oNotify);

        auto& pItem1 = vmCollection.Add(1, L"Test1");
        oNotify.AssertItemAdded(0);

        auto& pItem2 = vmCollection.Add(2, L"Test2");
        oNotify.AssertItemAdded(1);

        pItem1.SetString(L"Test1a");
        oNotify.AssertStringChanged(TestViewModel::StringProperty, 0, L"Test1", L"Test1a");
        pItem1.SetInt(4);
        oNotify.AssertIntChanged(TestViewModel::IntProperty, 0, 1, 4);
        pItem1.SetBool(true);
        oNotify.AssertBoolChanged(TestViewModel::BoolProperty, 0, false, true);

        vmCollection.RemoveAt(0);
        oNotify.AssertItemRemoved(0);

        // pItem1 is invalid now.

        pItem2.SetString(L"Test2a");
        oNotify.AssertStringChanged(TestViewModel::StringProperty, 0, L"Test2", L"Test2a");
        pItem2.SetInt(5);
        oNotify.AssertIntChanged(TestViewModel::IntProperty, 0, 2, 5);
        pItem2.SetBool(true);
        oNotify.AssertBoolChanged(TestViewModel::BoolProperty, 0, false, true);
    }
};

} // namespace tests
} // namespace ui
} // namespace ra
