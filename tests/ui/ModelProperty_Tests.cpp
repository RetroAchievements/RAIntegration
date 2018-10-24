#include "ui\ModelProperty.hh"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ra {
namespace ui {
namespace tests {

TEST_CLASS(ModelProperty_Tests)
{
    void AssertProperty(int nKey, const ModelPropertyBase* pExpected)
    {
        const ModelPropertyBase* pLookup = ModelPropertyBase::GetPropertyForKey(nKey);
        if (pLookup != pExpected)
        {
            if (pExpected)
                Assert::Fail(L"Found property when one wasn't expected");
            else
                Assert::Fail(L"Did not find expected property");
        }
    }

public:
    TEST_METHOD(TestStringModelProperty)
    {
        int nKey;
        {
            StringModelProperty pProperty("Test", "Property", L"Default");

            Assert::AreEqual("Test", pProperty.GetTypeName());
            Assert::AreEqual("Property", pProperty.GetPropertyName());
            Assert::AreEqual(std::wstring(L"Default"), pProperty.GetDefaultValue());

            nKey = pProperty.GetKey();
            AssertProperty(nKey, &pProperty);

            pProperty.SetDefaultValue(L"New Default");
            Assert::AreEqual(std::wstring(L"New Default"), pProperty.GetDefaultValue());
        }

        AssertProperty(nKey, nullptr);
    }

    TEST_METHOD(TestIntModelProperty)
    {
        int nKey;
        {
            IntModelProperty pProperty("Test", "Property", 8);

            Assert::AreEqual("Test", pProperty.GetTypeName());
            Assert::AreEqual("Property", pProperty.GetPropertyName());
            Assert::AreEqual(8, pProperty.GetDefaultValue());

            nKey = pProperty.GetKey();
            AssertProperty(nKey, &pProperty);

            pProperty.SetDefaultValue(13);
            Assert::AreEqual(13, pProperty.GetDefaultValue());
        }

        AssertProperty(nKey, nullptr);
    }

    TEST_METHOD(TestBoolModelProperty)
    {
        int nKey;
        {
            BoolModelProperty pProperty("Test", "Property", true);

            Assert::AreEqual("Test", pProperty.GetTypeName());
            Assert::AreEqual("Property", pProperty.GetPropertyName());
            Assert::AreEqual(true, pProperty.GetDefaultValue());

            nKey = pProperty.GetKey();
            AssertProperty(nKey, &pProperty);

            pProperty.SetDefaultValue(false);
            Assert::AreEqual(false, pProperty.GetDefaultValue());
        }

        AssertProperty(nKey, nullptr);
    }
};

} // namespace tests
} // namespace ui
} // namespace ra
