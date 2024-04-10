#include "ControlBinding.hh"

#include "data\context\EmulatorContext.hh"

#include "services\ServiceLocator.hh"
#include "services\IClock.hh"

#include "ui\viewmodels\MessageBoxViewModel.hh"

#include "ui\win32\Desktop.hh"

namespace ra {
namespace ui {
namespace win32 {
namespace bindings {

static std::mutex g_pSuspendRepaintMutex;
static std::atomic_int g_nSuspendRepaintCount = 0;
static std::set<HWND> g_vSuspendedRepaintHWnds;
bool ControlBinding::s_bNeedsUpdateWindow = false;

static std::vector<ControlBinding*> s_vSubclassedBindings;

ControlBinding::~ControlBinding() noexcept
{
    DisableBinding();

    UnsubclassWndProc();
}

void ControlBinding::ForceRepaint(HWND hWnd)
{
    ::InvalidateRect(hWnd, nullptr, FALSE);

    if (NeedsUpdateWindow())
        RedrawWindow(hWnd);
}

void ControlBinding::RedrawWindow(HWND hWnd)
{
    if (g_nSuspendRepaintCount == 0)
    {
        ::UpdateWindow(hWnd);
    }
    else
    {
        std::lock_guard<std::mutex> guard(g_pSuspendRepaintMutex);
        g_vSuspendedRepaintHWnds.insert(hWnd);
    }
}

void ControlBinding::SuspendRepaint()
{
    if (++g_nSuspendRepaintCount == 50)
        ra::ui::viewmodels::MessageBoxViewModel::ShowWarningMessage(L"SuspendRepaint count is abnormally high.", L"Make sure to call ResumeRepaint whenever you call SuspendRepaint.");
}

void ControlBinding::ResumeRepaint()
{
    if (--g_nSuspendRepaintCount == 0)
    {
        std::set<HWND> vSuspendedRepaintWnds;
        {
            std::lock_guard<std::mutex> guard(g_pSuspendRepaintMutex);
            vSuspendedRepaintWnds.swap(g_vSuspendedRepaintHWnds);
        }

        if (!vSuspendedRepaintWnds.empty())
        {
            for (HWND hWnd : vSuspendedRepaintWnds)
                ::UpdateWindow(hWnd);
        }
    }
    else
    {
        Expects(g_nSuspendRepaintCount > 0);
    }
}

_NODISCARD static INT_PTR CALLBACK ControlBindingWndProc(HWND hControl, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    ControlBinding* pBinding{};
    GSL_SUPPRESS_TYPE1 pBinding = reinterpret_cast<ControlBinding*>(GetWindowLongPtr(hControl, GWLP_USERDATA));

    if (pBinding != nullptr)
        return pBinding->WndProc(hControl, uMsg, wParam, lParam);

    return 0;
}

void ControlBinding::SubclassWndProc()
{
    if (m_pOriginalWndProc != nullptr)
        return;

    s_vSubclassedBindings.push_back(this);

    GSL_SUPPRESS_TYPE1 SetWindowLongPtr(m_hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    GSL_SUPPRESS_TYPE1 m_pOriginalWndProc = reinterpret_cast<WNDPROC>(
        SetWindowLongPtr(m_hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&ControlBindingWndProc)));
}

void ControlBinding::UnsubclassWndProc() noexcept
{
    if (m_pOriginalWndProc)
    {
        GSL_SUPPRESS_TYPE1 SetWindowLongPtr(m_hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(m_pOriginalWndProc));
        SetWindowLongPtr(m_hWnd, GWLP_USERDATA, 0);

        for (auto iter = s_vSubclassedBindings.begin(); iter != s_vSubclassedBindings.end(); ++iter)
        {
            if (*iter == this)
            {
                s_vSubclassedBindings.erase(iter);
                break;
            }
        }

        m_pOriginalWndProc = nullptr;
    }
}

void ControlBinding::DetachSubclasses() noexcept
{
    std::vector<ControlBinding*> vSubclassedBindings;
    vSubclassedBindings.swap(s_vSubclassedBindings);

    for (auto* pBinding : vSubclassedBindings)
    {
        if (pBinding != nullptr)
            pBinding->UnsubclassWndProc();
    }
}

_Use_decl_annotations_
INT_PTR CALLBACK ControlBinding::WndProc(HWND hControl, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return CallWindowProc(m_pOriginalWndProc, hControl, uMsg, wParam, lParam);
}

} // namespace bindings
} // namespace win32
} // namespace ui
} // namespace ra
