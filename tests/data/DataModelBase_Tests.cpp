#include "data\DataModelBase.hh"

#include "ra_fwd.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ra {
namespace data {
namespace tests {

TEST_CLASS(DataModelBase_Tests)
{
    class DataModelHarness : public DataModelBase
    {
    public:
        DataModelHarness() noexcept
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

    class NotifyTargetHarness : public DataModelBase::NotifyTarget
    {
    public:
        void Reset() noexcept
        {
            m_vChangedBools.clear();
            m_vChangedStrings.clear();
            m_vChangedInts.clear();
        }

        void OnDataModelBoolValueChanged(const BoolModelProperty::ChangeArgs& args) override
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

        void OnDataModelStringValueChanged(const StringModelProperty::ChangeArgs& args) override
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

        void OnDataModelIntValueChanged(const IntModelProperty::ChangeArgs& args) override
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
        DataModelHarness vmViewModel;
        Assert::IsFalse(vmViewModel.GetBool());
        Assert::IsFalse(vmViewModel.GetTransactionalBool());
        vmViewModel.BeginTransaction();

        NotifyTargetHarness oNotify;
        vmViewModel.AddNotifyTarget(oNotify);

        vmViewModel.SetBool(true);
        Assert::IsTrue(vmViewModel.GetBool());
        Assert::IsFalse(vmViewModel.GetTransactionalBool());
        Assert::IsFalse(vmViewModel.IsModified());
        Assert::IsFalse(vmViewModel.IsModified(DataModelHarness::BoolProperty));
        Assert::IsFalse(vmViewModel.IsModified(DataModelHarness::TransactionalBoolProperty));
        oNotify.AssertBoolChanged(DataModelHarness::BoolProperty, false, true);
        oNotify.AssertNotChanged(DataModelHarness::IsModifiedProperty);
        oNotify.Reset();

        vmViewModel.SetTransactionalBool(true);
        Assert::IsTrue(vmViewModel.GetBool());
        Assert::IsTrue(vmViewModel.GetTransactionalBool());
        Assert::IsTrue(vmViewModel.IsModified());
        Assert::IsFalse(vmViewModel.IsModified(DataModelHarness::BoolProperty));
        Assert::IsTrue(vmViewModel.IsModified(DataModelHarness::TransactionalBoolProperty));
        oNotify.AssertBoolChanged(DataModelHarness::TransactionalBoolProperty, false, true);
        oNotify.AssertBoolChanged(DataModelHarness::IsModifiedProperty, false, true);
        oNotify.Reset();

        vmViewModel.SetTransactionalBool(true);
        Assert::IsTrue(vmViewModel.GetBool());
        Assert::IsTrue(vmViewModel.GetTransactionalBool());
        Assert::IsTrue(vmViewModel.IsModified());
        Assert::IsFalse(vmViewModel.IsModified(DataModelHarness::BoolProperty));
        Assert::IsTrue(vmViewModel.IsModified(DataModelHarness::TransactionalBoolProperty));
        oNotify.AssertNotChanged(DataModelHarness::TransactionalBoolProperty);
        oNotify.AssertNotChanged(DataModelHarness::IsModifiedProperty);
        oNotify.Reset();

        vmViewModel.SetTransactionalBool(false);
        Assert::IsTrue(vmViewModel.GetBool());
        Assert::IsFalse(vmViewModel.GetTransactionalBool());
        Assert::IsFalse(vmViewModel.IsModified());
        Assert::IsFalse(vmViewModel.IsModified(DataModelHarness::BoolProperty));
        Assert::IsFalse(vmViewModel.IsModified(DataModelHarness::TransactionalBoolProperty));
        oNotify.AssertBoolChanged(DataModelHarness::TransactionalBoolProperty, true, false);
        oNotify.AssertBoolChanged(DataModelHarness::IsModifiedProperty, true, false);
    }

    TEST_METHOD(TestTransactionalStringProperty)
    {
        DataModelHarness vmViewModel;
        Assert::AreEqual(std::wstring(), vmViewModel.GetString());
        Assert::AreEqual(std::wstring(), vmViewModel.GetTransactionalString());
        vmViewModel.BeginTransaction();

        NotifyTargetHarness oNotify;
        vmViewModel.AddNotifyTarget(oNotify);

        vmViewModel.SetString(L"Test");
        Assert::AreEqual(std::wstring(L"Test"), vmViewModel.GetString());
        Assert::AreEqual(std::wstring(), vmViewModel.GetTransactionalString());
        Assert::IsFalse(vmViewModel.IsModified());
        Assert::IsFalse(vmViewModel.IsModified(DataModelHarness::StringProperty));
        Assert::IsFalse(vmViewModel.IsModified(DataModelHarness::TransactionalStringProperty));
        oNotify.AssertStringChanged(DataModelHarness::StringProperty, L"", L"Test");
        oNotify.AssertNotChanged(DataModelHarness::IsModifiedProperty);
        oNotify.Reset();

        vmViewModel.SetTransactionalString(L"Test");
        Assert::AreEqual(std::wstring(L"Test"), vmViewModel.GetString());
        Assert::AreEqual(std::wstring(L"Test"), vmViewModel.GetTransactionalString());
        Assert::IsTrue(vmViewModel.IsModified());
        Assert::IsFalse(vmViewModel.IsModified(DataModelHarness::StringProperty));
        Assert::IsTrue(vmViewModel.IsModified(DataModelHarness::TransactionalStringProperty));
        oNotify.AssertStringChanged(DataModelHarness::TransactionalStringProperty, L"", L"Test");
        oNotify.AssertBoolChanged(DataModelHarness::IsModifiedProperty, false, true);
        oNotify.Reset();

        vmViewModel.SetTransactionalString(L"Test");
        Assert::AreEqual(std::wstring(L"Test"), vmViewModel.GetString());
        Assert::AreEqual(std::wstring(L"Test"), vmViewModel.GetTransactionalString());
        Assert::IsTrue(vmViewModel.IsModified());
        Assert::IsFalse(vmViewModel.IsModified(DataModelHarness::StringProperty));
        Assert::IsTrue(vmViewModel.IsModified(DataModelHarness::TransactionalStringProperty));
        oNotify.AssertNotChanged(DataModelHarness::TransactionalStringProperty);
        oNotify.AssertNotChanged(DataModelHarness::IsModifiedProperty);
        oNotify.Reset();

        vmViewModel.SetTransactionalString(L"");
        Assert::AreEqual(std::wstring(L"Test"), vmViewModel.GetString());
        Assert::AreEqual(std::wstring(L""), vmViewModel.GetTransactionalString());
        Assert::IsFalse(vmViewModel.IsModified());
        Assert::IsFalse(vmViewModel.IsModified(DataModelHarness::StringProperty));
        Assert::IsFalse(vmViewModel.IsModified(DataModelHarness::TransactionalStringProperty));
        oNotify.AssertStringChanged(DataModelHarness::TransactionalStringProperty, L"Test", L"");
        oNotify.AssertBoolChanged(DataModelHarness::IsModifiedProperty, true, false);
    }

    TEST_METHOD(TestTransactionalIntProperty)
    {
        DataModelHarness vmViewModel;
        Assert::AreEqual(0, vmViewModel.GetInt());
        Assert::AreEqual(0, vmViewModel.GetTransactionalInt());
        vmViewModel.BeginTransaction();

        NotifyTargetHarness oNotify;
        vmViewModel.AddNotifyTarget(oNotify);

        vmViewModel.SetInt(99);
        Assert::AreEqual(99, vmViewModel.GetInt());
        Assert::AreEqual(0, vmViewModel.GetTransactionalInt());
        Assert::IsFalse(vmViewModel.IsModified());
        Assert::IsFalse(vmViewModel.IsModified(DataModelHarness::IntProperty));
        Assert::IsFalse(vmViewModel.IsModified(DataModelHarness::TransactionalIntProperty));
        oNotify.AssertIntChanged(DataModelHarness::IntProperty, 0, 99);
        oNotify.AssertNotChanged(DataModelHarness::IsModifiedProperty);
        oNotify.Reset();

        vmViewModel.SetTransactionalInt(99);
        Assert::AreEqual(99, vmViewModel.GetInt());
        Assert::AreEqual(99, vmViewModel.GetTransactionalInt());
        Assert::IsTrue(vmViewModel.IsModified());
        Assert::IsFalse(vmViewModel.IsModified(DataModelHarness::IntProperty));
        Assert::IsTrue(vmViewModel.IsModified(DataModelHarness::TransactionalIntProperty));
        oNotify.AssertIntChanged(DataModelHarness::TransactionalIntProperty, 0, 99);
        oNotify.AssertBoolChanged(DataModelHarness::IsModifiedProperty, false, true);
        oNotify.Reset();

        vmViewModel.SetTransactionalInt(99);
        Assert::AreEqual(99, vmViewModel.GetInt());
        Assert::AreEqual(99, vmViewModel.GetTransactionalInt());
        Assert::IsTrue(vmViewModel.IsModified());
        Assert::IsFalse(vmViewModel.IsModified(DataModelHarness::IntProperty));
        Assert::IsTrue(vmViewModel.IsModified(DataModelHarness::TransactionalIntProperty));
        oNotify.AssertNotChanged(DataModelHarness::TransactionalIntProperty);
        oNotify.AssertNotChanged(DataModelHarness::IsModifiedProperty);
        oNotify.Reset();

        vmViewModel.SetTransactionalInt(0);
        Assert::AreEqual(99, vmViewModel.GetInt());
        Assert::AreEqual(0, vmViewModel.GetTransactionalInt());
        Assert::IsFalse(vmViewModel.IsModified());
        Assert::IsFalse(vmViewModel.IsModified(DataModelHarness::IntProperty));
        Assert::IsFalse(vmViewModel.IsModified(DataModelHarness::TransactionalIntProperty));
        oNotify.AssertIntChanged(DataModelHarness::TransactionalIntProperty, 99, 0);
        oNotify.AssertBoolChanged(DataModelHarness::IsModifiedProperty, true, false);
    }

    TEST_METHOD(TestRevertTransaction)
    {
        DataModelHarness vmViewModel;
        NotifyTargetHarness oNotify;
        vmViewModel.AddNotifyTarget(oNotify);
        vmViewModel.BeginTransaction();

        Assert::IsFalse(vmViewModel.IsModified());
        Assert::IsFalse(vmViewModel.IsModified(DataModelHarness::TransactionalIntProperty));
        Assert::IsFalse(vmViewModel.IsModified(DataModelHarness::TransactionalStringProperty));
        Assert::IsFalse(vmViewModel.IsModified(DataModelHarness::TransactionalBoolProperty));

        vmViewModel.SetTransactionalInt(99);
        vmViewModel.SetTransactionalString(L"Test");
        Assert::IsTrue(vmViewModel.IsModified());
        Assert::IsTrue(vmViewModel.IsModified(DataModelHarness::TransactionalIntProperty));
        Assert::IsTrue(vmViewModel.IsModified(DataModelHarness::TransactionalStringProperty));
        Assert::IsFalse(vmViewModel.IsModified(DataModelHarness::TransactionalBoolProperty));
        oNotify.Reset();

        vmViewModel.RevertTransaction();
        Assert::IsFalse(vmViewModel.IsModified());
        Assert::IsFalse(vmViewModel.IsModified(DataModelHarness::TransactionalIntProperty));
        Assert::IsFalse(vmViewModel.IsModified(DataModelHarness::TransactionalStringProperty));
        Assert::IsFalse(vmViewModel.IsModified(DataModelHarness::TransactionalBoolProperty));

        Assert::AreEqual(0, vmViewModel.GetTransactionalInt());
        Assert::AreEqual(std::wstring(L""), vmViewModel.GetTransactionalString());
        Assert::AreEqual(false, vmViewModel.GetTransactionalBool());

        // no transaction to revert - modifications should be kept, but not tracked
        vmViewModel.SetTransactionalInt(99);
        vmViewModel.SetTransactionalString(L"Test");
        vmViewModel.RevertTransaction();
        Assert::IsFalse(vmViewModel.IsModified());
        Assert::IsFalse(vmViewModel.IsModified(DataModelHarness::TransactionalIntProperty));
        Assert::IsFalse(vmViewModel.IsModified(DataModelHarness::TransactionalStringProperty));
        Assert::IsFalse(vmViewModel.IsModified(DataModelHarness::TransactionalBoolProperty));

        Assert::AreEqual(99, vmViewModel.GetTransactionalInt());
        Assert::AreEqual(std::wstring(L"Test"), vmViewModel.GetTransactionalString());
        Assert::AreEqual(false, vmViewModel.GetTransactionalBool());
    }

    TEST_METHOD(TestRevertTransactionNested)
    {
        DataModelHarness vmViewModel;
        NotifyTargetHarness oNotify;
        vmViewModel.AddNotifyTarget(oNotify);

        // create a transaction and modify it
        vmViewModel.BeginTransaction();
        vmViewModel.SetTransactionalInt(99);
        vmViewModel.SetTransactionalString(L"Test");
        Assert::IsTrue(vmViewModel.IsModified());
        Assert::IsTrue(vmViewModel.IsModified(DataModelHarness::TransactionalIntProperty));
        Assert::IsTrue(vmViewModel.IsModified(DataModelHarness::TransactionalStringProperty));
        Assert::IsFalse(vmViewModel.IsModified(DataModelHarness::TransactionalBoolProperty));
        oNotify.Reset();

        // nested transaction is not initially considered modified
        vmViewModel.BeginTransaction();
        Assert::IsFalse(vmViewModel.IsModified());
        Assert::IsFalse(vmViewModel.IsModified(DataModelHarness::TransactionalIntProperty));
        Assert::IsFalse(vmViewModel.IsModified(DataModelHarness::TransactionalStringProperty));
        Assert::IsFalse(vmViewModel.IsModified(DataModelHarness::TransactionalBoolProperty));

        oNotify.AssertNotChanged(DataModelHarness::TransactionalIntProperty);
        oNotify.AssertNotChanged(DataModelHarness::TransactionalStringProperty);
        oNotify.AssertNotChanged(DataModelHarness::TransactionalBoolProperty);
        oNotify.AssertBoolChanged(DataModelHarness::IsModifiedProperty, true, false);
        oNotify.Reset();

        Assert::AreEqual(99, vmViewModel.GetTransactionalInt());
        Assert::AreEqual(std::wstring(L"Test"), vmViewModel.GetTransactionalString());
        Assert::AreEqual(false, vmViewModel.GetTransactionalBool());

        // revert nested transaction, will pick up modifications from first transaction
        vmViewModel.RevertTransaction();
        Assert::IsTrue(vmViewModel.IsModified());
        Assert::IsTrue(vmViewModel.IsModified(DataModelHarness::TransactionalIntProperty));
        Assert::IsTrue(vmViewModel.IsModified(DataModelHarness::TransactionalStringProperty));
        Assert::IsFalse(vmViewModel.IsModified(DataModelHarness::TransactionalBoolProperty));

        oNotify.AssertNotChanged(DataModelHarness::TransactionalIntProperty);
        oNotify.AssertNotChanged(DataModelHarness::TransactionalStringProperty);
        oNotify.AssertNotChanged(DataModelHarness::TransactionalBoolProperty);
        oNotify.AssertBoolChanged(DataModelHarness::IsModifiedProperty, false, true);
        oNotify.Reset();

        // another nested transacion, this time, we'll modify it
        vmViewModel.BeginTransaction();
        Assert::AreEqual(99, vmViewModel.GetTransactionalInt());
        Assert::AreEqual(std::wstring(L"Test"), vmViewModel.GetTransactionalString());
        Assert::AreEqual(false, vmViewModel.GetTransactionalBool());

        vmViewModel.SetTransactionalInt(50);
        Assert::IsTrue(vmViewModel.IsModified());
        Assert::IsTrue(vmViewModel.IsModified(DataModelHarness::TransactionalIntProperty));
        Assert::IsFalse(vmViewModel.IsModified(DataModelHarness::TransactionalStringProperty));
        Assert::IsFalse(vmViewModel.IsModified(DataModelHarness::TransactionalBoolProperty));
        oNotify.Reset();

        // revert nested transaction, will pick up modifications from first transaction
        vmViewModel.RevertTransaction();
        Assert::IsTrue(vmViewModel.IsModified());
        Assert::IsTrue(vmViewModel.IsModified(DataModelHarness::TransactionalIntProperty));
        Assert::IsTrue(vmViewModel.IsModified(DataModelHarness::TransactionalStringProperty));
        Assert::IsFalse(vmViewModel.IsModified(DataModelHarness::TransactionalBoolProperty));

        oNotify.AssertIntChanged(DataModelHarness::TransactionalIntProperty, 50, 99);
        oNotify.AssertNotChanged(DataModelHarness::TransactionalStringProperty);
        oNotify.AssertNotChanged(DataModelHarness::TransactionalBoolProperty);
        oNotify.AssertNotChanged(DataModelHarness::IsModifiedProperty);
        oNotify.Reset();

        Assert::AreEqual(99, vmViewModel.GetTransactionalInt());
        Assert::AreEqual(std::wstring(L"Test"), vmViewModel.GetTransactionalString());
        Assert::AreEqual(false, vmViewModel.GetTransactionalBool());

        // revert primary transaction
        vmViewModel.RevertTransaction();
        Assert::IsFalse(vmViewModel.IsModified());
        Assert::IsFalse(vmViewModel.IsModified(DataModelHarness::TransactionalIntProperty));
        Assert::IsFalse(vmViewModel.IsModified(DataModelHarness::TransactionalStringProperty));
        Assert::IsFalse(vmViewModel.IsModified(DataModelHarness::TransactionalBoolProperty));

        oNotify.AssertIntChanged(DataModelHarness::TransactionalIntProperty, 99, 0);
        oNotify.AssertStringChanged(DataModelHarness::TransactionalStringProperty, L"Test", L"");
        oNotify.AssertNotChanged(DataModelHarness::TransactionalBoolProperty);
        oNotify.AssertBoolChanged(DataModelHarness::IsModifiedProperty, true, false);

        Assert::AreEqual(0, vmViewModel.GetTransactionalInt());
        Assert::AreEqual(std::wstring(L""), vmViewModel.GetTransactionalString());
        Assert::AreEqual(false, vmViewModel.GetTransactionalBool());
    }

    TEST_METHOD(TestRevertTransactionRepeated)
    {
        DataModelHarness vmViewModel;
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
        DataModelHarness vmViewModel;
        NotifyTargetHarness oNotify;
        vmViewModel.AddNotifyTarget(oNotify);

        vmViewModel.SetTransactionalInt(99);
        vmViewModel.SetTransactionalString(L"Test");
        vmViewModel.BeginTransaction();
        oNotify.Reset();

        Assert::IsFalse(vmViewModel.IsModified());
        Assert::IsFalse(vmViewModel.IsModified(DataModelHarness::TransactionalIntProperty));
        Assert::IsFalse(vmViewModel.IsModified(DataModelHarness::TransactionalStringProperty));
        Assert::IsFalse(vmViewModel.IsModified(DataModelHarness::TransactionalBoolProperty));

        vmViewModel.SetTransactionalInt(50);
        vmViewModel.SetTransactionalString(L"Test2");
        Assert::IsTrue(vmViewModel.IsModified());
        Assert::IsTrue(vmViewModel.IsModified(DataModelHarness::TransactionalIntProperty));
        Assert::IsTrue(vmViewModel.IsModified(DataModelHarness::TransactionalStringProperty));
        Assert::IsFalse(vmViewModel.IsModified(DataModelHarness::TransactionalBoolProperty));
        oNotify.AssertStringChanged(DataModelHarness::TransactionalStringProperty, L"Test", L"Test2");
        oNotify.AssertIntChanged(DataModelHarness::TransactionalIntProperty, 99, 50);
        oNotify.AssertBoolChanged(DataModelHarness::IsModifiedProperty, false, true);
        oNotify.Reset();

        vmViewModel.SetTransactionalInt(75);
        vmViewModel.SetTransactionalString(L"Test3");
        Assert::IsTrue(vmViewModel.IsModified());
        Assert::IsTrue(vmViewModel.IsModified(DataModelHarness::TransactionalIntProperty));
        Assert::IsTrue(vmViewModel.IsModified(DataModelHarness::TransactionalStringProperty));
        Assert::IsFalse(vmViewModel.IsModified(DataModelHarness::TransactionalBoolProperty));
        oNotify.AssertStringChanged(DataModelHarness::TransactionalStringProperty, L"Test2", L"Test3");
        oNotify.AssertIntChanged(DataModelHarness::TransactionalIntProperty, 50, 75);
        oNotify.AssertNotChanged(DataModelHarness::IsModifiedProperty);
        oNotify.Reset();

        // revert should return to original state, regardless of how many times values have changed within the transaction
        vmViewModel.RevertTransaction();
        Assert::IsFalse(vmViewModel.IsModified());
        Assert::IsFalse(vmViewModel.IsModified(DataModelHarness::TransactionalIntProperty));
        Assert::IsFalse(vmViewModel.IsModified(DataModelHarness::TransactionalStringProperty));
        Assert::IsFalse(vmViewModel.IsModified(DataModelHarness::TransactionalBoolProperty));
        oNotify.AssertStringChanged(DataModelHarness::TransactionalStringProperty, L"Test3", L"Test");
        oNotify.AssertIntChanged(DataModelHarness::TransactionalIntProperty, 75, 99);
        oNotify.AssertBoolChanged(DataModelHarness::IsModifiedProperty, true, false);
    }

    TEST_METHOD(TestCommitTransaction)
    {
        DataModelHarness vmViewModel;
        NotifyTargetHarness oNotify;
        vmViewModel.AddNotifyTarget(oNotify);
        vmViewModel.BeginTransaction();

        Assert::IsFalse(vmViewModel.IsModified());
        Assert::IsFalse(vmViewModel.IsModified(DataModelHarness::TransactionalIntProperty));
        Assert::IsFalse(vmViewModel.IsModified(DataModelHarness::TransactionalStringProperty));
        Assert::IsFalse(vmViewModel.IsModified(DataModelHarness::TransactionalBoolProperty));

        vmViewModel.SetTransactionalInt(99);
        vmViewModel.SetTransactionalString(L"Test");
        Assert::IsTrue(vmViewModel.IsModified());
        Assert::IsTrue(vmViewModel.IsModified(DataModelHarness::TransactionalIntProperty));
        Assert::IsTrue(vmViewModel.IsModified(DataModelHarness::TransactionalStringProperty));
        Assert::IsFalse(vmViewModel.IsModified(DataModelHarness::TransactionalBoolProperty));
        oNotify.Reset();

        vmViewModel.CommitTransaction();
        Assert::IsFalse(vmViewModel.IsModified());
        Assert::IsFalse(vmViewModel.IsModified(DataModelHarness::TransactionalIntProperty));
        Assert::IsFalse(vmViewModel.IsModified(DataModelHarness::TransactionalStringProperty));
        Assert::IsFalse(vmViewModel.IsModified(DataModelHarness::TransactionalBoolProperty));

        Assert::AreEqual(99, vmViewModel.GetTransactionalInt());
        Assert::AreEqual(std::wstring(L"Test"), vmViewModel.GetTransactionalString());
        Assert::AreEqual(false, vmViewModel.GetTransactionalBool());

        // no transaction to commit - modifications should be kept, but not tracked
        vmViewModel.SetTransactionalInt(50);
        vmViewModel.SetTransactionalString(L"Test2");
        vmViewModel.RevertTransaction();
        Assert::IsFalse(vmViewModel.IsModified());
        Assert::IsFalse(vmViewModel.IsModified(DataModelHarness::TransactionalIntProperty));
        Assert::IsFalse(vmViewModel.IsModified(DataModelHarness::TransactionalStringProperty));
        Assert::IsFalse(vmViewModel.IsModified(DataModelHarness::TransactionalBoolProperty));

        Assert::AreEqual(50, vmViewModel.GetTransactionalInt());
        Assert::AreEqual(std::wstring(L"Test2"), vmViewModel.GetTransactionalString());
        Assert::AreEqual(false, vmViewModel.GetTransactionalBool());
    }

    TEST_METHOD(TestCommitTransactionNested)
    {
        DataModelHarness vmViewModel;
        NotifyTargetHarness oNotify;
        vmViewModel.AddNotifyTarget(oNotify);

        // create a transaction and modify it
        vmViewModel.BeginTransaction();
        vmViewModel.SetTransactionalInt(99);
        vmViewModel.SetTransactionalString(L"Test");
        Assert::IsTrue(vmViewModel.IsModified());
        Assert::IsTrue(vmViewModel.IsModified(DataModelHarness::TransactionalIntProperty));
        Assert::IsTrue(vmViewModel.IsModified(DataModelHarness::TransactionalStringProperty));
        Assert::IsFalse(vmViewModel.IsModified(DataModelHarness::TransactionalBoolProperty));
        oNotify.Reset();

        // nested transaction is not initially considered modified
        vmViewModel.BeginTransaction();
        Assert::IsFalse(vmViewModel.IsModified());
        Assert::IsFalse(vmViewModel.IsModified(DataModelHarness::TransactionalIntProperty));
        Assert::IsFalse(vmViewModel.IsModified(DataModelHarness::TransactionalStringProperty));
        Assert::IsFalse(vmViewModel.IsModified(DataModelHarness::TransactionalBoolProperty));

        oNotify.AssertNotChanged(DataModelHarness::TransactionalIntProperty);
        oNotify.AssertNotChanged(DataModelHarness::TransactionalStringProperty);
        oNotify.AssertNotChanged(DataModelHarness::TransactionalBoolProperty);
        oNotify.AssertBoolChanged(DataModelHarness::IsModifiedProperty, true, false);
        oNotify.Reset();

        Assert::AreEqual(99, vmViewModel.GetTransactionalInt());
        Assert::AreEqual(std::wstring(L"Test"), vmViewModel.GetTransactionalString());
        Assert::AreEqual(false, vmViewModel.GetTransactionalBool());

        // commit (unmodified) nested transaction, will pick up modifications from first transaction
        vmViewModel.CommitTransaction();
        Assert::IsTrue(vmViewModel.IsModified());
        Assert::IsTrue(vmViewModel.IsModified(DataModelHarness::TransactionalIntProperty));
        Assert::IsTrue(vmViewModel.IsModified(DataModelHarness::TransactionalStringProperty));
        Assert::IsFalse(vmViewModel.IsModified(DataModelHarness::TransactionalBoolProperty));

        oNotify.AssertNotChanged(DataModelHarness::TransactionalIntProperty);
        oNotify.AssertNotChanged(DataModelHarness::TransactionalStringProperty);
        oNotify.AssertNotChanged(DataModelHarness::TransactionalBoolProperty);
        oNotify.AssertBoolChanged(DataModelHarness::IsModifiedProperty, false, true);
        oNotify.Reset();

        // another nested transacion, this time, we'll modify it
        vmViewModel.BeginTransaction();
        Assert::AreEqual(99, vmViewModel.GetTransactionalInt());
        Assert::AreEqual(std::wstring(L"Test"), vmViewModel.GetTransactionalString());
        Assert::AreEqual(false, vmViewModel.GetTransactionalBool());

        vmViewModel.SetTransactionalInt(50);
        Assert::IsTrue(vmViewModel.IsModified());
        Assert::IsTrue(vmViewModel.IsModified(DataModelHarness::TransactionalIntProperty));
        Assert::IsFalse(vmViewModel.IsModified(DataModelHarness::TransactionalStringProperty));
        Assert::IsFalse(vmViewModel.IsModified(DataModelHarness::TransactionalBoolProperty));
        oNotify.Reset();

        // revert nested transaction, will pick up modifications from first transaction
        vmViewModel.CommitTransaction();
        Assert::IsTrue(vmViewModel.IsModified());
        Assert::IsTrue(vmViewModel.IsModified(DataModelHarness::TransactionalIntProperty));
        Assert::IsTrue(vmViewModel.IsModified(DataModelHarness::TransactionalStringProperty));
        Assert::IsFalse(vmViewModel.IsModified(DataModelHarness::TransactionalBoolProperty));

        oNotify.AssertNotChanged(DataModelHarness::TransactionalIntProperty);
        oNotify.AssertNotChanged(DataModelHarness::TransactionalStringProperty);
        oNotify.AssertNotChanged(DataModelHarness::TransactionalBoolProperty);
        oNotify.AssertNotChanged(DataModelHarness::IsModifiedProperty);
        oNotify.Reset();

        Assert::AreEqual(50, vmViewModel.GetTransactionalInt());
        Assert::AreEqual(std::wstring(L"Test"), vmViewModel.GetTransactionalString());
        Assert::AreEqual(false, vmViewModel.GetTransactionalBool());

        // commit primary transaction
        vmViewModel.CommitTransaction();
        Assert::IsFalse(vmViewModel.IsModified());
        Assert::IsFalse(vmViewModel.IsModified(DataModelHarness::TransactionalIntProperty));
        Assert::IsFalse(vmViewModel.IsModified(DataModelHarness::TransactionalStringProperty));
        Assert::IsFalse(vmViewModel.IsModified(DataModelHarness::TransactionalBoolProperty));

        oNotify.AssertNotChanged(DataModelHarness::TransactionalIntProperty);
        oNotify.AssertNotChanged(DataModelHarness::TransactionalStringProperty);
        oNotify.AssertNotChanged(DataModelHarness::TransactionalBoolProperty);
        oNotify.AssertBoolChanged(DataModelHarness::IsModifiedProperty, true, false);

        Assert::AreEqual(50, vmViewModel.GetTransactionalInt());
        Assert::AreEqual(std::wstring(L"Test"), vmViewModel.GetTransactionalString());
        Assert::AreEqual(false, vmViewModel.GetTransactionalBool());
    }

    TEST_METHOD(TestCommitTransactionNestedOriginalValues)
    {
        DataModelHarness vmViewModel;
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
        Assert::IsTrue(vmViewModel.IsModified(DataModelHarness::TransactionalIntProperty));
        Assert::IsTrue(vmViewModel.IsModified(DataModelHarness::TransactionalStringProperty));
        Assert::IsFalse(vmViewModel.IsModified(DataModelHarness::TransactionalBoolProperty));

        oNotify.AssertIntChanged(DataModelHarness::TransactionalIntProperty, 50, 99);
        oNotify.AssertStringChanged(DataModelHarness::TransactionalStringProperty, L"Test2", L"Test");
        oNotify.AssertNotChanged(DataModelHarness::TransactionalBoolProperty);
        oNotify.Reset();

        Assert::AreEqual(99, vmViewModel.GetTransactionalInt());
        Assert::AreEqual(std::wstring(L"Test"), vmViewModel.GetTransactionalString());
        Assert::AreEqual(false, vmViewModel.GetTransactionalBool());

        // commit nested transaction, since the values match the original values, the original transaction will appear unmodified
        vmViewModel.CommitTransaction();
        Assert::IsFalse(vmViewModel.IsModified());
        Assert::IsFalse(vmViewModel.IsModified(DataModelHarness::TransactionalIntProperty));
        Assert::IsFalse(vmViewModel.IsModified(DataModelHarness::TransactionalStringProperty));
        Assert::IsFalse(vmViewModel.IsModified(DataModelHarness::TransactionalBoolProperty));

        oNotify.AssertNotChanged(DataModelHarness::TransactionalIntProperty);
        oNotify.AssertNotChanged(DataModelHarness::TransactionalStringProperty);
        oNotify.AssertNotChanged(DataModelHarness::TransactionalBoolProperty);
        oNotify.AssertBoolChanged(DataModelHarness::IsModifiedProperty, true, false);
        oNotify.Reset();

        Assert::AreEqual(99, vmViewModel.GetTransactionalInt());
        Assert::AreEqual(std::wstring(L"Test"), vmViewModel.GetTransactionalString());
        Assert::AreEqual(false, vmViewModel.GetTransactionalBool());

        // commiting the now-unmodified original transaction should have no effect
        vmViewModel.CommitTransaction();
        Assert::IsFalse(vmViewModel.IsModified());
        Assert::IsFalse(vmViewModel.IsModified(DataModelHarness::TransactionalIntProperty));
        Assert::IsFalse(vmViewModel.IsModified(DataModelHarness::TransactionalStringProperty));
        Assert::IsFalse(vmViewModel.IsModified(DataModelHarness::TransactionalBoolProperty));

        oNotify.AssertNotChanged(DataModelHarness::TransactionalIntProperty);
        oNotify.AssertNotChanged(DataModelHarness::TransactionalStringProperty);
        oNotify.AssertNotChanged(DataModelHarness::TransactionalBoolProperty);
        oNotify.AssertNotChanged(DataModelHarness::IsModifiedProperty);

        Assert::AreEqual(99, vmViewModel.GetTransactionalInt());
        Assert::AreEqual(std::wstring(L"Test"), vmViewModel.GetTransactionalString());
        Assert::AreEqual(false, vmViewModel.GetTransactionalBool());
    }

    TEST_METHOD(TestPreviousValues)
    {
        DataModelHarness vmViewModel;
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
        DataModelHarness vmViewModel;
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

const StringModelProperty DataModelBase_Tests::DataModelHarness::StringProperty("DataModelHarness", "String", L"");
const IntModelProperty DataModelBase_Tests::DataModelHarness::IntProperty("DataModelHarness", "Int", 0);
const BoolModelProperty DataModelBase_Tests::DataModelHarness::BoolProperty("DataModelHarness", "Bool", false);
const StringModelProperty DataModelBase_Tests::DataModelHarness::TransactionalStringProperty("DataModelHarness", "TransactionalString", L"");
const IntModelProperty DataModelBase_Tests::DataModelHarness::TransactionalIntProperty("DataModelHarness", "TransactionalInt", 0);
const BoolModelProperty DataModelBase_Tests::DataModelHarness::TransactionalBoolProperty("DataModelHarness", "TransactionalBool", false);

} // namespace tests
} // namespace ui
} // namespace ra
