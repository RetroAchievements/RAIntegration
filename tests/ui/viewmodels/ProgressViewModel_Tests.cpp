#include "CppUnitTest.h"

#include "ui\viewmodels\ProgressViewModel.hh"

#include "tests\devkit\services\mocks\MockThreadPool.hh"
#include "tests\ui\UIAsserts.hh"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ra {
namespace ui {
namespace viewmodels {
namespace tests {

TEST_CLASS(ProgressViewModel_Tests)
{
private:
    class ProgressViewModelHarness : public ProgressViewModel
    {
    public:
        ra::services::mocks::MockThreadPool mockThreadPool;
    };

public:
    TEST_METHOD(TestInitialValues)
    {
        ProgressViewModelHarness vmProgress;
        Assert::AreEqual(std::wstring(), vmProgress.GetMessage());
        Assert::AreEqual(0, vmProgress.GetProgress());
    }

    TEST_METHOD(TestSingleTask)
    {
        ProgressViewModelHarness vmProgress;
        int nTasksCompleted = 0;
        vmProgress.QueueTask([&nTasksCompleted]() { ++nTasksCompleted; });
        Assert::AreEqual({ 1U }, vmProgress.TaskCount());

        Assert::AreEqual(std::wstring(), vmProgress.GetMessage());
        Assert::AreEqual(0, vmProgress.GetProgress());
        Assert::AreEqual({ 0U }, vmProgress.mockThreadPool.PendingTasks());

        // setting visible starts the task threads
        vmProgress.SetIsVisible(true);
        Assert::AreEqual(DialogResult::None, vmProgress.GetDialogResult());

        Assert::AreEqual({ 1U }, vmProgress.TaskCount());
        Assert::AreEqual(0, vmProgress.GetProgress());

        // "run" the background thread - will execute the single task and then go away
        Assert::AreEqual({ 1U }, vmProgress.mockThreadPool.PendingTasks());
        vmProgress.mockThreadPool.ExecuteNextTask();

        Assert::AreEqual({ 0U }, vmProgress.mockThreadPool.PendingTasks());
        Assert::AreEqual(1, nTasksCompleted);
        Assert::AreEqual(100, vmProgress.GetProgress());

        // dialog should automatically close once all tasks are completed
        Assert::AreEqual(DialogResult::OK, vmProgress.GetDialogResult());

        // simulate the hiding - tasks should not restart
        vmProgress.SetIsVisible(false);
        Assert::AreEqual({ 0U }, vmProgress.mockThreadPool.PendingTasks());
    }

    TEST_METHOD(TestManyTasks)
    {
        ProgressViewModelHarness vmProgress;
        int nTasksCompleted = 0;
        for (int i = 0; i < 20; ++i)
        {
            vmProgress.QueueTask([&vmProgress, &nTasksCompleted, i]()
            {
                Assert::AreEqual(i * 5, vmProgress.GetProgress());
                ++nTasksCompleted;
            });
        }
        Assert::AreEqual({ 20U }, vmProgress.TaskCount());

        Assert::AreEqual(std::wstring(), vmProgress.GetMessage());
        Assert::AreEqual(0, vmProgress.GetProgress());
        Assert::AreEqual({ 0U }, vmProgress.mockThreadPool.PendingTasks());

        // setting visible starts the task threads
        vmProgress.SetIsVisible(true);
        Assert::AreEqual(DialogResult::None, vmProgress.GetDialogResult());

        Assert::AreEqual({ 20U }, vmProgress.TaskCount());
        Assert::AreEqual(0, vmProgress.GetProgress());

        // there should be four background threads queued, but because they're only mocked
        // threads, as soon as we start the first thread, it will process the entire queue.
        Assert::AreEqual({ 4U }, vmProgress.mockThreadPool.PendingTasks());
        vmProgress.mockThreadPool.ExecuteNextTask();

        Assert::AreEqual(20, nTasksCompleted);
        Assert::AreEqual(100, vmProgress.GetProgress());

        // dialog should automatically close once all tasks are completed
        Assert::AreEqual(DialogResult::OK, vmProgress.GetDialogResult());

        // the other three background threads will be starved in the mock pool. flush them too
        Assert::AreEqual({ 3U }, vmProgress.mockThreadPool.PendingTasks());
        vmProgress.mockThreadPool.ExecuteNextTask();
        vmProgress.mockThreadPool.ExecuteNextTask();
        vmProgress.mockThreadPool.ExecuteNextTask();
        Assert::AreEqual({ 0U }, vmProgress.mockThreadPool.PendingTasks());

        // make sure we're still in the "end" state
        Assert::AreEqual(20, nTasksCompleted);
        Assert::AreEqual(100, vmProgress.GetProgress());
        Assert::AreEqual(DialogResult::OK, vmProgress.GetDialogResult());
    }

    TEST_METHOD(TestCancel)
    {
        ProgressViewModelHarness vmProgress;
        int nTasksCompleted = 0;
        for (int i = 0; i < 20; ++i)
        {
            vmProgress.QueueTask([&vmProgress, &nTasksCompleted, i]()
            {
                ++nTasksCompleted;
                if (i == 8) // 0-8 is 9 completed tasks
                    vmProgress.SetDialogResult(DialogResult::Cancel);
            });
        }
        Assert::AreEqual({ 20U }, vmProgress.TaskCount());

        Assert::AreEqual(std::wstring(), vmProgress.GetMessage());
        Assert::AreEqual(0, vmProgress.GetProgress());
        Assert::AreEqual({ 0U }, vmProgress.mockThreadPool.PendingTasks());

        // setting visible starts the task threads
        vmProgress.SetIsVisible(true);
        Assert::AreEqual(DialogResult::None, vmProgress.GetDialogResult());

        Assert::AreEqual({ 20U }, vmProgress.TaskCount());
        Assert::AreEqual(0, vmProgress.GetProgress());

        // there should be four background threads queued, but because they're only mocked
        // threads, as soon as we start the first thread, it will process the entire queue.
        Assert::AreEqual({ 4U }, vmProgress.mockThreadPool.PendingTasks());
        vmProgress.mockThreadPool.ExecuteNextTask();

        Assert::AreEqual(9, nTasksCompleted);
        Assert::AreEqual(40, vmProgress.GetProgress()); // the 9th task won't be counted because the loop aborts first

        // dialog should automatically close once all tasks are completed
        Assert::AreEqual(DialogResult::Cancel, vmProgress.GetDialogResult());

        // the other three background threads will be starved in the mock pool. flush them too
        Assert::AreEqual({ 3U }, vmProgress.mockThreadPool.PendingTasks());
        vmProgress.mockThreadPool.ExecuteNextTask();
        vmProgress.mockThreadPool.ExecuteNextTask();
        vmProgress.mockThreadPool.ExecuteNextTask();
        Assert::AreEqual({ 0U }, vmProgress.mockThreadPool.PendingTasks());

        // make sure we're still in the "end" state
        Assert::AreEqual(9, nTasksCompleted);
        Assert::AreEqual(40, vmProgress.GetProgress());
        Assert::AreEqual(DialogResult::Cancel, vmProgress.GetDialogResult());

        // simulate the hiding - tasks should not restart
        vmProgress.SetIsVisible(false);
        Assert::AreEqual({ 0U }, vmProgress.mockThreadPool.PendingTasks());
        Assert::AreEqual(9, nTasksCompleted);
        Assert::AreEqual(40, vmProgress.GetProgress());
        Assert::AreEqual(DialogResult::Cancel, vmProgress.GetDialogResult());
    }
};

} // namespace tests
} // namespace viewmodels
} // namespace ui
} // namespace ra
