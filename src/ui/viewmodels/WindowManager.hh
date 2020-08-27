#ifndef RA_UI_WINDOW_MANAGER
#define RA_UI_WINDOW_MANAGER

#include "AssetListViewModel.hh"
#include "CodeNotesViewModel.hh"
#include "EmulatorViewModel.hh"
#include "MemoryBookmarksViewModel.hh"
#include "MemoryInspectorViewModel.hh"
#include "RichPresenceMonitorViewModel.hh"

namespace ra {
namespace ui {
namespace viewmodels {

class WindowManager {
public:
    EmulatorViewModel Emulator;
    RichPresenceMonitorViewModel RichPresenceMonitor;
    AssetListViewModel AssetList;
    MemoryBookmarksViewModel MemoryBookmarks;
    MemoryInspectorViewModel MemoryInspector;
    CodeNotesViewModel CodeNotes;
};

} // namespace viewmodels
} // namespace ui
} // namespace ra

#endif // !RA_UI_WINDOW_MANAGER
