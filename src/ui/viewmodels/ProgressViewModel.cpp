#include "ProgressViewModel.hh"

#include "services\IThreadPool.hh"
#include "services\ServiceLocator.hh"

namespace ra {
namespace ui {
namespace viewmodels {

const StringModelProperty ProgressViewModel::MessageProperty("ProgressViewModel", "Message", L"");
const IntModelProperty ProgressViewModel::ProgressProperty("ProgressViewModel", "Progress", 0);

void ProgressViewModel::QueueTask(std::function<void()>&& fTaskHandler)
{
    std::lock_guard<std::mutex> pGuard(m_pMutex);

    auto& pItem = m_vTasks.emplace_back();
    pItem.fTaskHandler = std::move(fTaskHandler);
}

void ProgressViewModel::OnValueChanged(const BoolModelProperty::ChangeArgs& args)
{
    if (args.Property == IsVisibleProperty)
        BeginTasks();
}

void ProgressViewModel::BeginTasks()
{
    if (m_vTasks.empty())
    {
        SetDialogResult(DialogResult::OK);
        return;
    }

    // use up to four threads to process the queue
    auto& pThreadPool = ra::services::ServiceLocator::GetMutable<ra::services::IThreadPool>();
    for (int i = 0; i < std::min(gsl::narrow_cast<int>(m_vTasks.size()), 4); ++i)
    {
        pThreadPool.RunAsync([this, pAsyncHandle = CreateAsyncHandle()]()
        {
            ProcessQueue(*pAsyncHandle);
        });
    }
}

void ProgressViewModel::ProcessQueue(ra::data::AsyncHandle& pAsyncHandle)
{
    ra::data::AsyncKeepAlive pKeepAlive(pAsyncHandle);
    if (pAsyncHandle.IsDestroyed())
        return;

    do
    {
        TaskItem* pTask = nullptr;
        int nComplete = 0;
        {
            std::lock_guard<std::mutex> pGuard(m_pMutex);
            if (m_bQueueComplete || GetDialogResult() != DialogResult::None)
                return;

            for (auto& pItem : m_vTasks)
            {
                if (pItem.nState == TaskState::None && !pTask)
                {
                    pItem.nState = TaskState::Running;
                    pTask = &pItem;
                }
                else if (pItem.nState == TaskState::Done)
                {
                    ++nComplete;
                }
            }

            if (pTask == nullptr)
                m_bQueueComplete = true;

            nComplete = nComplete * 100 / gsl::narrow_cast<int>(m_vTasks.size());
        }

        SetValue(ProgressProperty, nComplete);

        if (pTask != nullptr)
        {
            pTask->fTaskHandler();
            pTask->nState = TaskState::Done;
        }
        else if (m_bQueueComplete)
        {
            SetDialogResult(DialogResult::OK);
        }
    } while (true);
}

} // namespace viewmodels
} // namespace ui
} // namespace ra
