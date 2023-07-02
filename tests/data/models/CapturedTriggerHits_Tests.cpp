#include "CppUnitTest.h"

#include "data\models\CapturedTriggerHits.hh"

#include "tests\mocks\MockConsoleContext.hh"
#include "tests\mocks\MockEmulatorContext.hh"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ra {
namespace data {
namespace models {
namespace tests {

TEST_CLASS(CapturedTriggerHits_Tests)
{
public:
    TEST_METHOD(TestRestoreTrigger)
    {
        const std::string trigger_definition = "0xH1234=1_0xH2345=2";
        char buffer[512];
        rc_trigger_t* trigger = rc_parse_trigger(buffer, trigger_definition.c_str(), NULL, 0);
        CapturedTriggerHits hits;

        trigger->requirement->conditions->current_hits = 6U;
        trigger->requirement->conditions->next->current_hits = 7U;
        trigger->has_hits = 1;

        hits.Capture(trigger, trigger_definition);

        trigger->requirement->conditions->current_hits = 8U;
        trigger->requirement->conditions->next->current_hits = 15U;

        Assert::IsTrue(hits.Restore(trigger, trigger_definition));

        Assert::AreEqual(6U, trigger->requirement->conditions->current_hits);
        Assert::AreEqual(7U, trigger->requirement->conditions->next->current_hits);
        Assert::AreEqual({1}, trigger->has_hits);
    }

    TEST_METHOD(TestRestoreTriggerAlts)
    {
        const std::string trigger_definition = "0xH1234=1S0xH2345=2S0xH3456=3";
        char buffer[512];
        rc_trigger_t* trigger = rc_parse_trigger(buffer, trigger_definition.c_str(), NULL, 0);
        CapturedTriggerHits hits;

        trigger->requirement->conditions->current_hits = 6U;
        trigger->alternative->conditions->current_hits = 7U;
        trigger->alternative->next->conditions->current_hits = 8U;
        trigger->has_hits = 1;

        hits.Capture(trigger, trigger_definition);

        trigger->requirement->conditions->current_hits = 8U;
        trigger->alternative->conditions->current_hits = 10U;
        trigger->alternative->next->conditions->current_hits = 15U;

        Assert::IsTrue(hits.Restore(trigger, trigger_definition));

        Assert::AreEqual(6U, trigger->requirement->conditions->current_hits);
        Assert::AreEqual(7U, trigger->alternative->conditions->current_hits);
        Assert::AreEqual(8U, trigger->alternative->next->conditions->current_hits);
        Assert::AreEqual({1}, trigger->has_hits);
    }

    TEST_METHOD(TestRestoreTriggerUncaptured)
    {
        const std::string trigger_definition = "0xH1234=1_0xH2345=2";
        char buffer[512];
        rc_trigger_t* trigger = rc_parse_trigger(buffer, trigger_definition.c_str(), NULL, 0);
        CapturedTriggerHits hits;

        trigger->requirement->conditions->current_hits = 6U;
        trigger->requirement->conditions->next->current_hits = 7U;
        trigger->has_hits = 1;

        Assert::IsFalse(hits.Restore(trigger, trigger_definition));

        Assert::AreEqual(6U, trigger->requirement->conditions->current_hits);
        Assert::AreEqual(7U, trigger->requirement->conditions->next->current_hits);
        Assert::AreEqual({1}, trigger->has_hits);
    }

    TEST_METHOD(TestRestoreTriggerAltered)
    {
        const std::string trigger_definition = "0xH1234=1_0xH2345=2";
        const std::string trigger_definition2 = "0xH1234=1_0xH2345=3";
        char buffer[512];
        rc_trigger_t* trigger = rc_parse_trigger(buffer, trigger_definition.c_str(), NULL, 0);
        CapturedTriggerHits hits;

        trigger->requirement->conditions->current_hits = 6U;
        trigger->requirement->conditions->next->current_hits = 7U;
        trigger->has_hits = 1;

        hits.Capture(trigger, trigger_definition);

        trigger->requirement->conditions->current_hits = 8U;
        trigger->requirement->conditions->next->current_hits = 10U;

        Assert::IsFalse(hits.Restore(trigger, trigger_definition2));

        Assert::AreEqual(8U, trigger->requirement->conditions->current_hits);
        Assert::AreEqual(10U, trigger->requirement->conditions->next->current_hits);
        Assert::AreEqual({1}, trigger->has_hits);
    }

    TEST_METHOD(TestRestoreTriggerZeroes)
    {
        const std::string trigger_definition = "0xH1234=1_0xH2345=2";
        char buffer[512];
        rc_trigger_t* trigger = rc_parse_trigger(buffer, trigger_definition.c_str(), NULL, 0);
        CapturedTriggerHits hits;

        hits.Capture(trigger, trigger_definition);

        trigger->requirement->conditions->current_hits = 8U;
        trigger->requirement->conditions->next->current_hits = 15U;

        Assert::IsTrue(hits.Restore(trigger, trigger_definition));

        Assert::AreEqual(0U, trigger->requirement->conditions->current_hits);
        Assert::AreEqual(0U, trigger->requirement->conditions->next->current_hits);
        Assert::AreEqual({0}, trigger->has_hits);
    }

    TEST_METHOD(TestRestoreValue)
    {
        const std::string value_definition = "M:0xH1234=1";
        char buffer[512];
        rc_value_t* value = rc_parse_value(buffer, value_definition.c_str(), NULL, 0);
        CapturedTriggerHits hits;

        value->conditions->conditions->current_hits = 6U;

        hits.Capture(value, value_definition);

        value->conditions->conditions->current_hits = 8U;

        Assert::IsTrue(hits.Restore(value, value_definition));

        Assert::AreEqual(6U, value->conditions->conditions->current_hits);
    }

    TEST_METHOD(TestRestoreValueAlts)
    {
        const std::string value_definition = "M:0xH1234=1$M:0xH2345=2$M:0xH3456=3";
        char buffer[512];
        rc_value_t* value = rc_parse_value(buffer, value_definition.c_str(), NULL, 0);
        CapturedTriggerHits hits;

        value->conditions->conditions->current_hits = 6U;
        value->conditions->next->conditions->current_hits = 7U;
        value->conditions->next->next->conditions->current_hits = 8U;

        hits.Capture(value, value_definition);

        value->conditions->conditions->current_hits = 8U;
        value->conditions->next->conditions->current_hits = 10U;
        value->conditions->next->next->conditions->current_hits = 15U;

        Assert::IsTrue(hits.Restore(value, value_definition));

        Assert::AreEqual(6U, value->conditions->conditions->current_hits);
        Assert::AreEqual(7U, value->conditions->next->conditions->current_hits);
        Assert::AreEqual(8U, value->conditions->next->next->conditions->current_hits);
    }

    TEST_METHOD(TestRestoreValueUncaptured)
    {
        const std::string value_definition = "M:0xH1234=1";
        char buffer[512];
        rc_value_t* value = rc_parse_value(buffer, value_definition.c_str(), NULL, 0);
        CapturedTriggerHits hits;

        value->conditions->conditions->current_hits = 6U;

        Assert::IsFalse(hits.Restore(value, value_definition));

        Assert::AreEqual(6U, value->conditions->conditions->current_hits);
    }

    TEST_METHOD(TestRestoreValueAltered)
    {
        const std::string value_definition = "M:0xH1234=1";
        const std::string value_definition2 = "M:0xH1234=2";
        char buffer[512];
        rc_value_t* value = rc_parse_value(buffer, value_definition.c_str(), NULL, 0);
        CapturedTriggerHits hits;

        value->conditions->conditions->current_hits = 6U;

        hits.Capture(value, value_definition);

        value->conditions->conditions->current_hits = 8U;

        Assert::IsFalse(hits.Restore(value, value_definition2));

        Assert::AreEqual(8U, value->conditions->conditions->current_hits);
    }

    TEST_METHOD(TestRestoreValueZeroes)
    {
        const std::string value_definition = "M:0xH1234=1";
        char buffer[512];
        rc_value_t* value = rc_parse_value(buffer, value_definition.c_str(), NULL, 0);
        CapturedTriggerHits hits;

        hits.Capture(value, value_definition);

        value->conditions->conditions->current_hits = 8U;

        Assert::IsTrue(hits.Restore(value, value_definition));

        Assert::AreEqual(0U, value->conditions->conditions->current_hits);
    }

};

} // namespace tests
} // namespace models
} // namespace data
} // namespace ra
