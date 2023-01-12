#ifndef RA_UI_WINDOW_MANAGER
#define RA_UI_WINDOW_MANAGER

#include "AssetEditorViewModel.hh"
#include "AssetListViewModel.hh"
#include "CodeNotesViewModel.hh"
#include "EmulatorViewModel.hh"
#include "MemoryBookmarksViewModel.hh"
#include "MemoryInspectorViewModel.hh"
#include "PointerFinderViewModel.hh"
#include "RichPresenceMonitorViewModel.hh"

namespace ra {
namespace ui {
namespace viewmodels {

class WindowManager {
public:
    EmulatorViewModel Emulator;
    RichPresenceMonitorViewModel RichPresenceMonitor;
    AssetListViewModel AssetList;
    AssetEditorViewModel AssetEditor;
    MemoryBookmarksViewModel MemoryBookmarks;
    MemoryInspectorViewModel MemoryInspector;
    CodeNotesViewModel CodeNotes;
    PointerFinderViewModel PointerFinder;
};

} // namespace viewmodels
} // namespace ui
} // namespace ra

#endif // !RA_UI_WINDOW_MANAGER
