#ifndef RA_UI_WINDOW_MANAGER
#define RA_UI_WINDOW_MANAGER

#include "CodeNotesViewModel.hh"
#include "EmulatorViewModel.hh"
#include "MemoryBookmarksViewModel.hh"
#include "RichPresenceMonitorViewModel.hh"

namespace ra {
namespace ui {
namespace viewmodels {

class WindowManager {
public:
    EmulatorViewModel Emulator;
    RichPresenceMonitorViewModel RichPresenceMonitor;
    MemoryBookmarksViewModel MemoryBookmarks;
    CodeNotesViewModel CodeNotes;
};

} // namespace viewmodels
} // namespace ui
} // namespace ra

#endif // !RA_UI_WINDOW_MANAGER
