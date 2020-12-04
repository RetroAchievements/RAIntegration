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
    auto pItem = std::make_unique<TaskItem>();
    pItem->fTaskHandler = std::move(fTaskHandler);

    std::lock_guard<std::mutex> pGuard(m_pMutex);
    m_vTasks.emplace_back(std::move(pItem));
}

void ProgressViewModel::OnValueChanged(const BoolModelProperty::ChangeArgs& args)
{
    if (args.Property == IsVisibleProperty && args.tNewValue)
        BeginTasks();
}

void ProgressViewModel::BeginTasks()
{
    if (m_vTasks.empty())
    {
        SetDialogResult(DialogResult::OK);
        return;
    }

    OnBegin();

    // use up to four threads to process the queue
    constexpr int MAX_THREADS = 4;
    auto& pThreadPool = ra::services::ServiceLocator::GetMutable<ra::services::IThreadPool>();
    for (int i = 0; i < std::min(gsl::narrow_cast<int>(m_vTasks.size()), MAX_THREADS); ++i)
    {
        pThreadPool.RunAsync([this, pAsyncHandle = CreateAsyncHandle()]()
        {
            ra::data::AsyncKeepAlive pKeepAlive(*pAsyncHandle);
            if (!pAsyncHandle->IsDestroyed())
                ProcessQueue();
        });
    }
}

void ProgressViewModel::ProcessQueue()
{
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
                if (pItem->nState == TaskState::None && !pTask)
                {
                    pItem->nState = TaskState::Running;
                    pTask = pItem.get();
                }
                else if (pItem->nState == TaskState::Done)
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
            OnComplete();
        }
    } while (true);
}

void ProgressViewModel::OnComplete()
{
    SetDialogResult(DialogResult::OK);
}

} // namespace viewmodels
} // namespace ui
} // namespace ra
