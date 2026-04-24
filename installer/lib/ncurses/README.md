# ncurses wrapper for HarukaInstaller

This wrapper is for HarukaInstaller to use ncurses.

This directory has `ncurseslib.cpp` and `ncurseslib.hpp`. 

Note to compile this as object file. But you can always compile this as a standalone program to test it out for interactivity, as long as flag -DTESTUI is present.

`build.sh` should have that already at `$ROOT/installer`

`ncurseslib.cpp` contains the implementation of the ncurses wrapper.
`ncurseslib.hpp` contains the declaration of the ncurses wrapper.

## How to Compile

The preferred way to build the project is using the root `build.sh` script, which wraps CMake:

```bash
# Build the TUI test executable
./build.sh test-ui

# Clean build artifacts
./build.sh clean
```

## How to Use

To use the TUI, you can utilize the `MainMenu` system along with custom `Page` implementations. The `DataStore` singleton is also available for persistent state management.

```cpp
#include "ncurseslib.hpp"
#include "mainmenu/mainmenu.hpp"
#include "mainmenu/pages/page.hpp"
#include "mainmenu/isolatebuffer.hpp"
#include "popups.hpp"

// Create a custom page
class MyPage : public Page {
public:
    std::string title() const override { return "My Custom Page"; }
    
    void render(WINDOW* win) override {
        wattron(win, COLOR_PAIR(CP_SECTION_TITLE) | A_BOLD);
        mvwprintw(win, 1, 2, "Welcome to My Page");
        wattroff(win, COLOR_PAIR(CP_SECTION_TITLE) | A_BOLD);
        
        mvwprintw(win, 3, 2, "Press ENTER to open a popup.");
    }
    
    bool handle_input(WINDOW* win, int ch) override {
        if (ch == '\n' || ch == KEY_ENTER) {
            // Use IsolateBuffer to prevent artifacts when opening popups
            std::vector<ListOption> opts = {{"Option 1", "v1"}, {"Option 2", "v2"}};
            ListSelectPopup::show("My Popup", {"Choose an option"}, opts, false, false);
            return true; 
        }
        return false;
    }
};

int main() {
    // Initialize ncurses RAII wrapper
    NcursesLib ncurses;
    ncurses.init_ncurses();

    // Setup the main menu layout
    MainMenu menu;
    menu.add_page("Settings", new MyPage());
    menu.add_separator();
    menu.add_action("Exit");

    // Enter the interactive loop
    menu.run();

    return 0; // ncurses is cleaned up automatically by NcursesLib destructor
}
```

## Test

If you plan to test the UI of ncurses, compile with -DTESTUI or use the build script in the root directory to build the test UI, as that build script already includes the flag.

Note that compiling the menu library to test, Test is **REQUIRED** to be compiled as an object.

`$ROOT/installer/lib/ncurses/test/testing.cpp` is a test program for the ncurses menu.

`$ROOT/installer/lib/ncurses/test/simulation_test.hpp` is a test header file for the ncurses menu. This is to also help compile the menu lib and addresses the absence of logic of detecting hardware, this is served as a logic placeholder since the code of the TUI is dependant on logic of other libraries for HarukaInstaller. Testing only the UI without logic can likely lead to segmentation fault errors. So, simulation_test.hpp is needed, as a simulation of a hardware.

If no testing is needed, do not invoke -DTESTUI. 

