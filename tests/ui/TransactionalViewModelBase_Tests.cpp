#include "ui\TransactionalViewModelBase.hh"

#include "ra_fwd.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ra {
namespace ui {
namespace tests {

TEST_CLASS(TransactionalViewModelBase_Tests)
{
    class ViewModelHarness : public TransactionalViewModelBase
    {
    public:
        ViewModelHarness() noexcept
        {
            SetTransactional(TransactionalBoolProperty);
            SetTransactional(TransactionalStringProperty);
            SetTransactional(TransactionalIntProperty);
        }

        static const StringModelProperty StringProperty;
        const std::wstring& GetString() const { return GetValue(StringProperty); }
        void SetString(const std::wstring& sValue) { SetValue(StringProperty, sValue); }

        static const IntModelProperty IntProperty;
        int GetInt() const { return GetValue(IntProperty); }
        void SetInt(int nValue) { SetValue(IntProperty, nValue); }

        static const BoolModelProperty BoolProperty;
        bool GetBool() const { return GetValue(BoolProperty); }
        void SetBool(bool bValue) { SetValue(BoolProperty, bValue); }

        static const StringModelProperty TransactionalStringProperty;
        const std::wstring& GetTransactionalString() const { return GetValue(TransactionalStringProperty); }
        void SetTransactionalString(const std::wstring& sValue) { SetValue(TransactionalStringProperty, sValue); }
        const std::wstring* GetPreviousTransactonalString() const { return m_pTransaction->GetPreviousValue(TransactionalStringProperty); }

        static const IntModelProperty TransactionalIntProperty;
        int GetTransactionalInt() const { return GetValue(TransactionalIntProperty); }
        void SetTransactionalInt(int nValue) { SetValue(TransactionalIntProperty, nValue); }
        const int* GetPreviousTransactonalInt() const { return m_pTransaction->GetPreviousValue(TransactionalIntProperty); }

        static const BoolModelProperty TransactionalBoolProperty;
        bool GetTransactionalBool() const { return GetValue(TransactionalBoolProperty); }
        void SetTransactionalBool(bool bValue) { SetValue(TransactionalBoolProperty, bValue); }
        const bool* GetPreviousTransactonalBool() const { return m_pTransaction->GetPreviousValue(TransactionalBoolProperty); }

        bool TransactionIsModified() const noexcept { return m_pTransaction->IsModified(); }
    };

    class NotifyTargetHarness : public ViewModelBase::NotifyTarget
    {
    public:
        void Reset() noexcept
        {
            m_vChangedBools.clear();
            m_vChangedStrings.clear();
            m_vChangedInts.clear();
        }

        void OnViewModelBoolValueChanged(const BoolModelProperty::ChangeArgs& args) override
        {
            m_vChangedBools.push_back({ args.Property, args.tOldValue, args.tNewValue });
        }

        void AssertBoolChanged(const BoolModelProperty& pProperty, bool bOldValue, bool bNewValue)
        {
            for (const auto& args : m_vChangedBools)
            {
                if (args.Property == pProperty)
                {
                    Assert::AreEqual(bOldValue, args.tOldValue);
                    Assert::AreEqual(bNewValue, args.tNewValue);
                    return;
                }
            }

            Assert::Fail(L"Property did not change");
        }

        void AssertNotChanged(const BoolModelProperty& pProperty)
        {
            for (const auto& args : m_vChangedBools)
            {
                if (args.Property == pProperty)
                    Assert::Fail(L"Property changed");
            }
        }

        void OnViewModelStringValueChanged(const StringModelProperty::ChangeArgs& args) override
        {
            m_vChangedStrings.push_back({ args.Property, args.tOldValue, args.tNewValue });
        }

        void AssertStringChanged(const StringModelProperty& pProperty, const std::wstring& sOldValue, const std::wstring& sNewValue)
        {
            for (const auto& args : m_vChangedStrings)
            {
                if (args.Property == pProperty)
                {
                    Assert::AreEqual(sOldValue, args.tOldValue);
                    Assert::AreEqual(sNewValue, args.tNewValue);
                    return;
                }
            }

            Assert::Fail(L"Property did not change");
        }

        void AssertNotChanged(const StringModelProperty& pProperty)
        {
            for (const auto& args : m_vChangedStrings)
            {
                if (args.Property == pProperty)
                    Assert::Fail(L"Property changed");
            }
        }

        void OnViewModelIntValueChanged(const IntModelProperty::ChangeArgs& args) override
        {
            m_vChangedInts.push_back({ args.Property, args.tOldValue, args.tNewValue });
        }

        void AssertIntChanged(const IntModelProperty& pProperty, int nOldValue, int nNewValue)
        {
            for (const auto& args : m_vChangedInts)
            {
                if (args.Property == pProperty)
                {
                    Assert::AreEqual(nOldValue, args.tOldValue);
                    Assert::AreEqual(nNewValue, args.tNewValue);
                    return;
                }
            }

            Assert::Fail(L"Property did not change");
        }

        void AssertNotChanged(const IntModelProperty& pProperty)
        {
            for (const auto& args : m_vChangedInts)
            {
                if (args.Property == pProperty)
                    Assert::Fail(L"Property changed");
            }
        }

    private:
        struct BoolChange
        {
            const BoolModelProperty& Property;
            bool tOldValue;
            bool tNewValue;
        };

        struct StringChange
        {
            const StringModelProperty& Property;
            std::wstring tOldValue;
            std::wstring tNewValue;
        };

        struct IntChange
        {
            const IntModelProperty& Property;
            int tOldValue;
            int tNewValue;
        };

        std::vector<BoolChange> m_vChangedBools;
        std::vector<StringChange> m_vChangedStrings;
        std::vector<IntChange> m_vChangedInts;
    };

public:
    TEST_METHOD(TestTransactionalBoolProperty)
    {
        ViewModelHarness vmViewModel;
        Assert::IsFalse(vmViewModel.GetBool());
        Assert::IsFalse(vmViewModel.GetTransactionalBool());
        vmViewModel.BeginTransaction();

        NotifyTargetHarness oNotify;
        vmViewModel.AddNotifyTarget(oNotify);

        vmViewModel.SetBool(true);
        Assert::IsTrue(vmViewModel.GetBool());
        Assert::IsFalse(vmViewModel.GetTransactionalBool());
        Assert::IsFalse(vmViewModel.IsModified());
        Assert::IsFalse(vmViewModel.IsModified(ViewModelHarness::BoolProperty));
        Assert::IsFalse(vmViewModel.IsModified(ViewModelHarness::TransactionalBoolProperty));
        oNotify.AssertBoolChanged(ViewModelHarness::BoolProperty, false, true);
        oNotify.AssertNotChanged(ViewModelHarness::IsModifiedProperty);
        oNotify.Reset();

        vmViewModel.SetTransactionalBool(true);
        Assert::IsTrue(vmViewModel.GetBool());
        Assert::IsTrue(vmViewModel.GetTransactionalBool());
        Assert::IsTrue(vmViewModel.IsModified());
        Assert::IsFalse(vmViewModel.IsModified(ViewModelHarness::BoolProperty));
        Assert::IsTrue(vmViewModel.IsModified(ViewModelHarness::TransactionalBoolProperty));
        oNotify.AssertBoolChanged(ViewModelHarness::TransactionalBoolProperty, false, true);
        oNotify.AssertBoolChanged(ViewModelHarness::IsModifiedProperty, false, true);
        oNotify.Reset();

        vmViewModel.SetTransactionalBool(true);
        Assert::IsTrue(vmViewModel.GetBool());
        Assert::IsTrue(vmViewModel.GetTransactionalBool());
        Assert::IsTrue(vmViewModel.IsModified());
        Assert::IsFalse(vmViewModel.IsModified(ViewModelHarness::BoolProperty));
        Assert::IsTrue(vmViewModel.IsModified(ViewModelHarness::TransactionalBoolProperty));
        oNotify.AssertNotChanged(ViewModelHarness::TransactionalBoolProperty);
        oNotify.AssertNotChanged(ViewModelHarness::IsModifiedProperty);
        oNotify.Reset();

        vmViewModel.SetTransactionalBool(false);
        Assert::IsTrue(vmViewModel.GetBool());
        Assert::IsFalse(vmViewModel.GetTransactionalBool());
        Assert::IsFalse(vmViewModel.IsModified());
        Assert::IsFalse(vmViewModel.IsModified(ViewModelHarness::BoolProperty));
        Assert::IsFalse(vmViewModel.IsModified(ViewModelHarness::TransactionalBoolProperty));
        oNotify.AssertBoolChanged(ViewModelHarness::TransactionalBoolProperty, true, false);
        oNotify.AssertBoolChanged(ViewModelHarness::IsModifiedProperty, true, false);
    }

    TEST_METHOD(TestTransactionalStringProperty)
    {
        ViewModelHarness vmViewModel;
        Assert::AreEqual(std::wstring(), vmViewModel.GetString());
        Assert::AreEqual(std::wstring(), vmViewModel.GetTransactionalString());
        vmViewModel.BeginTransaction();

        NotifyTargetHarness oNotify;
        vmViewModel.AddNotifyTarget(oNotify);

        vmViewModel.SetString(L"Test");
        Assert::AreEqual(std::wstring(L"Test"), vmViewModel.GetString());
        Assert::AreEqual(std::wstring(), vmViewModel.GetTransactionalString());
        Assert::IsFalse(vmViewModel.IsModified());
        Assert::IsFalse(vmViewModel.IsModified(ViewModelHarness::StringProperty));
        Assert::IsFalse(vmViewModel.IsModified(ViewModelHarness::TransactionalStringProperty));
        oNotify.AssertStringChanged(ViewModelHarness::StringProperty, L"", L"Test");
        oNotify.AssertNotChanged(ViewModelHarness::IsModifiedProperty);
        oNotify.Reset();

        vmViewModel.SetTransactionalString(L"Test");
        Assert::AreEqual(std::wstring(L"Test"), vmViewModel.GetString());
        Assert::AreEqual(std::wstring(L"Test"), vmViewModel.GetTransactionalString());
        Assert::IsTrue(vmViewModel.IsModified());
        Assert::IsFalse(vmViewModel.IsModified(ViewModelHarness::StringProperty));
        Assert::IsTrue(vmViewModel.IsModified(ViewModelHarness::TransactionalStringProperty));
        oNotify.AssertStringChanged(ViewModelHarness::TransactionalStringProperty, L"", L"Test");
        oNotify.AssertBoolChanged(ViewModelHarness::IsModifiedProperty, false, true);
        oNotify.Reset();

        vmViewModel.SetTransactionalString(L"Test");
        Assert::AreEqual(std::wstring(L"Test"), vmViewModel.GetString());
        Assert::AreEqual(std::wstring(L"Test"), vmViewModel.GetTransactionalString());
        Assert::IsTrue(vmViewModel.IsModified());
        Assert::IsFalse(vmViewModel.IsModified(ViewModelHarness::StringProperty));
        Assert::IsTrue(vmViewModel.IsModified(ViewModelHarness::TransactionalStringProperty));
        oNotify.AssertNotChanged(ViewModelHarness::TransactionalStringProperty);
        oNotify.AssertNotChanged(ViewModelHarness::IsModifiedProperty);
        oNotify.Reset();

        vmViewModel.SetTransactionalString(L"");
        Assert::AreEqual(std::wstring(L"Test"), vmViewModel.GetString());
        Assert::AreEqual(std::wstring(L""), vmViewModel.GetTransactionalString());
        Assert::IsFalse(vmViewModel.IsModified());
        Assert::IsFalse(vmViewModel.IsModified(ViewModelHarness::StringProperty));
        Assert::IsFalse(vmViewModel.IsModified(ViewModelHarness::TransactionalStringProperty));
        oNotify.AssertStringChanged(ViewModelHarness::TransactionalStringProperty, L"Test", L"");
        oNotify.AssertBoolChanged(ViewModelHarness::IsModifiedProperty, true, false);
    }

    TEST_METHOD(TestTransactionalIntProperty)
    {
        ViewModelHarness vmViewModel;
        Assert::AreEqual(0, vmViewModel.GetInt());
        Assert::AreEqual(0, vmViewModel.GetTransactionalInt());
        vmViewModel.BeginTransaction();

        NotifyTargetHarness oNotify;
        vmViewModel.AddNotifyTarget(oNotify);

        vmViewModel.SetInt(99);
        Assert::AreEqual(99, vmViewModel.GetInt());
        Assert::AreEqual(0, vmViewModel.GetTransactionalInt());
        Assert::IsFalse(vmViewModel.IsModified());
        Assert::IsFalse(vmViewModel.IsModified(ViewModelHarness::IntProperty));
        Assert::IsFalse(vmViewModel.IsModified(ViewModelHarness::TransactionalIntProperty));
        oNotify.AssertIntChanged(ViewModelHarness::IntProperty, 0, 99);
        oNotify.AssertNotChanged(ViewModelHarness::IsModifiedProperty);
        oNotify.Reset();

        vmViewModel.SetTransactionalInt(99);
        Assert::AreEqual(99, vmViewModel.GetInt());
        Assert::AreEqual(99, vmViewModel.GetTransactionalInt());
        Assert::IsTrue(vmViewModel.IsModified());
        Assert::IsFalse(vmViewModel.IsModified(ViewModelHarness::IntProperty));
        Assert::IsTrue(vmViewModel.IsModified(ViewModelHarness::TransactionalIntProperty));
        oNotify.AssertIntChanged(ViewModelHarness::TransactionalIntProperty, 0, 99);
        oNotify.AssertBoolChanged(ViewModelHarness::IsModifiedProperty, false, true);
        oNotify.Reset();

        vmViewModel.SetTransactionalInt(99);
        Assert::AreEqual(99, vmViewModel.GetInt());
        Assert::AreEqual(99, vmViewModel.GetTransactionalInt());
        Assert::IsTrue(vmViewModel.IsModified());
        Assert::IsFalse(vmViewModel.IsModified(ViewModelHarness::IntProperty));
        Assert::IsTrue(vmViewModel.IsModified(ViewModelHarness::TransactionalIntProperty));
        oNotify.AssertNotChanged(ViewModelHarness::TransactionalIntProperty);
        oNotify.AssertNotChanged(ViewModelHarness::IsModifiedProperty);
        oNotify.Reset();

        vmViewModel.SetTransactionalInt(0);
        Assert::AreEqual(99, vmViewModel.GetInt());
        Assert::AreEqual(0, vmViewModel.GetTransactionalInt());
        Assert::IsFalse(vmViewModel.IsModified());
        Assert::IsFalse(vmViewModel.IsModified(ViewModelHarness::IntProperty));
        Assert::IsFalse(vmViewModel.IsModified(ViewModelHarness::TransactionalIntProperty));
        oNotify.AssertIntChanged(ViewModelHarness::TransactionalIntProperty, 99, 0);
        oNotify.AssertBoolChanged(ViewModelHarness::IsModifiedProperty, true, false);
    }

    TEST_METHOD(TestRevertTransaction)
    {
        ViewModelHarness vmViewModel;
        NotifyTargetHarness oNotify;
        vmViewModel.AddNotifyTarget(oNotify);
        vmViewModel.BeginTransaction();

        Assert::IsFalse(vmViewModel.IsModified());
        Assert::IsFalse(vmViewModel.IsModified(ViewModelHarness::TransactionalIntProperty));
        Assert::IsFalse(vmViewModel.IsModified(ViewModelHarness::TransactionalStringProperty));
        Assert::IsFalse(vmViewModel.IsModified(ViewModelHarness::TransactionalBoolProperty));

        vmViewModel.SetTransactionalInt(99);
        vmViewModel.SetTransactionalString(L"Test");
        Assert::IsTrue(vmViewModel.IsModified());
        Assert::IsTrue(vmViewModel.IsModified(ViewModelHarness::TransactionalIntProperty));
        Assert::IsTrue(vmViewModel.IsModified(ViewModelHarness::TransactionalStringProperty));
        Assert::IsFalse(vmViewModel.IsModified(ViewModelHarness::TransactionalBoolProperty));
        oNotify.Reset();

        vmViewModel.RevertTransaction();
        Assert::IsFalse(vmViewModel.IsModified());
        Assert::IsFalse(vmViewModel.IsModified(ViewModelHarness::TransactionalIntProperty));
        Assert::IsFalse(vmViewModel.IsModified(ViewModelHarness::TransactionalStringProperty));
        Assert::IsFalse(vmViewModel.IsModified(ViewModelHarness::TransactionalBoolProperty));

        Assert::AreEqual(0, vmViewModel.GetTransactionalInt());
        Assert::AreEqual(std::wstring(L""), vmViewModel.GetTransactionalString());
        Assert::AreEqual(false, vmViewModel.GetTransactionalBool());

        // no transaction to revert - modifications should be kept, but not tracked
        vmViewModel.SetTransactionalInt(99);
        vmViewModel.SetTransactionalString(L"Test");
        vmViewModel.RevertTransaction();
        Assert::IsFalse(vmViewModel.IsModified());
        Assert::IsFalse(vmViewModel.IsModified(ViewModelHarness::TransactionalIntProperty));
        Assert::IsFalse(vmViewModel.IsModified(ViewModelHarness::TransactionalStringProperty));
        Assert::IsFalse(vmViewModel.IsModified(ViewModelHarness::TransactionalBoolProperty));

        Assert::AreEqual(99, vmViewModel.GetTransactionalInt());
        Assert::AreEqual(std::wstring(L"Test"), vmViewModel.GetTransactionalString());
        Assert::AreEqual(false, vmViewModel.GetTransactionalBool());
    }

    TEST_METHOD(TestRevertTransactionNested)
    {
        ViewModelHarness vmViewModel;
        NotifyTargetHarness oNotify;
        vmViewModel.AddNotifyTarget(oNotify);

        // create a transaction and modify it
        vmViewModel.BeginTransaction();
        vmViewModel.SetTransactionalInt(99);
        vmViewModel.SetTransactionalString(L"Test");
        Assert::IsTrue(vmViewModel.IsModified());
        Assert::IsTrue(vmViewModel.IsModified(ViewModelHarness::TransactionalIntProperty));
        Assert::IsTrue(vmViewModel.IsModified(ViewModelHarness::TransactionalStringProperty));
        Assert::IsFalse(vmViewModel.IsModified(ViewModelHarness::TransactionalBoolProperty));
        oNotify.Reset();

        // nested transaction is not initially considered modified
        vmViewModel.BeginTransaction();
        Assert::IsFalse(vmViewModel.IsModified());
        Assert::IsFalse(vmViewModel.IsModified(ViewModelHarness::TransactionalIntProperty));
        Assert::IsFalse(vmViewModel.IsModified(ViewModelHarness::TransactionalStringProperty));
        Assert::IsFalse(vmViewModel.IsModified(ViewModelHarness::TransactionalBoolProperty));

        oNotify.AssertNotChanged(ViewModelHarness::TransactionalIntProperty);
        oNotify.AssertNotChanged(ViewModelHarness::TransactionalStringProperty);
        oNotify.AssertNotChanged(ViewModelHarness::TransactionalBoolProperty);
        oNotify.AssertBoolChanged(ViewModelHarness::IsModifiedProperty, true, false);
        oNotify.Reset();

        Assert::AreEqual(99, vmViewModel.GetTransactionalInt());
        Assert::AreEqual(std::wstring(L"Test"), vmViewModel.GetTransactionalString());
        Assert::AreEqual(false, vmViewModel.GetTransactionalBool());

        // revert nested transaction, will pick up modifications from first transaction
        vmViewModel.RevertTransaction();
        Assert::IsTrue(vmViewModel.IsModified());
        Assert::IsTrue(vmViewModel.IsModified(ViewModelHarness::TransactionalIntProperty));
        Assert::IsTrue(vmViewModel.IsModified(ViewModelHarness::TransactionalStringProperty));
        Assert::IsFalse(vmViewModel.IsModified(ViewModelHarness::TransactionalBoolProperty));

        oNotify.AssertNotChanged(ViewModelHarness::TransactionalIntProperty);
        oNotify.AssertNotChanged(ViewModelHarness::TransactionalStringProperty);
        oNotify.AssertNotChanged(ViewModelHarness::TransactionalBoolProperty);
        oNotify.AssertBoolChanged(ViewModelHarness::IsModifiedProperty, false, true);
        oNotify.Reset();

        // another nested transacion, this time, we'll modify it
        vmViewModel.BeginTransaction();
        Assert::AreEqual(99, vmViewModel.GetTransactionalInt());
        Assert::AreEqual(std::wstring(L"Test"), vmViewModel.GetTransactionalString());
        Assert::AreEqual(false, vmViewModel.GetTransactionalBool());

        vmViewModel.SetTransactionalInt(50);
        Assert::IsTrue(vmViewModel.IsModified());
        Assert::IsTrue(vmViewModel.IsModified(ViewModelHarness::TransactionalIntProperty));
        Assert::IsFalse(vmViewModel.IsModified(ViewModelHarness::TransactionalStringProperty));
        Assert::IsFalse(vmViewModel.IsModified(ViewModelHarness::TransactionalBoolProperty));
        oNotify.Reset();

        // revert nested transaction, will pick up modifications from first transaction
        vmViewModel.RevertTransaction();
        Assert::IsTrue(vmViewModel.IsModified());
        Assert::IsTrue(vmViewModel.IsModified(ViewModelHarness::TransactionalIntProperty));
        Assert::IsTrue(vmViewModel.IsModified(ViewModelHarness::TransactionalStringProperty));
        Assert::IsFalse(vmViewModel.IsModified(ViewModelHarness::TransactionalBoolProperty));

        oNotify.AssertIntChanged(ViewModelHarness::TransactionalIntProperty, 50, 99);
        oNotify.AssertNotChanged(ViewModelHarness::TransactionalStringProperty);
        oNotify.AssertNotChanged(ViewModelHarness::TransactionalBoolProperty);
        oNotify.AssertNotChanged(ViewModelHarness::IsModifiedProperty);
        oNotify.Reset();

        Assert::AreEqual(99, vmViewModel.GetTransactionalInt());
        Assert::AreEqual(std::wstring(L"Test"), vmViewModel.GetTransactionalString());
        Assert::AreEqual(false, vmViewModel.GetTransactionalBool());

        // revert primary transaction
        vmViewModel.RevertTransaction();
        Assert::IsFalse(vmViewModel.IsModified());
        Assert::IsFalse(vmViewModel.IsModified(ViewModelHarness::TransactionalIntProperty));
        Assert::IsFalse(vmViewModel.IsModified(ViewModelHarness::TransactionalStringProperty));
        Assert::IsFalse(vmViewModel.IsModified(ViewModelHarness::TransactionalBoolProperty));

        oNotify.AssertIntChanged(ViewModelHarness::TransactionalIntProperty, 99, 0);
        oNotify.AssertStringChanged(ViewModelHarness::TransactionalStringProperty, L"Test", L"");
        oNotify.AssertNotChanged(ViewModelHarness::TransactionalBoolProperty);
        oNotify.AssertBoolChanged(ViewModelHarness::IsModifiedProperty, true, false);

        Assert::AreEqual(0, vmViewModel.GetTransactionalInt());
        Assert::AreEqual(std::wstring(L""), vmViewModel.GetTransactionalString());
        Assert::AreEqual(false, vmViewModel.GetTransactionalBool());
    }

    TEST_METHOD(TestRevertTransactionRepeated)
    {
        ViewModelHarness vmViewModel;
        NotifyTargetHarness oNotify;
        vmViewModel.AddNotifyTarget(oNotify);

        Assert::AreEqual(0, vmViewModel.GetTransactionalInt());

        vmViewModel.BeginTransaction();
        vmViewModel.BeginTransaction();
        vmViewModel.SetTransactionalInt(99);
        Assert::AreEqual(99, vmViewModel.GetTransactionalInt());

        vmViewModel.RevertTransaction();
        Assert::AreEqual(0, vmViewModel.GetTransactionalInt());
        Assert::IsFalse(vmViewModel.IsModified());

        vmViewModel.RevertTransaction();
        Assert::AreEqual(0, vmViewModel.GetTransactionalInt());
        Assert::IsFalse(vmViewModel.IsModified());
    }

    TEST_METHOD(TestRevertTransactionMultipleChanges)
    {
        ViewModelHarness vmViewModel;
        NotifyTargetHarness oNotify;
        vmViewModel.AddNotifyTarget(oNotify);

        vmViewModel.SetTransactionalInt(99);
        vmViewModel.SetTransactionalString(L"Test");
        vmViewModel.BeginTransaction();
        oNotify.Reset();

        Assert::IsFalse(vmViewModel.IsModified());
        Assert::IsFalse(vmViewModel.IsModified(ViewModelHarness::TransactionalIntProperty));
        Assert::IsFalse(vmViewModel.IsModified(ViewModelHarness::TransactionalStringProperty));
        Assert::IsFalse(vmViewModel.IsModified(ViewModelHarness::TransactionalBoolProperty));

        vmViewModel.SetTransactionalInt(50);
        vmViewModel.SetTransactionalString(L"Test2");
        Assert::IsTrue(vmViewModel.IsModified());
        Assert::IsTrue(vmViewModel.IsModified(ViewModelHarness::TransactionalIntProperty));
        Assert::IsTrue(vmViewModel.IsModified(ViewModelHarness::TransactionalStringProperty));
        Assert::IsFalse(vmViewModel.IsModified(ViewModelHarness::TransactionalBoolProperty));
        oNotify.AssertStringChanged(ViewModelHarness::TransactionalStringProperty, L"Test", L"Test2");
        oNotify.AssertIntChanged(ViewModelHarness::TransactionalIntProperty, 99, 50);
        oNotify.AssertBoolChanged(ViewModelHarness::IsModifiedProperty, false, true);
        oNotify.Reset();

        vmViewModel.SetTransactionalInt(75);
        vmViewModel.SetTransactionalString(L"Test3");
        Assert::IsTrue(vmViewModel.IsModified());
        Assert::IsTrue(vmViewModel.IsModified(ViewModelHarness::TransactionalIntProperty));
        Assert::IsTrue(vmViewModel.IsModified(ViewModelHarness::TransactionalStringProperty));
        Assert::IsFalse(vmViewModel.IsModified(ViewModelHarness::TransactionalBoolProperty));
        oNotify.AssertStringChanged(ViewModelHarness::TransactionalStringProperty, L"Test2", L"Test3");
        oNotify.AssertIntChanged(ViewModelHarness::TransactionalIntProperty, 50, 75);
        oNotify.AssertNotChanged(ViewModelHarness::IsModifiedProperty);
        oNotify.Reset();

        // revert should return to original state, regardless of how many times values have changed within the transaction
        vmViewModel.RevertTransaction();
        Assert::IsFalse(vmViewModel.IsModified());
        Assert::IsFalse(vmViewModel.IsModified(ViewModelHarness::TransactionalIntProperty));
        Assert::IsFalse(vmViewModel.IsModified(ViewModelHarness::TransactionalStringProperty));
        Assert::IsFalse(vmViewModel.IsModified(ViewModelHarness::TransactionalBoolProperty));
        oNotify.AssertStringChanged(ViewModelHarness::TransactionalStringProperty, L"Test3", L"Test");
        oNotify.AssertIntChanged(ViewModelHarness::TransactionalIntProperty, 75, 99);
        oNotify.AssertBoolChanged(ViewModelHarness::IsModifiedProperty, true, false);
    }

    TEST_METHOD(TestCommitTransaction)
    {
        ViewModelHarness vmViewModel;
        NotifyTargetHarness oNotify;
        vmViewModel.AddNotifyTarget(oNotify);
        vmViewModel.BeginTransaction();

        Assert::IsFalse(vmViewModel.IsModified());
        Assert::IsFalse(vmViewModel.IsModified(ViewModelHarness::TransactionalIntProperty));
        Assert::IsFalse(vmViewModel.IsModified(ViewModelHarness::TransactionalStringProperty));
        Assert::IsFalse(vmViewModel.IsModified(ViewModelHarness::TransactionalBoolProperty));

        vmViewModel.SetTransactionalInt(99);
        vmViewModel.SetTransactionalString(L"Test");
        Assert::IsTrue(vmViewModel.IsModified());
        Assert::IsTrue(vmViewModel.IsModified(ViewModelHarness::TransactionalIntProperty));
        Assert::IsTrue(vmViewModel.IsModified(ViewModelHarness::TransactionalStringProperty));
        Assert::IsFalse(vmViewModel.IsModified(ViewModelHarness::TransactionalBoolProperty));
        oNotify.Reset();

        vmViewModel.CommitTransaction();
        Assert::IsFalse(vmViewModel.IsModified());
        Assert::IsFalse(vmViewModel.IsModified(ViewModelHarness::TransactionalIntProperty));
        Assert::IsFalse(vmViewModel.IsModified(ViewModelHarness::TransactionalStringProperty));
        Assert::IsFalse(vmViewModel.IsModified(ViewModelHarness::TransactionalBoolProperty));

        Assert::AreEqual(99, vmViewModel.GetTransactionalInt());
        Assert::AreEqual(std::wstring(L"Test"), vmViewModel.GetTransactionalString());
        Assert::AreEqual(false, vmViewModel.GetTransactionalBool());

        // no transaction to commit - modifications should be kept, but not tracked
        vmViewModel.SetTransactionalInt(50);
        vmViewModel.SetTransactionalString(L"Test2");
        vmViewModel.RevertTransaction();
        Assert::IsFalse(vmViewModel.IsModified());
        Assert::IsFalse(vmViewModel.IsModified(ViewModelHarness::TransactionalIntProperty));
        Assert::IsFalse(vmViewModel.IsModified(ViewModelHarness::TransactionalStringProperty));
        Assert::IsFalse(vmViewModel.IsModified(ViewModelHarness::TransactionalBoolProperty));

        Assert::AreEqual(50, vmViewModel.GetTransactionalInt());
        Assert::AreEqual(std::wstring(L"Test2"), vmViewModel.GetTransactionalString());
        Assert::AreEqual(false, vmViewModel.GetTransactionalBool());
    }

    TEST_METHOD(TestCommitTransactionNested)
    {
        ViewModelHarness vmViewModel;
        NotifyTargetHarness oNotify;
        vmViewModel.AddNotifyTarget(oNotify);

        // create a transaction and modify it
        vmViewModel.BeginTransaction();
        vmViewModel.SetTransactionalInt(99);
        vmViewModel.SetTransactionalString(L"Test");
        Assert::IsTrue(vmViewModel.IsModified());
        Assert::IsTrue(vmViewModel.IsModified(ViewModelHarness::TransactionalIntProperty));
        Assert::IsTrue(vmViewModel.IsModified(ViewModelHarness::TransactionalStringProperty));
        Assert::IsFalse(vmViewModel.IsModified(ViewModelHarness::TransactionalBoolProperty));
        oNotify.Reset();

        // nested transaction is not initially considered modified
        vmViewModel.BeginTransaction();
        Assert::IsFalse(vmViewModel.IsModified());
        Assert::IsFalse(vmViewModel.IsModified(ViewModelHarness::TransactionalIntProperty));
        Assert::IsFalse(vmViewModel.IsModified(ViewModelHarness::TransactionalStringProperty));
        Assert::IsFalse(vmViewModel.IsModified(ViewModelHarness::TransactionalBoolProperty));

        oNotify.AssertNotChanged(ViewModelHarness::TransactionalIntProperty);
        oNotify.AssertNotChanged(ViewModelHarness::TransactionalStringProperty);
        oNotify.AssertNotChanged(ViewModelHarness::TransactionalBoolProperty);
        oNotify.AssertBoolChanged(ViewModelHarness::IsModifiedProperty, true, false);
        oNotify.Reset();

        Assert::AreEqual(99, vmViewModel.GetTransactionalInt());
        Assert::AreEqual(std::wstring(L"Test"), vmViewModel.GetTransactionalString());
        Assert::AreEqual(false, vmViewModel.GetTransactionalBool());

        // commit (unmodified) nested transaction, will pick up modifications from first transaction
        vmViewModel.CommitTransaction();
        Assert::IsTrue(vmViewModel.IsModified());
        Assert::IsTrue(vmViewModel.IsModified(ViewModelHarness::TransactionalIntProperty));
        Assert::IsTrue(vmViewModel.IsModified(ViewModelHarness::TransactionalStringProperty));
        Assert::IsFalse(vmViewModel.IsModified(ViewModelHarness::TransactionalBoolProperty));

        oNotify.AssertNotChanged(ViewModelHarness::TransactionalIntProperty);
        oNotify.AssertNotChanged(ViewModelHarness::TransactionalStringProperty);
        oNotify.AssertNotChanged(ViewModelHarness::TransactionalBoolProperty);
        oNotify.AssertBoolChanged(ViewModelHarness::IsModifiedProperty, false, true);
        oNotify.Reset();

        // another nested transacion, this time, we'll modify it
        vmViewModel.BeginTransaction();
        Assert::AreEqual(99, vmViewModel.GetTransactionalInt());
        Assert::AreEqual(std::wstring(L"Test"), vmViewModel.GetTransactionalString());
        Assert::AreEqual(false, vmViewModel.GetTransactionalBool());

        vmViewModel.SetTransactionalInt(50);
        Assert::IsTrue(vmViewModel.IsModified());
        Assert::IsTrue(vmViewModel.IsModified(ViewModelHarness::TransactionalIntProperty));
        Assert::IsFalse(vmViewModel.IsModified(ViewModelHarness::TransactionalStringProperty));
        Assert::IsFalse(vmViewModel.IsModified(ViewModelHarness::TransactionalBoolProperty));
        oNotify.Reset();

        // revert nested transaction, will pick up modifications from first transaction
        vmViewModel.CommitTransaction();
        Assert::IsTrue(vmViewModel.IsModified());
        Assert::IsTrue(vmViewModel.IsModified(ViewModelHarness::TransactionalIntProperty));
        Assert::IsTrue(vmViewModel.IsModified(ViewModelHarness::TransactionalStringProperty));
        Assert::IsFalse(vmViewModel.IsModified(ViewModelHarness::TransactionalBoolProperty));

        oNotify.AssertNotChanged(ViewModelHarness::TransactionalIntProperty);
        oNotify.AssertNotChanged(ViewModelHarness::TransactionalStringProperty);
        oNotify.AssertNotChanged(ViewModelHarness::TransactionalBoolProperty);
        oNotify.AssertNotChanged(ViewModelHarness::IsModifiedProperty);
        oNotify.Reset();

        Assert::AreEqual(50, vmViewModel.GetTransactionalInt());
        Assert::AreEqual(std::wstring(L"Test"), vmViewModel.GetTransactionalString());
        Assert::AreEqual(false, vmViewModel.GetTransactionalBool());

        // commit primary transaction
        vmViewModel.CommitTransaction();
        Assert::IsFalse(vmViewModel.IsModified());
        Assert::IsFalse(vmViewModel.IsModified(ViewModelHarness::TransactionalIntProperty));
        Assert::IsFalse(vmViewModel.IsModified(ViewModelHarness::TransactionalStringProperty));
        Assert::IsFalse(vmViewModel.IsModified(ViewModelHarness::TransactionalBoolProperty));

        oNotify.AssertNotChanged(ViewModelHarness::TransactionalIntProperty);
        oNotify.AssertNotChanged(ViewModelHarness::TransactionalStringProperty);
        oNotify.AssertNotChanged(ViewModelHarness::TransactionalBoolProperty);
        oNotify.AssertBoolChanged(ViewModelHarness::IsModifiedProperty, true, false);

        Assert::AreEqual(50, vmViewModel.GetTransactionalInt());
        Assert::AreEqual(std::wstring(L"Test"), vmViewModel.GetTransactionalString());
        Assert::AreEqual(false, vmViewModel.GetTransactionalBool());
    }

    TEST_METHOD(TestCommitTransactionNestedOriginalValues)
    {
        ViewModelHarness vmViewModel;
        NotifyTargetHarness oNotify;
        vmViewModel.AddNotifyTarget(oNotify);

        // set some initial values
        vmViewModel.SetTransactionalInt(99);
        vmViewModel.SetTransactionalString(L"Test");
        oNotify.Reset();

        // create a transaction and modify it
        vmViewModel.BeginTransaction();
        vmViewModel.SetTransactionalInt(50);
        vmViewModel.SetTransactionalString(L"Test2");
        oNotify.Reset();

        // create another transaction and set the values back to the original values
        vmViewModel.BeginTransaction();
        vmViewModel.SetTransactionalInt(99);
        vmViewModel.SetTransactionalString(L"Test");

        Assert::IsTrue(vmViewModel.IsModified());
        Assert::IsTrue(vmViewModel.IsModified(ViewModelHarness::TransactionalIntProperty));
        Assert::IsTrue(vmViewModel.IsModified(ViewModelHarness::TransactionalStringProperty));
        Assert::IsFalse(vmViewModel.IsModified(ViewModelHarness::TransactionalBoolProperty));

        oNotify.AssertIntChanged(ViewModelHarness::TransactionalIntProperty, 50, 99);
        oNotify.AssertStringChanged(ViewModelHarness::TransactionalStringProperty, L"Test2", L"Test");
        oNotify.AssertNotChanged(ViewModelHarness::TransactionalBoolProperty);
        oNotify.Reset();

        Assert::AreEqual(99, vmViewModel.GetTransactionalInt());
        Assert::AreEqual(std::wstring(L"Test"), vmViewModel.GetTransactionalString());
        Assert::AreEqual(false, vmViewModel.GetTransactionalBool());

        // commit nested transaction, since the values match the original values, the original transaction will appear unmodified
        vmViewModel.CommitTransaction();
        Assert::IsFalse(vmViewModel.IsModified());
        Assert::IsFalse(vmViewModel.IsModified(ViewModelHarness::TransactionalIntProperty));
        Assert::IsFalse(vmViewModel.IsModified(ViewModelHarness::TransactionalStringProperty));
        Assert::IsFalse(vmViewModel.IsModified(ViewModelHarness::TransactionalBoolProperty));

        oNotify.AssertNotChanged(ViewModelHarness::TransactionalIntProperty);
        oNotify.AssertNotChanged(ViewModelHarness::TransactionalStringProperty);
        oNotify.AssertNotChanged(ViewModelHarness::TransactionalBoolProperty);
        oNotify.AssertBoolChanged(ViewModelHarness::IsModifiedProperty, true, false);
        oNotify.Reset();

        Assert::AreEqual(99, vmViewModel.GetTransactionalInt());
        Assert::AreEqual(std::wstring(L"Test"), vmViewModel.GetTransactionalString());
        Assert::AreEqual(false, vmViewModel.GetTransactionalBool());

        // commiting the now-unmodified original transaction should have no effect
        vmViewModel.CommitTransaction();
        Assert::IsFalse(vmViewModel.IsModified());
        Assert::IsFalse(vmViewModel.IsModified(ViewModelHarness::TransactionalIntProperty));
        Assert::IsFalse(vmViewModel.IsModified(ViewModelHarness::TransactionalStringProperty));
        Assert::IsFalse(vmViewModel.IsModified(ViewModelHarness::TransactionalBoolProperty));

        oNotify.AssertNotChanged(ViewModelHarness::TransactionalIntProperty);
        oNotify.AssertNotChanged(ViewModelHarness::TransactionalStringProperty);
        oNotify.AssertNotChanged(ViewModelHarness::TransactionalBoolProperty);
        oNotify.AssertNotChanged(ViewModelHarness::IsModifiedProperty);

        Assert::AreEqual(99, vmViewModel.GetTransactionalInt());
        Assert::AreEqual(std::wstring(L"Test"), vmViewModel.GetTransactionalString());
        Assert::AreEqual(false, vmViewModel.GetTransactionalBool());
    }

    TEST_METHOD(TestPreviousValues)
    {
        ViewModelHarness vmViewModel;
        vmViewModel.BeginTransaction();

        // no previous value captured
        Assert::IsNull(vmViewModel.GetPreviousTransactonalBool());
        Assert::IsNull(vmViewModel.GetPreviousTransactonalInt());
        Assert::IsNull(vmViewModel.GetPreviousTransactonalString());

        vmViewModel.BeginTransaction();
        vmViewModel.SetTransactionalBool(true);
        vmViewModel.SetTransactionalInt(50);
        vmViewModel.SetTransactionalString(L"Test2");

        // original values captured
        Assert::AreEqual(false, *vmViewModel.GetPreviousTransactonalBool());
        Assert::AreEqual(0, *vmViewModel.GetPreviousTransactonalInt());
        Assert::AreEqual(std::wstring(), *vmViewModel.GetPreviousTransactonalString());

        vmViewModel.BeginTransaction();

        // no previous values captured for this transaction
        Assert::IsNull(vmViewModel.GetPreviousTransactonalBool());
        Assert::IsNull(vmViewModel.GetPreviousTransactonalInt());
        Assert::IsNull(vmViewModel.GetPreviousTransactonalString());

        vmViewModel.SetTransactionalBool(false);
        vmViewModel.SetTransactionalInt(100);
        vmViewModel.SetTransactionalString(L"Test3");

        // previous values captured
        Assert::AreEqual(true, *vmViewModel.GetPreviousTransactonalBool());
        Assert::AreEqual(50, *vmViewModel.GetPreviousTransactonalInt());
        Assert::AreEqual(std::wstring(L"Test2"), *vmViewModel.GetPreviousTransactonalString());

        vmViewModel.CommitTransaction();

        // previous values updated from commit - since the boolean matches the original value
        // it returns null. the other older original values are maintained
        Assert::IsNull(vmViewModel.GetPreviousTransactonalBool());
        Assert::AreEqual(0, *vmViewModel.GetPreviousTransactonalInt());
        Assert::AreEqual(std::wstring(), *vmViewModel.GetPreviousTransactonalString());
    }

    TEST_METHOD(TestIsModified)
    {
        ViewModelHarness vmViewModel;
        vmViewModel.BeginTransaction();

        Assert::IsFalse(vmViewModel.TransactionIsModified());

        vmViewModel.BeginTransaction();
        vmViewModel.SetTransactionalBool(true);
        vmViewModel.SetTransactionalInt(50);
        vmViewModel.SetTransactionalString(L"Test2");
        Assert::IsTrue(vmViewModel.TransactionIsModified());

        vmViewModel.BeginTransaction();
        Assert::IsFalse(vmViewModel.TransactionIsModified());

        vmViewModel.SetTransactionalBool(false);
        Assert::IsTrue(vmViewModel.TransactionIsModified());
    }
};

const StringModelProperty TransactionalViewModelBase_Tests::ViewModelHarness::StringProperty("ViewModelHarness", "String", L"");
const IntModelProperty TransactionalViewModelBase_Tests::ViewModelHarness::IntProperty("ViewModelHarness", "Int", 0);
const BoolModelProperty TransactionalViewModelBase_Tests::ViewModelHarness::BoolProperty("ViewModelHarness", "Bool", false);
const StringModelProperty TransactionalViewModelBase_Tests::ViewModelHarness::TransactionalStringProperty("ViewModelHarness", "TransactionalString", L"");
const IntModelProperty TransactionalViewModelBase_Tests::ViewModelHarness::TransactionalIntProperty("ViewModelHarness", "TransactionalInt", 0);
const BoolModelProperty TransactionalViewModelBase_Tests::ViewModelHarness::TransactionalBoolProperty("ViewModelHarness", "TransactionalBool", false);

} // namespace tests
} // namespace ui
} // namespace ra
