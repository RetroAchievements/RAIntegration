#include "ui\ViewModelCollection.hh"

#include "ra_fwd.h"

#include "..\RA_UnitTestHelpers.h"

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
        GSL_SUPPRESS_F6 SetInt(nInt);
        GSL_SUPPRESS_F6 SetString(sString);
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

        void OnViewModelAdded(gsl::index nIndex) override
        {
            m_nChanges[nIndex] += "~ADDED~";
        }

        void AssertItemAdded(gsl::index nIndex)
        {
            Assert::AreEqual(std::string("~ADDED~"), m_nChanges[nIndex]);
        }

        void OnViewModelRemoved(gsl::index nIndex) override
        {
            m_nChanges[nIndex] += "~REMOVED~";
        }

        void AssertItemRemoved(gsl::index nIndex)
        {
            Assert::AreEqual(std::string("~REMOVED~"), m_nChanges[nIndex]);
        }

        void AssertItemReplaced(gsl::index nIndex)
        {
            Assert::AreEqual(std::string("~REMOVED~~ADDED~"), m_nChanges[nIndex]);
        }

        void OnViewModelChanged(gsl::index nIndex) override
        {
            m_nChanges[nIndex] += "~CHANGED~";
        }

        void AssertItemChanged(gsl::index nIndex)
        {
            Assert::AreEqual(std::string("~CHANGED~"), m_nChanges[nIndex]);
        }

        void AssertItemNotChanged(gsl::index nIndex)
        {
            Assert::IsTrue(m_nChanges.find(nIndex) == m_nChanges.end());
        }

        void ResetChanges() noexcept
        {
            m_nChanges.clear();
        }

    private:
        std::string m_sLastPropertyChanged;
        std::wstring m_sOldValue, m_sNewValue;
        int m_nOldValue{}, m_nNewValue{};
        bool m_bOldValue{}, m_bNewValue{};
        gsl::index m_nChangeIndex = 0;
        std::map<gsl::index, std::string> m_nChanges;
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

        Assert::AreEqual({ 2U }, vmCollection.Count());

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
        oNotify.ResetChanges();

        vmCollection.Add(2, L"Test2");
        oNotify.AssertItemAdded(1);
        oNotify.ResetChanges();

        auto& pItem3 = vmCollection.Add(3, L"Test3");
        oNotify.AssertItemAdded(2);
        oNotify.ResetChanges();

        Assert::AreEqual({ 3U }, vmCollection.Count());

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

    TEST_METHOD(TestGetViewModelAt)
    {
        ViewModelCollection<TestViewModel> vmCollection;
        auto& pItem1 = vmCollection.Add(1, L"Test1");
        pItem1.SetBool(false);

        auto& pItem2 = vmCollection.Add(2, L"Test2");
        pItem2.SetBool(true);

        const auto* vm1 = vmCollection.GetViewModelAt(0);
        Assert::IsTrue(vm1 == &pItem1);

        const auto* vm2 = vmCollection.GetViewModelAt(1);
        Assert::IsTrue(vm2 == &pItem2);

        const auto* vm3 = vmCollection.GetViewModelAt(2);
        Assert::IsNull(vm3);

        const auto* vm0 = vmCollection.GetViewModelAt(-1);
        Assert::IsNull(vm0);
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
        oNotify.ResetChanges();

        auto& pItem2 = vmCollection.Add(2, L"Test2");
        oNotify.AssertItemAdded(1);
        oNotify.ResetChanges();

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

    TEST_METHOD(TestAddWhileUpdateSuspendedPropertyNotification)
    {
        ViewModelCollection<TestViewModel> vmCollection;
        NotifyTargetHarness oNotify;
        vmCollection.AddNotifyTarget(oNotify);
        vmCollection.BeginUpdate();

        vmCollection.Add(1, L"Test1");
        oNotify.AssertItemNotChanged(0);

        vmCollection.Add(2, L"Test2");
        oNotify.AssertItemNotChanged(0);
        oNotify.AssertItemNotChanged(1);

        vmCollection.EndUpdate();
        oNotify.AssertItemAdded(0);
        oNotify.AssertItemAdded(1);
    }

    TEST_METHOD(TestRemoveWhileUpdateSuspendedPropertyNotification)
    {
        ViewModelCollection<TestViewModel> vmCollection;
        vmCollection.Add(1, L"Test1");
        vmCollection.Add(2, L"Test2");

        NotifyTargetHarness oNotify;
        vmCollection.AddNotifyTarget(oNotify);
        vmCollection.BeginUpdate();

        vmCollection.RemoveAt(0);
        oNotify.AssertItemNotChanged(0);

        vmCollection.RemoveAt(0);
        oNotify.AssertItemNotChanged(0);
        oNotify.AssertItemNotChanged(1);

        vmCollection.EndUpdate();
        oNotify.AssertItemRemoved(0);
        oNotify.AssertItemRemoved(1);
    }

    TEST_METHOD(TestClearWhileUpdateSuspendedPropertyNotification)
    {
        ViewModelCollection<TestViewModel> vmCollection;
        vmCollection.Add(1, L"Test1");
        vmCollection.Add(2, L"Test2");

        NotifyTargetHarness oNotify;
        vmCollection.AddNotifyTarget(oNotify);
        vmCollection.BeginUpdate();

        vmCollection.Clear();
        oNotify.AssertItemNotChanged(0);
        oNotify.AssertItemNotChanged(1);

        vmCollection.EndUpdate();
        oNotify.AssertItemRemoved(0);
        oNotify.AssertItemRemoved(1);

        Assert::AreEqual({ 0U }, vmCollection.Count());
    }

    TEST_METHOD(TestReplaceWhileUpdateSuspendedPropertyNotification)
    {
        ViewModelCollection<TestViewModel> vmCollection;
        NotifyTargetHarness oNotify;
        vmCollection.AddNotifyTarget(oNotify);

        vmCollection.Add(1, L"Test1");
        oNotify.AssertItemAdded(0);
        oNotify.ResetChanges();

        vmCollection.BeginUpdate();

        vmCollection.RemoveAt(0);
        oNotify.AssertItemNotChanged(0);

        vmCollection.Add(2, L"Test2");
        oNotify.AssertItemNotChanged(0);
        oNotify.AssertItemNotChanged(1);

        vmCollection.EndUpdate();

        oNotify.AssertItemReplaced(0);
    }

    TEST_METHOD(TestFindItemIndexAfterRemovedWhileUpdateSuspended)
    {
        ViewModelCollection<TestViewModel> vmCollection;
        vmCollection.Add(1, L"Test1");
        vmCollection.Add(2, L"Test2");
        vmCollection.Add(3, L"Test3");
        vmCollection.Add(4, L"Test4");

        vmCollection.BeginUpdate();

        Assert::AreEqual({ 0 }, vmCollection.FindItemIndex(TestViewModel::IntProperty, 1));
        Assert::AreEqual({ 1 }, vmCollection.FindItemIndex(TestViewModel::IntProperty, 2));
        Assert::AreEqual({ 2 }, vmCollection.FindItemIndex(TestViewModel::IntProperty, 3));
        Assert::AreEqual({ 3 }, vmCollection.FindItemIndex(TestViewModel::IntProperty, 4));

        vmCollection.RemoveAt(1);

        Assert::AreEqual({ 0 }, vmCollection.FindItemIndex(TestViewModel::IntProperty, 1));
        Assert::AreEqual({ -1 }, vmCollection.FindItemIndex(TestViewModel::IntProperty, 2));
        Assert::AreEqual({ 1 }, vmCollection.FindItemIndex(TestViewModel::IntProperty, 3));
        Assert::AreEqual({ 2 }, vmCollection.FindItemIndex(TestViewModel::IntProperty, 4));

        vmCollection.EndUpdate();

        Assert::AreEqual({ 0 }, vmCollection.FindItemIndex(TestViewModel::IntProperty, 1));
        Assert::AreEqual({ -1 }, vmCollection.FindItemIndex(TestViewModel::IntProperty, 2));
        Assert::AreEqual({ 1 }, vmCollection.FindItemIndex(TestViewModel::IntProperty, 3));
        Assert::AreEqual({ 2 }, vmCollection.FindItemIndex(TestViewModel::IntProperty, 4));
    }

    TEST_METHOD(TestGetItemWhileUpdateSuspended)
    {
        ViewModelCollection<TestViewModel> vmCollection;
        vmCollection.Add(1, L"Test1");

        vmCollection.BeginUpdate();

        Assert::AreEqual({ 1 }, vmCollection.Count());
        Assert::AreEqual(1, vmCollection.GetItemAt(0)->GetInt());

        vmCollection.Add(2, L"Test2");
        Assert::AreEqual({ 2 }, vmCollection.Count());
        Assert::AreEqual(1, vmCollection.GetItemAt(0)->GetInt());
        Assert::AreEqual(2, vmCollection.GetItemAt(1)->GetInt());

        vmCollection.RemoveAt(0);
        Assert::AreEqual({ 1 }, vmCollection.Count());
        Assert::AreEqual(2, vmCollection.GetItemAt(0)->GetInt());
        Assert::IsNull(vmCollection.GetItemAt(1));

        vmCollection.Add(3, L"Test3");
        Assert::AreEqual({ 2 }, vmCollection.Count());
        Assert::AreEqual(2, vmCollection.GetItemAt(0)->GetInt());
        Assert::AreEqual(3, vmCollection.GetItemAt(1)->GetInt());

        vmCollection.EndUpdate();
        Assert::AreEqual({ 2 }, vmCollection.Count());
        Assert::AreEqual(2, vmCollection.GetItemAt(0)->GetInt());
        Assert::AreEqual(3, vmCollection.GetItemAt(1)->GetInt());
    }

    TEST_METHOD(TestMovePropertyNotification)
    {
        ViewModelCollection<TestViewModel> vmCollection;
        auto& pItem1 = vmCollection.Add(1, L"Test1");
        auto& pItem2 = vmCollection.Add(2, L"Test2");
        auto& pItem3 = vmCollection.Add(3, L"Test3");
        auto& pItem4 = vmCollection.Add(4, L"Test4");
        auto& pItem5 = vmCollection.Add(5, L"Test5");

        NotifyTargetHarness oNotify;
        vmCollection.AddNotifyTarget(oNotify);

        // move item in slot 1 into slot 3, moving slots 2 and 3 back one to accommodate
        vmCollection.MoveItem(1, 3);

        oNotify.AssertItemNotChanged(0);
        oNotify.AssertItemChanged(1);
        oNotify.AssertItemChanged(2);
        oNotify.AssertItemChanged(3);
        oNotify.AssertItemNotChanged(4);

        Assert::AreEqual((void*)&pItem1, (void*)vmCollection.GetItemAt(0));
        Assert::AreEqual((void*)&pItem3, (void*)vmCollection.GetItemAt(1));
        Assert::AreEqual((void*)&pItem4, (void*)vmCollection.GetItemAt(2));
        Assert::AreEqual((void*)&pItem2, (void*)vmCollection.GetItemAt(3));
        Assert::AreEqual((void*)&pItem5, (void*)vmCollection.GetItemAt(4));
    }

    TEST_METHOD(TestMoveWhileUpdateSuspendedPropertyNotification)
    {
        ViewModelCollection<TestViewModel> vmCollection;
        auto& pItem1 = vmCollection.Add(1, L"Test1");
        auto& pItem2 = vmCollection.Add(2, L"Test2");
        auto& pItem3 = vmCollection.Add(3, L"Test3");
        auto& pItem4 = vmCollection.Add(4, L"Test4");
        auto& pItem5 = vmCollection.Add(5, L"Test5");

        NotifyTargetHarness oNotify;
        vmCollection.AddNotifyTarget(oNotify);

        vmCollection.BeginUpdate();

        // move item in slot 1 into slot 3, moving slots 2 and 3 back one to accommodate
        vmCollection.MoveItem(1, 3);

        oNotify.AssertItemNotChanged(0);
        oNotify.AssertItemNotChanged(1);
        oNotify.AssertItemNotChanged(2);
        oNotify.AssertItemNotChanged(3);
        oNotify.AssertItemNotChanged(4);

        // move item in slot 3 into slot 2, moving slot 2 forward one to accommodate
        vmCollection.MoveItem(3, 2);

        oNotify.AssertItemNotChanged(0);
        oNotify.AssertItemNotChanged(1);
        oNotify.AssertItemNotChanged(2);
        oNotify.AssertItemNotChanged(3);
        oNotify.AssertItemNotChanged(4);

        // EndUpdate should trigger notifications
        vmCollection.EndUpdate();

        oNotify.AssertItemNotChanged(0);
        oNotify.AssertItemChanged(1);
        oNotify.AssertItemChanged(2);
        oNotify.AssertItemNotChanged(3);
        oNotify.AssertItemNotChanged(4);

        Assert::AreEqual((void*)&pItem1, (void*)vmCollection.GetItemAt(0));
        Assert::AreEqual((void*)&pItem3, (void*)vmCollection.GetItemAt(1));
        Assert::AreEqual((void*)&pItem2, (void*)vmCollection.GetItemAt(2));
        Assert::AreEqual((void*)&pItem4, (void*)vmCollection.GetItemAt(3));
        Assert::AreEqual((void*)&pItem5, (void*)vmCollection.GetItemAt(4));
    }

    TEST_METHOD(TestMoveNewPropertyNotification)
    {
        ViewModelCollection<TestViewModel> vmCollection;
        auto& pItem1 = vmCollection.Add(1, L"Test1");
        auto& pItem2 = vmCollection.Add(2, L"Test2");
        auto& pItem3 = vmCollection.Add(3, L"Test3");
        auto& pItem4 = vmCollection.Add(4, L"Test4");
        auto& pItem5 = vmCollection.Add(5, L"Test5");

        NotifyTargetHarness oNotify;
        vmCollection.AddNotifyTarget(oNotify);

        // add a new item, then move it into slot 3, moving slots 3 and 4 forward one to accommodate
        vmCollection.BeginUpdate();
        auto& pItem6 = vmCollection.Add(6, L"Test6");
        vmCollection.MoveItem(5, 3);
        vmCollection.EndUpdate();

        oNotify.AssertItemNotChanged(0);
        oNotify.AssertItemNotChanged(1);
        oNotify.AssertItemNotChanged(2);
        oNotify.AssertItemAdded(3);
        oNotify.AssertItemNotChanged(4);
        oNotify.AssertItemNotChanged(5);

        Assert::AreEqual((void*)&pItem1, (void*)vmCollection.GetItemAt(0));
        Assert::AreEqual((void*)&pItem2, (void*)vmCollection.GetItemAt(1));
        Assert::AreEqual((void*)&pItem3, (void*)vmCollection.GetItemAt(2));
        Assert::AreEqual((void*)&pItem6, (void*)vmCollection.GetItemAt(3));
        Assert::AreEqual((void*)&pItem4, (void*)vmCollection.GetItemAt(4));
        Assert::AreEqual((void*)&pItem5, (void*)vmCollection.GetItemAt(5));
    }

    TEST_METHOD(TestMoveNewPropertyNotification2)
    {
        ViewModelCollection<TestViewModel> vmCollection;
        auto& pItem1 = vmCollection.Add(1, L"Test1");
        auto& pItem2 = vmCollection.Add(2, L"Test2");
        auto& pItem3 = vmCollection.Add(3, L"Test3");
        auto& pItem4 = vmCollection.Add(4, L"Test4");
        auto& pItem5 = vmCollection.Add(5, L"Test5");

        NotifyTargetHarness oNotify;
        vmCollection.AddNotifyTarget(oNotify);

        // add a new item, then move it into slot 2, moving slots 2, 3, and 4 forward one to accommodate
        // then move the item in slot 4 to slot 1, moving slots 1, 2, and 3 forward one to accomodate
        vmCollection.BeginUpdate();
        auto& pItem6 = vmCollection.Add(6, L"Test6");
        vmCollection.MoveItem(5, 2);
        vmCollection.MoveItem(4, 1);
        vmCollection.EndUpdate();

        oNotify.AssertItemNotChanged(0);  // not moved
        oNotify.AssertItemChanged(1);     // moved from 4->1
        oNotify.AssertItemChanged(2);     // moved from 1->2
        oNotify.AssertItemAdded(3);       // added
        oNotify.AssertItemChanged(4);     // moved from 2->3 and moved by new item
        oNotify.AssertItemNotChanged(5);  // only moved by new item

        Assert::AreEqual((void*)&pItem1, (void*)vmCollection.GetItemAt(0));
        Assert::AreEqual((void*)&pItem4, (void*)vmCollection.GetItemAt(1));
        Assert::AreEqual((void*)&pItem2, (void*)vmCollection.GetItemAt(2));
        Assert::AreEqual((void*)&pItem6, (void*)vmCollection.GetItemAt(3));
        Assert::AreEqual((void*)&pItem3, (void*)vmCollection.GetItemAt(4));
        Assert::AreEqual((void*)&pItem5, (void*)vmCollection.GetItemAt(5));
    }

    TEST_METHOD(TestShiftItemsUp)
    {
        ViewModelCollection<TestViewModel> vmCollection;
        auto& pItem1 = vmCollection.Add(1, L"Test1");
        auto& pItem2 = vmCollection.Add(2, L"Test2");
        auto& pItem3 = vmCollection.Add(3, L"Test3");
        auto& pItem4 = vmCollection.Add(4, L"Test4");
        auto& pItem5 = vmCollection.Add(5, L"Test5");

        pItem2.SetBool(true);
        pItem4.SetBool(true);

        NotifyTargetHarness oNotify;
        vmCollection.AddNotifyTarget(oNotify);

        // items 2 and 4 are selected (indices 1 and 3) - they should get moved into indices 0 and 1
        Assert::AreEqual({ 2U }, vmCollection.ShiftItemsUp(TestViewModel::BoolProperty));

        Assert::AreEqual((void*)&pItem2, (void*)vmCollection.GetItemAt(0));
        Assert::AreEqual((void*)&pItem4, (void*)vmCollection.GetItemAt(1));
        Assert::AreEqual((void*)&pItem1, (void*)vmCollection.GetItemAt(2));
        Assert::AreEqual((void*)&pItem3, (void*)vmCollection.GetItemAt(3));
        Assert::AreEqual((void*)&pItem5, (void*)vmCollection.GetItemAt(4));

        oNotify.AssertItemChanged(0);
        oNotify.AssertItemChanged(1);
        oNotify.AssertItemChanged(2);
        oNotify.AssertItemChanged(3);
        oNotify.AssertItemNotChanged(4);

        pItem4.SetBool(false);
        pItem1.SetBool(true);
        oNotify.ResetChanges();

        // now items 1 and 2 are selected (indices 0 and 2)
        Assert::AreEqual({ 2U }, vmCollection.ShiftItemsUp(TestViewModel::BoolProperty));

        Assert::AreEqual((void*)&pItem2, (void*)vmCollection.GetItemAt(0));
        Assert::AreEqual((void*)&pItem1, (void*)vmCollection.GetItemAt(1));
        Assert::AreEqual((void*)&pItem4, (void*)vmCollection.GetItemAt(2));
        Assert::AreEqual((void*)&pItem3, (void*)vmCollection.GetItemAt(3));
        Assert::AreEqual((void*)&pItem5, (void*)vmCollection.GetItemAt(4));

        oNotify.AssertItemNotChanged(0);
        oNotify.AssertItemChanged(1);
        oNotify.AssertItemChanged(2);
        oNotify.AssertItemNotChanged(3);
        oNotify.AssertItemNotChanged(4);
    }

    TEST_METHOD(TestShiftItemsDown)
    {
        ViewModelCollection<TestViewModel> vmCollection;
        auto& pItem1 = vmCollection.Add(1, L"Test1");
        auto& pItem2 = vmCollection.Add(2, L"Test2");
        auto& pItem3 = vmCollection.Add(3, L"Test3");
        auto& pItem4 = vmCollection.Add(4, L"Test4");
        auto& pItem5 = vmCollection.Add(5, L"Test5");

        pItem2.SetBool(true);
        pItem4.SetBool(true);

        NotifyTargetHarness oNotify;
        vmCollection.AddNotifyTarget(oNotify);

        // items 2 and 4 are selected (indices 1 and 3) - they should get moved into indices 3 and 4
        Assert::AreEqual({ 2U }, vmCollection.ShiftItemsDown(TestViewModel::BoolProperty));

        Assert::AreEqual((void*)&pItem1, (void*)vmCollection.GetItemAt(0));
        Assert::AreEqual((void*)&pItem3, (void*)vmCollection.GetItemAt(1));
        Assert::AreEqual((void*)&pItem5, (void*)vmCollection.GetItemAt(2));
        Assert::AreEqual((void*)&pItem2, (void*)vmCollection.GetItemAt(3));
        Assert::AreEqual((void*)&pItem4, (void*)vmCollection.GetItemAt(4));

        oNotify.AssertItemNotChanged(0);
        oNotify.AssertItemChanged(1);
        oNotify.AssertItemChanged(2);
        oNotify.AssertItemChanged(3);
        oNotify.AssertItemChanged(4);

        pItem2.SetBool(false);
        pItem5.SetBool(true);
        oNotify.ResetChanges();

        // now items 4 and 5 are selected (indices 2 and 4)
        Assert::AreEqual({ 2U }, vmCollection.ShiftItemsDown(TestViewModel::BoolProperty));

        Assert::AreEqual((void*)&pItem1, (void*)vmCollection.GetItemAt(0));
        Assert::AreEqual((void*)&pItem3, (void*)vmCollection.GetItemAt(1));
        Assert::AreEqual((void*)&pItem2, (void*)vmCollection.GetItemAt(2));
        Assert::AreEqual((void*)&pItem5, (void*)vmCollection.GetItemAt(3));
        Assert::AreEqual((void*)&pItem4, (void*)vmCollection.GetItemAt(4));

        oNotify.AssertItemNotChanged(0);
        oNotify.AssertItemNotChanged(1);
        oNotify.AssertItemChanged(2);
        oNotify.AssertItemChanged(3);
        oNotify.AssertItemNotChanged(4);
    }

    TEST_METHOD(TestReverse)
    {
        ViewModelCollection<TestViewModel> vmCollection;
        auto& pItem1 = vmCollection.Add(1, L"Test1");
        auto& pItem2 = vmCollection.Add(2, L"Test2");
        auto& pItem3 = vmCollection.Add(3, L"Test3");
        auto& pItem4 = vmCollection.Add(4, L"Test4");
        auto& pItem5 = vmCollection.Add(5, L"Test5");

        NotifyTargetHarness oNotify;
        vmCollection.AddNotifyTarget(oNotify);

        // odd number of items, middle item shouldn't be notified
        vmCollection.Reverse();

        Assert::AreEqual((void*)&pItem5, (void*)vmCollection.GetItemAt(0));
        Assert::AreEqual((void*)&pItem4, (void*)vmCollection.GetItemAt(1));
        Assert::AreEqual((void*)&pItem3, (void*)vmCollection.GetItemAt(2));
        Assert::AreEqual((void*)&pItem2, (void*)vmCollection.GetItemAt(3));
        Assert::AreEqual((void*)&pItem1, (void*)vmCollection.GetItemAt(4));

        oNotify.AssertItemChanged(0);
        oNotify.AssertItemChanged(1);
        oNotify.AssertItemNotChanged(2);
        oNotify.AssertItemChanged(3);
        oNotify.AssertItemChanged(4);

        auto& pItem0 = vmCollection.Add(0, L"Test0");
        oNotify.ResetChanges();

        // even number of items, all items should be notified
        vmCollection.Reverse();

        Assert::AreEqual((void*)&pItem0, (void*)vmCollection.GetItemAt(0));
        Assert::AreEqual((void*)&pItem1, (void*)vmCollection.GetItemAt(1));
        Assert::AreEqual((void*)&pItem2, (void*)vmCollection.GetItemAt(2));
        Assert::AreEqual((void*)&pItem3, (void*)vmCollection.GetItemAt(3));
        Assert::AreEqual((void*)&pItem4, (void*)vmCollection.GetItemAt(4));
        Assert::AreEqual((void*)&pItem5, (void*)vmCollection.GetItemAt(5));

        oNotify.AssertItemChanged(0);
        oNotify.AssertItemChanged(1);
        oNotify.AssertItemChanged(2);
        oNotify.AssertItemChanged(3);
        oNotify.AssertItemChanged(4);
        oNotify.AssertItemChanged(5);
    }
};

} // namespace tests
} // namespace ui
} // namespace ra
