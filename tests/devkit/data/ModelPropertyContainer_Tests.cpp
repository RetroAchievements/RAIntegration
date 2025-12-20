#include "data\ModelPropertyContainer.hh"

#include "testutil\CppUnitTest.hh"

namespace ra {
namespace data {
namespace tests {

TEST_CLASS(ModelPropertyContainer_Tests)
{
    class ModelPropertyContainerHarness : public ModelPropertyContainer
    {
    public:
        StringModelProperty StringProperty{ "ModelPropertyContainerHarness", "String", L"" };
        const std::wstring& GetString() const { return GetValue(StringProperty); }
        void SetString(const std::wstring& sValue) { SetValue(StringProperty, sValue); }

        IntModelProperty IntProperty{ "ModelPropertyContainerHarness", "Int", 0 };
        int GetInt() const { return GetValue(IntProperty); }
        void SetInt(int nValue) { SetValue(IntProperty, nValue); }

        BoolModelProperty BoolProperty{ "ModelPropertyContainerHarness", "Bool", false };
        bool GetBool() const { return GetValue(BoolProperty); }
        void SetBool(bool bValue) { SetValue(BoolProperty, bValue); }
    };

public:
    TEST_METHOD(TestStringProperty)
    {
        ModelPropertyContainerHarness container;
        Assert::AreEqual(std::wstring(), container.GetString());

        container.SetString(L"Test");
        Assert::AreEqual(std::wstring(L"Test"), container.GetString());

        container.SetString(L"Test2");
        Assert::AreEqual(std::wstring(L"Test2"), container.GetString());

        container.SetString(container.StringProperty.GetDefaultValue());
        Assert::AreEqual(std::wstring(), container.GetString());
    }

    TEST_METHOD(TestIntProperty)
    {
        ModelPropertyContainerHarness container;
        Assert::AreEqual(0, container.GetInt());

        container.SetInt(32);
        Assert::AreEqual(32, container.GetInt());

        container.SetInt(64);
        Assert::AreEqual(64, container.GetInt());

        container.SetInt(-1);
        Assert::AreEqual(-1, container.GetInt());

        container.SetInt(container.IntProperty.GetDefaultValue());
        Assert::AreEqual(0, container.GetInt());
    }

    TEST_METHOD(TestBoolProperty)
    {
        ModelPropertyContainerHarness container;
        Assert::AreEqual(false, container.GetBool());

        container.SetBool(true);
        Assert::AreEqual(true, container.GetBool());

        container.SetBool(container.BoolProperty.GetDefaultValue());
        Assert::AreEqual(false, container.GetBool());
    }

    TEST_METHOD(TestSetAllInOrder)
    {
        ModelPropertyContainerHarness container;
        Assert::AreEqual(std::wstring(), container.GetString());
        Assert::AreEqual(0, container.GetInt());
        Assert::AreEqual(false, container.GetBool());

        container.SetString(L"Test");
        container.SetInt(32);
        container.SetBool(true);

        Assert::AreEqual(std::wstring(L"Test"), container.GetString());
        Assert::AreEqual(32, container.GetInt());
        Assert::AreEqual(true, container.GetBool());

        container.SetString(container.StringProperty.GetDefaultValue());
        container.SetInt(container.IntProperty.GetDefaultValue());
        container.SetBool(container.BoolProperty.GetDefaultValue());

        Assert::AreEqual(std::wstring(), container.GetString());
        Assert::AreEqual(0, container.GetInt());
        Assert::AreEqual(false, container.GetBool());
    }

    TEST_METHOD(TestSetAllInReverseOrder)
    {
        ModelPropertyContainerHarness container;
        Assert::AreEqual(std::wstring(), container.GetString());
        Assert::AreEqual(0, container.GetInt());
        Assert::AreEqual(false, container.GetBool());

        container.SetBool(true);
        container.SetInt(32);
        container.SetString(L"Test");

        Assert::AreEqual(std::wstring(L"Test"), container.GetString());
        Assert::AreEqual(32, container.GetInt());
        Assert::AreEqual(true, container.GetBool());

        container.SetBool(container.BoolProperty.GetDefaultValue());
        container.SetInt(container.IntProperty.GetDefaultValue());
        container.SetString(container.StringProperty.GetDefaultValue());

        Assert::AreEqual(std::wstring(), container.GetString());
        Assert::AreEqual(0, container.GetInt());
        Assert::AreEqual(false, container.GetBool());
    }

    TEST_METHOD(TestSetAllInZigZagOrder)
    {
        ModelPropertyContainerHarness container;
        Assert::AreEqual(std::wstring(), container.GetString());
        Assert::AreEqual(0, container.GetInt());
        Assert::AreEqual(false, container.GetBool());

        container.SetInt(32);
        container.SetString(L"Test");
        container.SetBool(true);

        Assert::AreEqual(std::wstring(L"Test"), container.GetString());
        Assert::AreEqual(32, container.GetInt());
        Assert::AreEqual(true, container.GetBool());

        container.SetInt(container.IntProperty.GetDefaultValue());
        container.SetString(container.StringProperty.GetDefaultValue());
        container.SetBool(container.BoolProperty.GetDefaultValue());

        Assert::AreEqual(std::wstring(), container.GetString());
        Assert::AreEqual(0, container.GetInt());
        Assert::AreEqual(false, container.GetBool());
    }
};

} // namespace tests
} // namespace data
} // namespace ra
