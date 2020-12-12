#ifndef RA_UI_PROGRESS_VIEW_MODEL_H
#define RA_UI_PROGRESS_VIEW_MODEL_H
#pragma once

#include "data\AsyncObject.hh"

#include "ui\WindowViewModelBase.hh"

#include "data\models\AchievementModel.hh"

namespace ra {
namespace ui {
namespace viewmodels {

class ProgressViewModel : public WindowViewModelBase, protected ra::data::AsyncObject
{
public:
    ProgressViewModel() noexcept {}
    ~ProgressViewModel()
    {
        ra::data::AsyncObject::BeginDestruction();
    }

    ProgressViewModel(const ProgressViewModel&) noexcept = delete;
    ProgressViewModel& operator=(const ProgressViewModel&) noexcept = delete;
    ProgressViewModel(ProgressViewModel&&) noexcept = delete;
    ProgressViewModel& operator=(ProgressViewModel&&) noexcept = delete;


    /// <summary>
    /// The <see cref="ModelProperty" /> for the message.
    /// </summary>
    static const StringModelProperty MessageProperty;

    /// <summary>
    /// Gets the message.
    /// </summary>
    const std::wstring& GetMessage() const { return GetValue(MessageProperty); }

    /// <summary>
    /// Sets the message.
    /// </summary>
    void SetMessage(const std::wstring& sValue) { SetValue(MessageProperty, sValue); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the current progress.
    /// </summary>
    static const IntModelProperty ProgressProperty;

    /// <summary>
    /// Gets the current progress (0-100).
    /// </summary>
    const int GetProgress() const { return GetValue(ProgressProperty); }

    void QueueTask(std::function<void()>&& fTaskHandler);

    const size_t TaskCount() const noexcept { return m_vTasks.size(); }

protected:
    void OnValueChanged(const BoolModelProperty::ChangeArgs& args) override;

    virtual void OnBegin() noexcept(false) {}
    virtual void OnComplete();

private:
    void BeginTasks();
    void ProcessQueue();

    enum class TaskState
    {
        None = 0,
        Running,
        Done
    };

    struct TaskItem
    {
        std::function<void()> fTaskHandler;
        TaskState nState = TaskState::None;
    };

    std::vector<std::unique_ptr<TaskItem>> m_vTasks;
    std::mutex m_pMutex;
    bool m_bQueueComplete = false;
};

} // namespace viewmodels
} // namespace ui
} // namespace ra

#endif RA_UI_PROGRESS_VIEW_MODEL_H
