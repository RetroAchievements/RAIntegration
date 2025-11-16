#include "data\DataModelBase.hh"

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

        using DataModelBase::BeginUpdate;
        using DataModelBase::EndUpdate;
        using DataModelBase::IsUpdating;
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
        DataModelHarness pModel;
        Assert::IsFalse(pModel.GetBool());
        Assert::IsFalse(pModel.GetTransactionalBool());
        pModel.BeginTransaction();

        NotifyTargetHarness oNotify;
        pModel.AddNotifyTarget(oNotify);

        pModel.SetBool(true);
        Assert::IsTrue(pModel.GetBool());
        Assert::IsFalse(pModel.GetTransactionalBool());
        Assert::IsFalse(pModel.IsModified());
        Assert::IsFalse(pModel.IsModified(DataModelHarness::BoolProperty));
        Assert::IsFalse(pModel.IsModified(DataModelHarness::TransactionalBoolProperty));
        oNotify.AssertBoolChanged(DataModelHarness::BoolProperty, false, true);
        oNotify.AssertNotChanged(DataModelHarness::IsModifiedProperty);
        oNotify.Reset();

        pModel.SetTransactionalBool(true);
        Assert::IsTrue(pModel.GetBool());
        Assert::IsTrue(pModel.GetTransactionalBool());
        Assert::IsTrue(pModel.IsModified());
        Assert::IsFalse(pModel.IsModified(DataModelHarness::BoolProperty));
        Assert::IsTrue(pModel.IsModified(DataModelHarness::TransactionalBoolProperty));
        oNotify.AssertBoolChanged(DataModelHarness::TransactionalBoolProperty, false, true);
        oNotify.AssertBoolChanged(DataModelHarness::IsModifiedProperty, false, true);
        oNotify.Reset();

        pModel.SetTransactionalBool(true);
        Assert::IsTrue(pModel.GetBool());
        Assert::IsTrue(pModel.GetTransactionalBool());
        Assert::IsTrue(pModel.IsModified());
        Assert::IsFalse(pModel.IsModified(DataModelHarness::BoolProperty));
        Assert::IsTrue(pModel.IsModified(DataModelHarness::TransactionalBoolProperty));
        oNotify.AssertNotChanged(DataModelHarness::TransactionalBoolProperty);
        oNotify.AssertNotChanged(DataModelHarness::IsModifiedProperty);
        oNotify.Reset();

        pModel.SetTransactionalBool(false);
        Assert::IsTrue(pModel.GetBool());
        Assert::IsFalse(pModel.GetTransactionalBool());
        Assert::IsFalse(pModel.IsModified());
        Assert::IsFalse(pModel.IsModified(DataModelHarness::BoolProperty));
        Assert::IsFalse(pModel.IsModified(DataModelHarness::TransactionalBoolProperty));
        oNotify.AssertBoolChanged(DataModelHarness::TransactionalBoolProperty, true, false);
        oNotify.AssertBoolChanged(DataModelHarness::IsModifiedProperty, true, false);
    }

    TEST_METHOD(TestTransactionalStringProperty)
    {
        DataModelHarness pModel;
        Assert::AreEqual(std::wstring(), pModel.GetString());
        Assert::AreEqual(std::wstring(), pModel.GetTransactionalString());
        pModel.BeginTransaction();

        NotifyTargetHarness oNotify;
        pModel.AddNotifyTarget(oNotify);

        pModel.SetString(L"Test");
        Assert::AreEqual(std::wstring(L"Test"), pModel.GetString());
        Assert::AreEqual(std::wstring(), pModel.GetTransactionalString());
        Assert::IsFalse(pModel.IsModified());
        Assert::IsFalse(pModel.IsModified(DataModelHarness::StringProperty));
        Assert::IsFalse(pModel.IsModified(DataModelHarness::TransactionalStringProperty));
        oNotify.AssertStringChanged(DataModelHarness::StringProperty, L"", L"Test");
        oNotify.AssertNotChanged(DataModelHarness::IsModifiedProperty);
        oNotify.Reset();

        pModel.SetTransactionalString(L"Test");
        Assert::AreEqual(std::wstring(L"Test"), pModel.GetString());
        Assert::AreEqual(std::wstring(L"Test"), pModel.GetTransactionalString());
        Assert::IsTrue(pModel.IsModified());
        Assert::IsFalse(pModel.IsModified(DataModelHarness::StringProperty));
        Assert::IsTrue(pModel.IsModified(DataModelHarness::TransactionalStringProperty));
        oNotify.AssertStringChanged(DataModelHarness::TransactionalStringProperty, L"", L"Test");
        oNotify.AssertBoolChanged(DataModelHarness::IsModifiedProperty, false, true);
        oNotify.Reset();

        pModel.SetTransactionalString(L"Test");
        Assert::AreEqual(std::wstring(L"Test"), pModel.GetString());
        Assert::AreEqual(std::wstring(L"Test"), pModel.GetTransactionalString());
        Assert::IsTrue(pModel.IsModified());
        Assert::IsFalse(pModel.IsModified(DataModelHarness::StringProperty));
        Assert::IsTrue(pModel.IsModified(DataModelHarness::TransactionalStringProperty));
        oNotify.AssertNotChanged(DataModelHarness::TransactionalStringProperty);
        oNotify.AssertNotChanged(DataModelHarness::IsModifiedProperty);
        oNotify.Reset();

        pModel.SetTransactionalString(L"");
        Assert::AreEqual(std::wstring(L"Test"), pModel.GetString());
        Assert::AreEqual(std::wstring(L""), pModel.GetTransactionalString());
        Assert::IsFalse(pModel.IsModified());
        Assert::IsFalse(pModel.IsModified(DataModelHarness::StringProperty));
        Assert::IsFalse(pModel.IsModified(DataModelHarness::TransactionalStringProperty));
        oNotify.AssertStringChanged(DataModelHarness::TransactionalStringProperty, L"Test", L"");
        oNotify.AssertBoolChanged(DataModelHarness::IsModifiedProperty, true, false);
    }

    TEST_METHOD(TestTransactionalIntProperty)
    {
        DataModelHarness pModel;
        Assert::AreEqual(0, pModel.GetInt());
        Assert::AreEqual(0, pModel.GetTransactionalInt());
        pModel.BeginTransaction();

        NotifyTargetHarness oNotify;
        pModel.AddNotifyTarget(oNotify);

        pModel.SetInt(99);
        Assert::AreEqual(99, pModel.GetInt());
        Assert::AreEqual(0, pModel.GetTransactionalInt());
        Assert::IsFalse(pModel.IsModified());
        Assert::IsFalse(pModel.IsModified(DataModelHarness::IntProperty));
        Assert::IsFalse(pModel.IsModified(DataModelHarness::TransactionalIntProperty));
        oNotify.AssertIntChanged(DataModelHarness::IntProperty, 0, 99);
        oNotify.AssertNotChanged(DataModelHarness::IsModifiedProperty);
        oNotify.Reset();

        pModel.SetTransactionalInt(99);
        Assert::AreEqual(99, pModel.GetInt());
        Assert::AreEqual(99, pModel.GetTransactionalInt());
        Assert::IsTrue(pModel.IsModified());
        Assert::IsFalse(pModel.IsModified(DataModelHarness::IntProperty));
        Assert::IsTrue(pModel.IsModified(DataModelHarness::TransactionalIntProperty));
        oNotify.AssertIntChanged(DataModelHarness::TransactionalIntProperty, 0, 99);
        oNotify.AssertBoolChanged(DataModelHarness::IsModifiedProperty, false, true);
        oNotify.Reset();

        pModel.SetTransactionalInt(99);
        Assert::AreEqual(99, pModel.GetInt());
        Assert::AreEqual(99, pModel.GetTransactionalInt());
        Assert::IsTrue(pModel.IsModified());
        Assert::IsFalse(pModel.IsModified(DataModelHarness::IntProperty));
        Assert::IsTrue(pModel.IsModified(DataModelHarness::TransactionalIntProperty));
        oNotify.AssertNotChanged(DataModelHarness::TransactionalIntProperty);
        oNotify.AssertNotChanged(DataModelHarness::IsModifiedProperty);
        oNotify.Reset();

        pModel.SetTransactionalInt(0);
        Assert::AreEqual(99, pModel.GetInt());
        Assert::AreEqual(0, pModel.GetTransactionalInt());
        Assert::IsFalse(pModel.IsModified());
        Assert::IsFalse(pModel.IsModified(DataModelHarness::IntProperty));
        Assert::IsFalse(pModel.IsModified(DataModelHarness::TransactionalIntProperty));
        oNotify.AssertIntChanged(DataModelHarness::TransactionalIntProperty, 99, 0);
        oNotify.AssertBoolChanged(DataModelHarness::IsModifiedProperty, true, false);
    }

    TEST_METHOD(TestRevertTransaction)
    {
        DataModelHarness pModel;
        NotifyTargetHarness oNotify;
        pModel.AddNotifyTarget(oNotify);
        pModel.BeginTransaction();

        Assert::IsFalse(pModel.IsModified());
        Assert::IsFalse(pModel.IsModified(DataModelHarness::TransactionalIntProperty));
        Assert::IsFalse(pModel.IsModified(DataModelHarness::TransactionalStringProperty));
        Assert::IsFalse(pModel.IsModified(DataModelHarness::TransactionalBoolProperty));

        pModel.SetTransactionalInt(99);
        pModel.SetTransactionalString(L"Test");
        Assert::IsTrue(pModel.IsModified());
        Assert::IsTrue(pModel.IsModified(DataModelHarness::TransactionalIntProperty));
        Assert::IsTrue(pModel.IsModified(DataModelHarness::TransactionalStringProperty));
        Assert::IsFalse(pModel.IsModified(DataModelHarness::TransactionalBoolProperty));
        oNotify.Reset();

        pModel.RevertTransaction();
        Assert::IsFalse(pModel.IsModified());
        Assert::IsFalse(pModel.IsModified(DataModelHarness::TransactionalIntProperty));
        Assert::IsFalse(pModel.IsModified(DataModelHarness::TransactionalStringProperty));
        Assert::IsFalse(pModel.IsModified(DataModelHarness::TransactionalBoolProperty));

        Assert::AreEqual(0, pModel.GetTransactionalInt());
        Assert::AreEqual(std::wstring(L""), pModel.GetTransactionalString());
        Assert::AreEqual(false, pModel.GetTransactionalBool());

        // no transaction to revert - modifications should be kept, but not tracked
        pModel.SetTransactionalInt(99);
        pModel.SetTransactionalString(L"Test");
        pModel.RevertTransaction();
        Assert::IsFalse(pModel.IsModified());
        Assert::IsFalse(pModel.IsModified(DataModelHarness::TransactionalIntProperty));
        Assert::IsFalse(pModel.IsModified(DataModelHarness::TransactionalStringProperty));
        Assert::IsFalse(pModel.IsModified(DataModelHarness::TransactionalBoolProperty));

        Assert::AreEqual(99, pModel.GetTransactionalInt());
        Assert::AreEqual(std::wstring(L"Test"), pModel.GetTransactionalString());
        Assert::AreEqual(false, pModel.GetTransactionalBool());
    }

    TEST_METHOD(TestRevertTransactionNested)
    {
        DataModelHarness pModel;
        NotifyTargetHarness oNotify;
        pModel.AddNotifyTarget(oNotify);

        // create a transaction and modify it
        pModel.BeginTransaction();
        pModel.SetTransactionalInt(99);
        pModel.SetTransactionalString(L"Test");
        Assert::IsTrue(pModel.IsModified());
        Assert::IsTrue(pModel.IsModified(DataModelHarness::TransactionalIntProperty));
        Assert::IsTrue(pModel.IsModified(DataModelHarness::TransactionalStringProperty));
        Assert::IsFalse(pModel.IsModified(DataModelHarness::TransactionalBoolProperty));
        oNotify.Reset();

        // nested transaction is not initially considered modified
        pModel.BeginTransaction();
        Assert::IsFalse(pModel.IsModified());
        Assert::IsFalse(pModel.IsModified(DataModelHarness::TransactionalIntProperty));
        Assert::IsFalse(pModel.IsModified(DataModelHarness::TransactionalStringProperty));
        Assert::IsFalse(pModel.IsModified(DataModelHarness::TransactionalBoolProperty));

        oNotify.AssertNotChanged(DataModelHarness::TransactionalIntProperty);
        oNotify.AssertNotChanged(DataModelHarness::TransactionalStringProperty);
        oNotify.AssertNotChanged(DataModelHarness::TransactionalBoolProperty);
        oNotify.AssertBoolChanged(DataModelHarness::IsModifiedProperty, true, false);
        oNotify.Reset();

        Assert::AreEqual(99, pModel.GetTransactionalInt());
        Assert::AreEqual(std::wstring(L"Test"), pModel.GetTransactionalString());
        Assert::AreEqual(false, pModel.GetTransactionalBool());

        // revert nested transaction, will pick up modifications from first transaction
        pModel.RevertTransaction();
        Assert::IsTrue(pModel.IsModified());
        Assert::IsTrue(pModel.IsModified(DataModelHarness::TransactionalIntProperty));
        Assert::IsTrue(pModel.IsModified(DataModelHarness::TransactionalStringProperty));
        Assert::IsFalse(pModel.IsModified(DataModelHarness::TransactionalBoolProperty));

        oNotify.AssertNotChanged(DataModelHarness::TransactionalIntProperty);
        oNotify.AssertNotChanged(DataModelHarness::TransactionalStringProperty);
        oNotify.AssertNotChanged(DataModelHarness::TransactionalBoolProperty);
        oNotify.AssertBoolChanged(DataModelHarness::IsModifiedProperty, false, true);
        oNotify.Reset();

        // another nested transacion, this time, we'll modify it
        pModel.BeginTransaction();
        Assert::AreEqual(99, pModel.GetTransactionalInt());
        Assert::AreEqual(std::wstring(L"Test"), pModel.GetTransactionalString());
        Assert::AreEqual(false, pModel.GetTransactionalBool());

        pModel.SetTransactionalInt(50);
        Assert::IsTrue(pModel.IsModified());
        Assert::IsTrue(pModel.IsModified(DataModelHarness::TransactionalIntProperty));
        Assert::IsFalse(pModel.IsModified(DataModelHarness::TransactionalStringProperty));
        Assert::IsFalse(pModel.IsModified(DataModelHarness::TransactionalBoolProperty));
        oNotify.Reset();

        // revert nested transaction, will pick up modifications from first transaction
        pModel.RevertTransaction();
        Assert::IsTrue(pModel.IsModified());
        Assert::IsTrue(pModel.IsModified(DataModelHarness::TransactionalIntProperty));
        Assert::IsTrue(pModel.IsModified(DataModelHarness::TransactionalStringProperty));
        Assert::IsFalse(pModel.IsModified(DataModelHarness::TransactionalBoolProperty));

        oNotify.AssertIntChanged(DataModelHarness::TransactionalIntProperty, 50, 99);
        oNotify.AssertNotChanged(DataModelHarness::TransactionalStringProperty);
        oNotify.AssertNotChanged(DataModelHarness::TransactionalBoolProperty);
        oNotify.AssertNotChanged(DataModelHarness::IsModifiedProperty);
        oNotify.Reset();

        Assert::AreEqual(99, pModel.GetTransactionalInt());
        Assert::AreEqual(std::wstring(L"Test"), pModel.GetTransactionalString());
        Assert::AreEqual(false, pModel.GetTransactionalBool());

        // revert primary transaction
        pModel.RevertTransaction();
        Assert::IsFalse(pModel.IsModified());
        Assert::IsFalse(pModel.IsModified(DataModelHarness::TransactionalIntProperty));
        Assert::IsFalse(pModel.IsModified(DataModelHarness::TransactionalStringProperty));
        Assert::IsFalse(pModel.IsModified(DataModelHarness::TransactionalBoolProperty));

        oNotify.AssertIntChanged(DataModelHarness::TransactionalIntProperty, 99, 0);
        oNotify.AssertStringChanged(DataModelHarness::TransactionalStringProperty, L"Test", L"");
        oNotify.AssertNotChanged(DataModelHarness::TransactionalBoolProperty);
        oNotify.AssertBoolChanged(DataModelHarness::IsModifiedProperty, true, false);

        Assert::AreEqual(0, pModel.GetTransactionalInt());
        Assert::AreEqual(std::wstring(L""), pModel.GetTransactionalString());
        Assert::AreEqual(false, pModel.GetTransactionalBool());
    }

    TEST_METHOD(TestRevertTransactionRepeated)
    {
        DataModelHarness pModel;
        NotifyTargetHarness oNotify;
        pModel.AddNotifyTarget(oNotify);

        Assert::AreEqual(0, pModel.GetTransactionalInt());

        pModel.BeginTransaction();
        pModel.BeginTransaction();
        pModel.SetTransactionalInt(99);
        Assert::AreEqual(99, pModel.GetTransactionalInt());

        pModel.RevertTransaction();
        Assert::AreEqual(0, pModel.GetTransactionalInt());
        Assert::IsFalse(pModel.IsModified());

        pModel.RevertTransaction();
        Assert::AreEqual(0, pModel.GetTransactionalInt());
        Assert::IsFalse(pModel.IsModified());
    }

    TEST_METHOD(TestRevertTransactionMultipleChanges)
    {
        DataModelHarness pModel;
        NotifyTargetHarness oNotify;
        pModel.AddNotifyTarget(oNotify);

        pModel.SetTransactionalInt(99);
        pModel.SetTransactionalString(L"Test");
        pModel.BeginTransaction();
        oNotify.Reset();

        Assert::IsFalse(pModel.IsModified());
        Assert::IsFalse(pModel.IsModified(DataModelHarness::TransactionalIntProperty));
        Assert::IsFalse(pModel.IsModified(DataModelHarness::TransactionalStringProperty));
        Assert::IsFalse(pModel.IsModified(DataModelHarness::TransactionalBoolProperty));

        pModel.SetTransactionalInt(50);
        pModel.SetTransactionalString(L"Test2");
        Assert::IsTrue(pModel.IsModified());
        Assert::IsTrue(pModel.IsModified(DataModelHarness::TransactionalIntProperty));
        Assert::IsTrue(pModel.IsModified(DataModelHarness::TransactionalStringProperty));
        Assert::IsFalse(pModel.IsModified(DataModelHarness::TransactionalBoolProperty));
        oNotify.AssertStringChanged(DataModelHarness::TransactionalStringProperty, L"Test", L"Test2");
        oNotify.AssertIntChanged(DataModelHarness::TransactionalIntProperty, 99, 50);
        oNotify.AssertBoolChanged(DataModelHarness::IsModifiedProperty, false, true);
        oNotify.Reset();

        pModel.SetTransactionalInt(75);
        pModel.SetTransactionalString(L"Test3");
        Assert::IsTrue(pModel.IsModified());
        Assert::IsTrue(pModel.IsModified(DataModelHarness::TransactionalIntProperty));
        Assert::IsTrue(pModel.IsModified(DataModelHarness::TransactionalStringProperty));
        Assert::IsFalse(pModel.IsModified(DataModelHarness::TransactionalBoolProperty));
        oNotify.AssertStringChanged(DataModelHarness::TransactionalStringProperty, L"Test2", L"Test3");
        oNotify.AssertIntChanged(DataModelHarness::TransactionalIntProperty, 50, 75);
        oNotify.AssertNotChanged(DataModelHarness::IsModifiedProperty);
        oNotify.Reset();

        // revert should return to original state, regardless of how many times values have changed within the transaction
        pModel.RevertTransaction();
        Assert::IsFalse(pModel.IsModified());
        Assert::IsFalse(pModel.IsModified(DataModelHarness::TransactionalIntProperty));
        Assert::IsFalse(pModel.IsModified(DataModelHarness::TransactionalStringProperty));
        Assert::IsFalse(pModel.IsModified(DataModelHarness::TransactionalBoolProperty));
        oNotify.AssertStringChanged(DataModelHarness::TransactionalStringProperty, L"Test3", L"Test");
        oNotify.AssertIntChanged(DataModelHarness::TransactionalIntProperty, 75, 99);
        oNotify.AssertBoolChanged(DataModelHarness::IsModifiedProperty, true, false);
    }

    TEST_METHOD(TestCommitTransaction)
    {
        DataModelHarness pModel;
        NotifyTargetHarness oNotify;
        pModel.AddNotifyTarget(oNotify);
        pModel.BeginTransaction();

        Assert::IsFalse(pModel.IsModified());
        Assert::IsFalse(pModel.IsModified(DataModelHarness::TransactionalIntProperty));
        Assert::IsFalse(pModel.IsModified(DataModelHarness::TransactionalStringProperty));
        Assert::IsFalse(pModel.IsModified(DataModelHarness::TransactionalBoolProperty));

        pModel.SetTransactionalInt(99);
        pModel.SetTransactionalString(L"Test");
        Assert::IsTrue(pModel.IsModified());
        Assert::IsTrue(pModel.IsModified(DataModelHarness::TransactionalIntProperty));
        Assert::IsTrue(pModel.IsModified(DataModelHarness::TransactionalStringProperty));
        Assert::IsFalse(pModel.IsModified(DataModelHarness::TransactionalBoolProperty));
        oNotify.Reset();

        pModel.CommitTransaction();
        Assert::IsFalse(pModel.IsModified());
        Assert::IsFalse(pModel.IsModified(DataModelHarness::TransactionalIntProperty));
        Assert::IsFalse(pModel.IsModified(DataModelHarness::TransactionalStringProperty));
        Assert::IsFalse(pModel.IsModified(DataModelHarness::TransactionalBoolProperty));

        Assert::AreEqual(99, pModel.GetTransactionalInt());
        Assert::AreEqual(std::wstring(L"Test"), pModel.GetTransactionalString());
        Assert::AreEqual(false, pModel.GetTransactionalBool());

        // no transaction to commit - modifications should be kept, but not tracked
        pModel.SetTransactionalInt(50);
        pModel.SetTransactionalString(L"Test2");
        pModel.RevertTransaction();
        Assert::IsFalse(pModel.IsModified());
        Assert::IsFalse(pModel.IsModified(DataModelHarness::TransactionalIntProperty));
        Assert::IsFalse(pModel.IsModified(DataModelHarness::TransactionalStringProperty));
        Assert::IsFalse(pModel.IsModified(DataModelHarness::TransactionalBoolProperty));

        Assert::AreEqual(50, pModel.GetTransactionalInt());
        Assert::AreEqual(std::wstring(L"Test2"), pModel.GetTransactionalString());
        Assert::AreEqual(false, pModel.GetTransactionalBool());
    }

    TEST_METHOD(TestCommitTransactionNested)
    {
        DataModelHarness pModel;
        NotifyTargetHarness oNotify;
        pModel.AddNotifyTarget(oNotify);

        // create a transaction and modify it
        pModel.BeginTransaction();
        pModel.SetTransactionalInt(99);
        pModel.SetTransactionalString(L"Test");
        Assert::IsTrue(pModel.IsModified());
        Assert::IsTrue(pModel.IsModified(DataModelHarness::TransactionalIntProperty));
        Assert::IsTrue(pModel.IsModified(DataModelHarness::TransactionalStringProperty));
        Assert::IsFalse(pModel.IsModified(DataModelHarness::TransactionalBoolProperty));
        oNotify.Reset();

        // nested transaction is not initially considered modified
        pModel.BeginTransaction();
        Assert::IsFalse(pModel.IsModified());
        Assert::IsFalse(pModel.IsModified(DataModelHarness::TransactionalIntProperty));
        Assert::IsFalse(pModel.IsModified(DataModelHarness::TransactionalStringProperty));
        Assert::IsFalse(pModel.IsModified(DataModelHarness::TransactionalBoolProperty));

        oNotify.AssertNotChanged(DataModelHarness::TransactionalIntProperty);
        oNotify.AssertNotChanged(DataModelHarness::TransactionalStringProperty);
        oNotify.AssertNotChanged(DataModelHarness::TransactionalBoolProperty);
        oNotify.AssertBoolChanged(DataModelHarness::IsModifiedProperty, true, false);
        oNotify.Reset();

        Assert::AreEqual(99, pModel.GetTransactionalInt());
        Assert::AreEqual(std::wstring(L"Test"), pModel.GetTransactionalString());
        Assert::AreEqual(false, pModel.GetTransactionalBool());

        // commit (unmodified) nested transaction, will pick up modifications from first transaction
        pModel.CommitTransaction();
        Assert::IsTrue(pModel.IsModified());
        Assert::IsTrue(pModel.IsModified(DataModelHarness::TransactionalIntProperty));
        Assert::IsTrue(pModel.IsModified(DataModelHarness::TransactionalStringProperty));
        Assert::IsFalse(pModel.IsModified(DataModelHarness::TransactionalBoolProperty));

        oNotify.AssertNotChanged(DataModelHarness::TransactionalIntProperty);
        oNotify.AssertNotChanged(DataModelHarness::TransactionalStringProperty);
        oNotify.AssertNotChanged(DataModelHarness::TransactionalBoolProperty);
        oNotify.AssertBoolChanged(DataModelHarness::IsModifiedProperty, false, true);
        oNotify.Reset();

        // another nested transacion, this time, we'll modify it
        pModel.BeginTransaction();
        Assert::AreEqual(99, pModel.GetTransactionalInt());
        Assert::AreEqual(std::wstring(L"Test"), pModel.GetTransactionalString());
        Assert::AreEqual(false, pModel.GetTransactionalBool());

        pModel.SetTransactionalInt(50);
        Assert::IsTrue(pModel.IsModified());
        Assert::IsTrue(pModel.IsModified(DataModelHarness::TransactionalIntProperty));
        Assert::IsFalse(pModel.IsModified(DataModelHarness::TransactionalStringProperty));
        Assert::IsFalse(pModel.IsModified(DataModelHarness::TransactionalBoolProperty));
        oNotify.Reset();

        // revert nested transaction, will pick up modifications from first transaction
        pModel.CommitTransaction();
        Assert::IsTrue(pModel.IsModified());
        Assert::IsTrue(pModel.IsModified(DataModelHarness::TransactionalIntProperty));
        Assert::IsTrue(pModel.IsModified(DataModelHarness::TransactionalStringProperty));
        Assert::IsFalse(pModel.IsModified(DataModelHarness::TransactionalBoolProperty));

        oNotify.AssertNotChanged(DataModelHarness::TransactionalIntProperty);
        oNotify.AssertNotChanged(DataModelHarness::TransactionalStringProperty);
        oNotify.AssertNotChanged(DataModelHarness::TransactionalBoolProperty);
        oNotify.AssertNotChanged(DataModelHarness::IsModifiedProperty);
        oNotify.Reset();

        Assert::AreEqual(50, pModel.GetTransactionalInt());
        Assert::AreEqual(std::wstring(L"Test"), pModel.GetTransactionalString());
        Assert::AreEqual(false, pModel.GetTransactionalBool());

        // commit primary transaction
        pModel.CommitTransaction();
        Assert::IsFalse(pModel.IsModified());
        Assert::IsFalse(pModel.IsModified(DataModelHarness::TransactionalIntProperty));
        Assert::IsFalse(pModel.IsModified(DataModelHarness::TransactionalStringProperty));
        Assert::IsFalse(pModel.IsModified(DataModelHarness::TransactionalBoolProperty));

        oNotify.AssertNotChanged(DataModelHarness::TransactionalIntProperty);
        oNotify.AssertNotChanged(DataModelHarness::TransactionalStringProperty);
        oNotify.AssertNotChanged(DataModelHarness::TransactionalBoolProperty);
        oNotify.AssertBoolChanged(DataModelHarness::IsModifiedProperty, true, false);

        Assert::AreEqual(50, pModel.GetTransactionalInt());
        Assert::AreEqual(std::wstring(L"Test"), pModel.GetTransactionalString());
        Assert::AreEqual(false, pModel.GetTransactionalBool());
    }

    TEST_METHOD(TestCommitTransactionNestedOriginalValues)
    {
        DataModelHarness pModel;
        NotifyTargetHarness oNotify;
        pModel.AddNotifyTarget(oNotify);

        // set some initial values
        pModel.SetTransactionalInt(99);
        pModel.SetTransactionalString(L"Test");
        oNotify.Reset();

        // create a transaction and modify it
        pModel.BeginTransaction();
        pModel.SetTransactionalInt(50);
        pModel.SetTransactionalString(L"Test2");
        oNotify.Reset();

        // create another transaction and set the values back to the original values
        pModel.BeginTransaction();
        pModel.SetTransactionalInt(99);
        pModel.SetTransactionalString(L"Test");

        Assert::IsTrue(pModel.IsModified());
        Assert::IsTrue(pModel.IsModified(DataModelHarness::TransactionalIntProperty));
        Assert::IsTrue(pModel.IsModified(DataModelHarness::TransactionalStringProperty));
        Assert::IsFalse(pModel.IsModified(DataModelHarness::TransactionalBoolProperty));

        oNotify.AssertIntChanged(DataModelHarness::TransactionalIntProperty, 50, 99);
        oNotify.AssertStringChanged(DataModelHarness::TransactionalStringProperty, L"Test2", L"Test");
        oNotify.AssertNotChanged(DataModelHarness::TransactionalBoolProperty);
        oNotify.Reset();

        Assert::AreEqual(99, pModel.GetTransactionalInt());
        Assert::AreEqual(std::wstring(L"Test"), pModel.GetTransactionalString());
        Assert::AreEqual(false, pModel.GetTransactionalBool());

        // commit nested transaction, since the values match the original values, the original transaction will appear unmodified
        pModel.CommitTransaction();
        Assert::IsFalse(pModel.IsModified());
        Assert::IsFalse(pModel.IsModified(DataModelHarness::TransactionalIntProperty));
        Assert::IsFalse(pModel.IsModified(DataModelHarness::TransactionalStringProperty));
        Assert::IsFalse(pModel.IsModified(DataModelHarness::TransactionalBoolProperty));

        oNotify.AssertNotChanged(DataModelHarness::TransactionalIntProperty);
        oNotify.AssertNotChanged(DataModelHarness::TransactionalStringProperty);
        oNotify.AssertNotChanged(DataModelHarness::TransactionalBoolProperty);
        oNotify.AssertBoolChanged(DataModelHarness::IsModifiedProperty, true, false);
        oNotify.Reset();

        Assert::AreEqual(99, pModel.GetTransactionalInt());
        Assert::AreEqual(std::wstring(L"Test"), pModel.GetTransactionalString());
        Assert::AreEqual(false, pModel.GetTransactionalBool());

        // commiting the now-unmodified original transaction should have no effect
        pModel.CommitTransaction();
        Assert::IsFalse(pModel.IsModified());
        Assert::IsFalse(pModel.IsModified(DataModelHarness::TransactionalIntProperty));
        Assert::IsFalse(pModel.IsModified(DataModelHarness::TransactionalStringProperty));
        Assert::IsFalse(pModel.IsModified(DataModelHarness::TransactionalBoolProperty));

        oNotify.AssertNotChanged(DataModelHarness::TransactionalIntProperty);
        oNotify.AssertNotChanged(DataModelHarness::TransactionalStringProperty);
        oNotify.AssertNotChanged(DataModelHarness::TransactionalBoolProperty);
        oNotify.AssertNotChanged(DataModelHarness::IsModifiedProperty);

        Assert::AreEqual(99, pModel.GetTransactionalInt());
        Assert::AreEqual(std::wstring(L"Test"), pModel.GetTransactionalString());
        Assert::AreEqual(false, pModel.GetTransactionalBool());
    }

    TEST_METHOD(TestPreviousValues)
    {
        DataModelHarness pModel;
        pModel.BeginTransaction();

        // no previous value captured
        Assert::IsNull(pModel.GetPreviousTransactonalBool());
        Assert::IsNull(pModel.GetPreviousTransactonalInt());
        Assert::IsNull(pModel.GetPreviousTransactonalString());

        pModel.BeginTransaction();
        pModel.SetTransactionalBool(true);
        pModel.SetTransactionalInt(50);
        pModel.SetTransactionalString(L"Test2");

        // original values captured
        Assert::AreEqual(false, *pModel.GetPreviousTransactonalBool());
        Assert::AreEqual(0, *pModel.GetPreviousTransactonalInt());
        Assert::AreEqual(std::wstring(), *pModel.GetPreviousTransactonalString());

        pModel.BeginTransaction();

        // no previous values captured for this transaction
        Assert::IsNull(pModel.GetPreviousTransactonalBool());
        Assert::IsNull(pModel.GetPreviousTransactonalInt());
        Assert::IsNull(pModel.GetPreviousTransactonalString());

        pModel.SetTransactionalBool(false);
        pModel.SetTransactionalInt(100);
        pModel.SetTransactionalString(L"Test3");

        // previous values captured
        Assert::AreEqual(true, *pModel.GetPreviousTransactonalBool());
        Assert::AreEqual(50, *pModel.GetPreviousTransactonalInt());
        Assert::AreEqual(std::wstring(L"Test2"), *pModel.GetPreviousTransactonalString());

        pModel.CommitTransaction();

        // previous values updated from commit - since the boolean matches the original value
        // it returns null. the other older original values are maintained
        Assert::IsNull(pModel.GetPreviousTransactonalBool());
        Assert::AreEqual(0, *pModel.GetPreviousTransactonalInt());
        Assert::AreEqual(std::wstring(), *pModel.GetPreviousTransactonalString());
    }

    TEST_METHOD(TestIsModified)
    {
        DataModelHarness pModel;
        pModel.BeginTransaction();

        Assert::IsFalse(pModel.TransactionIsModified());

        pModel.BeginTransaction();
        pModel.SetTransactionalBool(true);
        pModel.SetTransactionalInt(50);
        pModel.SetTransactionalString(L"Test2");
        Assert::IsTrue(pModel.TransactionIsModified());

        pModel.BeginTransaction();
        Assert::IsFalse(pModel.TransactionIsModified());

        pModel.SetTransactionalBool(false);
        Assert::IsTrue(pModel.TransactionIsModified());
    }

    TEST_METHOD(TestBeginUpdate)
    {
        DataModelHarness pModel;
        Assert::IsFalse(pModel.GetBool());
        Assert::IsFalse(pModel.GetTransactionalBool());
        Assert::AreEqual(0, pModel.GetInt());
        Assert::AreEqual(0, pModel.GetTransactionalInt());
        Assert::AreEqual(std::wstring(), pModel.GetString());
        Assert::AreEqual(std::wstring(), pModel.GetTransactionalString());
        pModel.BeginTransaction();

        NotifyTargetHarness oNotify;
        pModel.AddNotifyTarget(oNotify);

        Assert::IsFalse(pModel.IsUpdating());
        pModel.BeginUpdate();
        Assert::IsTrue(pModel.IsUpdating());

        pModel.SetBool(true);
        Assert::IsTrue(pModel.GetBool());
        Assert::IsFalse(pModel.IsModified());
        oNotify.AssertNotChanged(DataModelHarness::BoolProperty);
        oNotify.AssertNotChanged(DataModelHarness::IsModifiedProperty);

        pModel.SetTransactionalBool(true);
        Assert::IsTrue(pModel.GetTransactionalBool());
        Assert::IsTrue(pModel.IsModified());
        oNotify.AssertNotChanged(DataModelHarness::TransactionalBoolProperty);
        oNotify.AssertNotChanged(DataModelHarness::IsModifiedProperty); // this change also queued

        pModel.SetString(L"Test");
        Assert::AreEqual(std::wstring(L"Test"), pModel.GetString());
        pModel.SetTransactionalString(L"Test");
        Assert::AreEqual(std::wstring(L"Test"), pModel.GetTransactionalString());
        oNotify.AssertNotChanged(DataModelHarness::StringProperty);
        oNotify.AssertNotChanged(DataModelHarness::TransactionalStringProperty);

        pModel.SetInt(99);
        Assert::AreEqual(99, pModel.GetInt());
        pModel.SetTransactionalInt(99);
        Assert::AreEqual(99, pModel.GetTransactionalInt());
        oNotify.AssertNotChanged(DataModelHarness::IntProperty);
        oNotify.AssertNotChanged(DataModelHarness::TransactionalIntProperty);

        oNotify.Reset();
        Assert::IsTrue(pModel.IsUpdating());
        pModel.EndUpdate();
        Assert::IsFalse(pModel.IsUpdating());

        oNotify.AssertBoolChanged(DataModelHarness::BoolProperty, false, true);
        oNotify.AssertBoolChanged(DataModelHarness::TransactionalBoolProperty, false, true);
        oNotify.AssertIntChanged(DataModelHarness::IntProperty, 0, 99);
        oNotify.AssertIntChanged(DataModelHarness::TransactionalIntProperty, 0, 99);
        oNotify.AssertStringChanged(DataModelHarness::StringProperty, L"", L"Test");
        oNotify.AssertStringChanged(DataModelHarness::TransactionalStringProperty, L"", L"Test");
        oNotify.AssertBoolChanged(DataModelHarness::IsModifiedProperty, false, true);
    }

    TEST_METHOD(TestBeginUpdateNested)
    {
        DataModelHarness pModel;
        Assert::IsFalse(pModel.GetBool());
        Assert::IsFalse(pModel.GetTransactionalBool());
        Assert::AreEqual(0, pModel.GetInt());
        Assert::AreEqual(0, pModel.GetTransactionalInt());
        Assert::AreEqual(std::wstring(), pModel.GetString());
        Assert::AreEqual(std::wstring(), pModel.GetTransactionalString());
        pModel.BeginTransaction();

        NotifyTargetHarness oNotify;
        pModel.AddNotifyTarget(oNotify);

        Assert::IsFalse(pModel.IsUpdating());
        pModel.BeginUpdate();
        Assert::IsTrue(pModel.IsUpdating());
        pModel.BeginUpdate();
        Assert::IsTrue(pModel.IsUpdating());

        pModel.SetBool(true);
        pModel.SetTransactionalBool(true);
        pModel.SetString(L"Test");
        pModel.SetTransactionalString(L"Test");
        pModel.SetInt(99);
        pModel.SetTransactionalInt(99);

        oNotify.AssertNotChanged(DataModelHarness::BoolProperty);
        oNotify.AssertNotChanged(DataModelHarness::TransactionalBoolProperty);
        oNotify.AssertNotChanged(DataModelHarness::StringProperty);
        oNotify.AssertNotChanged(DataModelHarness::TransactionalStringProperty);
        oNotify.AssertNotChanged(DataModelHarness::IntProperty);
        oNotify.AssertNotChanged(DataModelHarness::TransactionalIntProperty);
        oNotify.AssertNotChanged(DataModelHarness::IsModifiedProperty);

        oNotify.Reset();
        Assert::IsTrue(pModel.IsUpdating());
        pModel.EndUpdate();
        Assert::IsTrue(pModel.IsUpdating());

        oNotify.AssertNotChanged(DataModelHarness::BoolProperty);
        oNotify.AssertNotChanged(DataModelHarness::TransactionalBoolProperty);
        oNotify.AssertNotChanged(DataModelHarness::StringProperty);
        oNotify.AssertNotChanged(DataModelHarness::TransactionalStringProperty);
        oNotify.AssertNotChanged(DataModelHarness::IntProperty);
        oNotify.AssertNotChanged(DataModelHarness::TransactionalIntProperty);
        oNotify.AssertNotChanged(DataModelHarness::IsModifiedProperty);

        oNotify.Reset();
        Assert::IsTrue(pModel.IsUpdating());
        pModel.EndUpdate();
        Assert::IsFalse(pModel.IsUpdating());

        oNotify.AssertBoolChanged(DataModelHarness::BoolProperty, false, true);
        oNotify.AssertBoolChanged(DataModelHarness::TransactionalBoolProperty, false, true);
        oNotify.AssertIntChanged(DataModelHarness::IntProperty, 0, 99);
        oNotify.AssertIntChanged(DataModelHarness::TransactionalIntProperty, 0, 99);
        oNotify.AssertStringChanged(DataModelHarness::StringProperty, L"", L"Test");
        oNotify.AssertStringChanged(DataModelHarness::TransactionalStringProperty, L"", L"Test");
        oNotify.AssertBoolChanged(DataModelHarness::IsModifiedProperty, false, true);
    }


    TEST_METHOD(TestBeginUpdateNestedChangesBetweenBeginUpdatesInt)
    {
        DataModelHarness pModel;
        Assert::AreEqual(0, pModel.GetInt());
        Assert::AreEqual(0, pModel.GetTransactionalInt());
        pModel.BeginTransaction();

        NotifyTargetHarness oNotify;
        pModel.AddNotifyTarget(oNotify);

        pModel.BeginUpdate();

        pModel.SetInt(66);
        pModel.SetTransactionalInt(66);

        pModel.BeginUpdate();

        pModel.SetInt(99);
        pModel.SetTransactionalInt(99);

        oNotify.AssertNotChanged(DataModelHarness::IntProperty);
        oNotify.AssertNotChanged(DataModelHarness::TransactionalIntProperty);
        oNotify.AssertNotChanged(DataModelHarness::IsModifiedProperty);

        oNotify.Reset();
        pModel.EndUpdate();

        oNotify.AssertNotChanged(DataModelHarness::IntProperty);
        oNotify.AssertNotChanged(DataModelHarness::TransactionalIntProperty);
        oNotify.AssertNotChanged(DataModelHarness::IsModifiedProperty);

        oNotify.Reset();
        pModel.EndUpdate();

        oNotify.AssertIntChanged(DataModelHarness::IntProperty, 0, 99);
        oNotify.AssertIntChanged(DataModelHarness::TransactionalIntProperty, 0, 99);
        oNotify.AssertBoolChanged(DataModelHarness::IsModifiedProperty, false, true);
    }

    TEST_METHOD(TestBeginUpdateNestedChangesBetweenEndUpdatesInt)
    {
        DataModelHarness pModel;
        Assert::AreEqual(0, pModel.GetInt());
        Assert::AreEqual(0, pModel.GetTransactionalInt());
        pModel.BeginTransaction();

        NotifyTargetHarness oNotify;
        pModel.AddNotifyTarget(oNotify);

        pModel.BeginUpdate();
        pModel.BeginUpdate();

        pModel.SetInt(66);
        pModel.SetTransactionalInt(66);

        oNotify.AssertNotChanged(DataModelHarness::IntProperty);
        oNotify.AssertNotChanged(DataModelHarness::TransactionalIntProperty);
        oNotify.AssertNotChanged(DataModelHarness::IsModifiedProperty);

        oNotify.Reset();
        pModel.EndUpdate();

        pModel.SetInt(99);
        pModel.SetTransactionalInt(99);

        oNotify.AssertNotChanged(DataModelHarness::IntProperty);
        oNotify.AssertNotChanged(DataModelHarness::TransactionalIntProperty);
        oNotify.AssertNotChanged(DataModelHarness::IsModifiedProperty);

        oNotify.Reset();
        pModel.EndUpdate();

        oNotify.AssertIntChanged(DataModelHarness::IntProperty, 0, 99);
        oNotify.AssertIntChanged(DataModelHarness::TransactionalIntProperty, 0, 99);
        oNotify.AssertBoolChanged(DataModelHarness::IsModifiedProperty, false, true);
    }

    TEST_METHOD(TestBeginUpdateChangeAndChangeBackBool)
    {
        DataModelHarness pModel;
        pModel.BeginTransaction();
        Assert::IsFalse(pModel.GetBool());
        Assert::IsFalse(pModel.GetTransactionalBool());

        NotifyTargetHarness oNotify;
        pModel.AddNotifyTarget(oNotify);

        pModel.BeginUpdate();

        pModel.SetBool(true);
        pModel.SetTransactionalBool(true);

        pModel.SetBool(false);
        pModel.SetTransactionalBool(false);

        pModel.EndUpdate();

        oNotify.AssertNotChanged(DataModelHarness::BoolProperty);
        oNotify.AssertNotChanged(DataModelHarness::TransactionalBoolProperty);
        oNotify.AssertNotChanged(DataModelHarness::IsModifiedProperty);

        Assert::IsFalse(pModel.GetBool());
        Assert::IsFalse(pModel.GetTransactionalBool());
    }

    TEST_METHOD(TestBeginUpdateChangeAndChangeBackInt)
    {
        DataModelHarness pModel;
        pModel.BeginTransaction();
        Assert::AreEqual(0, pModel.GetInt());
        Assert::AreEqual(0, pModel.GetTransactionalInt());

        NotifyTargetHarness oNotify;
        pModel.AddNotifyTarget(oNotify);

        pModel.BeginUpdate();

        pModel.SetInt(5);
        pModel.SetTransactionalInt(5);

        pModel.SetInt(99);
        pModel.SetTransactionalInt(99);

        pModel.SetInt(0);
        pModel.SetTransactionalInt(0);

        pModel.EndUpdate();

        oNotify.AssertNotChanged(DataModelHarness::IntProperty);
        oNotify.AssertNotChanged(DataModelHarness::TransactionalIntProperty);
        oNotify.AssertNotChanged(DataModelHarness::IsModifiedProperty);

        Assert::AreEqual(0, pModel.GetInt());
        Assert::AreEqual(0, pModel.GetTransactionalInt());
    }

    TEST_METHOD(TestBeginUpdateChangeAndChangeBackString)
    {
        DataModelHarness pModel;
        pModel.BeginTransaction();
        Assert::AreEqual(std::wstring(), pModel.GetString());
        Assert::AreEqual(std::wstring(), pModel.GetTransactionalString());

        NotifyTargetHarness oNotify;
        pModel.AddNotifyTarget(oNotify);

        pModel.BeginUpdate();

        pModel.SetString(L"Test");
        pModel.SetTransactionalString(L"Test");

        pModel.SetString(L"Test2");
        pModel.SetTransactionalString(L"Test2");

        pModel.SetString(L"");
        pModel.SetTransactionalString(L"");

        pModel.EndUpdate();

        oNotify.AssertNotChanged(DataModelHarness::StringProperty);
        oNotify.AssertNotChanged(DataModelHarness::TransactionalStringProperty);
        oNotify.AssertNotChanged(DataModelHarness::IsModifiedProperty);

        Assert::AreEqual(std::wstring(), pModel.GetString());
        Assert::AreEqual(std::wstring(), pModel.GetTransactionalString());
    }

    TEST_METHOD(TestBeginUpdateMultipleChangesBool)
    {
        DataModelHarness pModel;
        pModel.BeginTransaction();
        Assert::IsFalse(pModel.GetBool());
        Assert::IsFalse(pModel.GetTransactionalBool());

        NotifyTargetHarness oNotify;
        pModel.AddNotifyTarget(oNotify);

        pModel.BeginUpdate();

        pModel.SetBool(true);
        pModel.SetTransactionalBool(true);

        pModel.SetBool(false);
        pModel.SetTransactionalBool(false);

        pModel.SetBool(true);
        pModel.SetTransactionalBool(true);

        pModel.EndUpdate();

        oNotify.AssertBoolChanged(DataModelHarness::BoolProperty, false, true);
        oNotify.AssertBoolChanged(DataModelHarness::TransactionalBoolProperty, false, true);
        oNotify.AssertBoolChanged(DataModelHarness::IsModifiedProperty, false, true);

        Assert::IsTrue(pModel.GetBool());
        Assert::IsTrue(pModel.GetTransactionalBool());
    }

    TEST_METHOD(TestBeginUpdateMultipleChangesInt)
    {
        DataModelHarness pModel;
        pModel.BeginTransaction();
        Assert::AreEqual(0, pModel.GetInt());
        Assert::AreEqual(0, pModel.GetTransactionalInt());

        NotifyTargetHarness oNotify;
        pModel.AddNotifyTarget(oNotify);

        pModel.BeginUpdate();

        pModel.SetInt(5);
        pModel.SetTransactionalInt(5);

        pModel.SetInt(99);
        pModel.SetTransactionalInt(99);

        pModel.SetInt(67);
        pModel.SetTransactionalInt(68);

        pModel.EndUpdate();

        oNotify.AssertIntChanged(DataModelHarness::IntProperty, 0, 67);
        oNotify.AssertIntChanged(DataModelHarness::TransactionalIntProperty, 0, 68);
        oNotify.AssertBoolChanged(DataModelHarness::IsModifiedProperty, false, true);

        Assert::AreEqual(67, pModel.GetInt());
        Assert::AreEqual(68, pModel.GetTransactionalInt());
    }

    TEST_METHOD(TestBeginUpdateMultipleChangesString)
    {
        DataModelHarness pModel;
        pModel.BeginTransaction();
        Assert::AreEqual(std::wstring(), pModel.GetString());
        Assert::AreEqual(std::wstring(), pModel.GetTransactionalString());

        NotifyTargetHarness oNotify;
        pModel.AddNotifyTarget(oNotify);

        pModel.BeginUpdate();

        pModel.SetString(L"Test");
        pModel.SetTransactionalString(L"Test");

        pModel.SetString(L"Test2");
        pModel.SetTransactionalString(L"Test2");

        pModel.SetString(L"Test3a");
        pModel.SetTransactionalString(L"Test3b");

        pModel.EndUpdate();

        oNotify.AssertStringChanged(DataModelHarness::StringProperty, L"", L"Test3a");
        oNotify.AssertStringChanged(DataModelHarness::TransactionalStringProperty, L"", L"Test3b");
        oNotify.AssertBoolChanged(DataModelHarness::IsModifiedProperty, false, true);

        Assert::AreEqual(std::wstring(L"Test3a"), pModel.GetString());
        Assert::AreEqual(std::wstring(L"Test3b"), pModel.GetTransactionalString());
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
