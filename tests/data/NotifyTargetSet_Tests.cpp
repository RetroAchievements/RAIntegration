#include "data\NotifyTargetSet.hh"

#include "ra_fwd.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ra {
namespace data {
namespace tests {

TEST_CLASS(NotifyTargetSet_Tests)
{
    using NotifyTargetSetString = NotifyTargetSet<std::string>;

public:
    TEST_METHOD(TestAddAndRemove)
    {
        NotifyTargetSetString set;
        std::string one = "one";
        std::string two = "two";
        std::string three = "three";
        std::string four = "four";

        Assert::AreEqual({ 0U }, set.Targets().size());

        set.Add(one);
        set.Add(two);
        Assert::AreEqual({ 2U }, set.Targets().size());

        set.Add(one);
        set.Add(three);
        Assert::AreEqual({ 3U }, set.Targets().size());

        set.Remove(two);
        Assert::AreEqual({ 2U }, set.Targets().size());

        set.Remove(two);
        set.Remove(one);
        Assert::AreEqual({ 1U }, set.Targets().size());

        set.Add(two);
        Assert::AreEqual({ 2U }, set.Targets().size());

        for (auto& str : set.Targets())
            str.push_back('!');

        Assert::AreEqual(std::string("one"), one);
        Assert::AreEqual(std::string("two!"), two);
        Assert::AreEqual(std::string("three!"), three);
        Assert::AreEqual(std::string("four"), four);

        set.Remove(three);
        set.Remove(two);
        Assert::AreEqual({ 0U }, set.Targets().size());

        for (auto& str : set.Targets())
            str.push_back('?');

        Assert::AreEqual(std::string("one"), one);
        Assert::AreEqual(std::string("two!"), two);
        Assert::AreEqual(std::string("three!"), three);
        Assert::AreEqual(std::string("four"), four);
    }

    TEST_METHOD(TestLockAndUnlock)
    {
        NotifyTargetSetString set;
        std::string one = "one";
        std::string two = "two";
        std::string three = "three";
        std::string four = "four";

        Assert::AreEqual({ 0U }, set.Targets().size());

        set.Lock();
        set.Add(one);
        Assert::AreEqual({ 0U }, set.Targets().size()); // add is delayed
        set.Unlock();
        Assert::AreEqual({ 1U }, set.Targets().size());

        set.Lock();
        set.Add(two);
        Assert::AreEqual({ 1U }, set.Targets().size());
        set.Unlock();
        Assert::AreEqual({ 2U }, set.Targets().size());

        set.Lock();
        set.Remove(one);
        Assert::AreEqual({ 2U }, set.Targets().size());
        set.Unlock();
        Assert::AreEqual({ 1U }, set.Targets().size());

        set.Lock();
        set.Remove(two);
        Assert::AreEqual({ 1U }, set.Targets().size());
        set.Unlock();
        Assert::AreEqual({ 0U }, set.Targets().size());
    }

    TEST_METHOD(TestLockIfNotEmpty)
    {
        NotifyTargetSetString set;
        std::string one = "one";
        std::string two = "two";
        std::string three = "three";
        std::string four = "four";

        Assert::AreEqual({ 0U }, set.Targets().size());

        // empty set not locked by LockIfNotEmpty
        Assert::AreEqual(false, set.LockIfNotEmpty());
        set.Add(one);
        Assert::AreEqual({ 1U }, set.Targets().size());

        // no-op. set is not locked
        set.Unlock();
        Assert::AreEqual({ 1U }, set.Targets().size());

        // non empty set is locked by LockIfNotEmpty
        Assert::AreEqual(true, set.LockIfNotEmpty());
        set.Add(two);
        Assert::AreEqual({ 1U }, set.Targets().size());
        set.Unlock();
        Assert::AreEqual({ 2U }, set.Targets().size());

        // non empty set is locked by LockIfNotEmpty
        Assert::AreEqual(true, set.LockIfNotEmpty());
        set.Remove(one);
        Assert::AreEqual({ 2U }, set.Targets().size());
        set.Unlock();
        Assert::AreEqual({ 1U }, set.Targets().size());

        // non empty set is locked by LockIfNotEmpty
        Assert::AreEqual(true, set.LockIfNotEmpty());
        set.Remove(two);
        Assert::AreEqual({ 1U }, set.Targets().size());
        set.Unlock();
        Assert::AreEqual({ 0U }, set.Targets().size());

        // empty set not locked by LockIfNotEmpty
        Assert::AreEqual(false, set.LockIfNotEmpty());
    }

    TEST_METHOD(TestLockAndUnlockMultiple)
    {
        NotifyTargetSetString set;
        std::string one = "one";
        std::string two = "two";
        std::string three = "three";
        std::string four = "four";

        Assert::AreEqual({ 0U }, set.Targets().size());

        set.Lock();
        set.Add(one);
        set.Add(two);
        Assert::AreEqual({ 0U }, set.Targets().size());
        set.Unlock();
        Assert::AreEqual({ 2U }, set.Targets().size());

        set.Lock();
        set.Add(three);
        set.Remove(one);
        Assert::AreEqual({ 2U }, set.Targets().size());
        set.Unlock();
        Assert::AreEqual({ 2U }, set.Targets().size());

        set.Lock();
        set.Add(three);
        set.Remove(one);
        Assert::AreEqual({ 2U }, set.Targets().size());
        set.Unlock();
        Assert::AreEqual({ 2U }, set.Targets().size());

        set.Lock();
        set.Add(four);
        set.Remove(four);
        Assert::AreEqual({ 2U }, set.Targets().size());
        set.Unlock();
        Assert::AreEqual({ 2U }, set.Targets().size());

        for (auto& str : set.Targets())
            str.push_back('!');

        Assert::AreEqual(std::string("one"), one);
        Assert::AreEqual(std::string("two!"), two);
        Assert::AreEqual(std::string("three!"), three);
        Assert::AreEqual(std::string("four"), four);
    }
};

} // namespace tests
} // namespace ui
} // namespace ra
