# ncurses wrapper for HarukaInstaller

This wrapper is for HarukaInstaller to use ncurses.

This directory has `ncurseslib.cpp` and `ncurseslib.hpp`. 

Note to compile this as object file. But you can always compile this as a standalone program to test it out for interactivity, as long as flag -DTESTUI is present.

`build.sh` should have that already at `$ROOT/installer`

`ncurseslib.cpp` contains the implementation of the ncurses wrapper.
`ncurseslib.hpp` contains the declaration of the ncurses wrapper.

## How to Compile

```bash
g++ -Ilib/ncurses -Llib/ncurses -lncurses -o ncurseslib ncurseslib.cpp
```

## How to Use

To use the TUI, you can utilize the `MainMenu` system along with custom `Page` implementations. The `DataStore` singleton is also available for persistent state management.

```cpp
#include "ncurseslib.hpp"
#include "mainmenu/mainmenu.hpp"
#include "mainmenu/pages/page.hpp"
#include "configurations/datastore.hpp"

// Create a custom page
class MyPage : public Page {
public:
    std::string title() const override { return "My Custom Page"; }
    
    void render(WINDOW* win) override {
        wattron(win, COLOR_PAIR(CP_SECTION_TITLE) | A_BOLD);
        mvwprintw(win, 1, 2, "Welcome to My Page");
        wattroff(win, COLOR_PAIR(CP_SECTION_TITLE) | A_BOLD);
        
        mvwprintw(win, 3, 2, "Press ENTER to return to the main menu.");
    }
    
    bool handle_input(WINDOW* win, int ch) override {
        (void)win;
        if (ch == '\n' || ch == KEY_ENTER) {
            return true; // Return true to signal we want to go back to the menu
        }
        return false;
    }
};

int main() {
    // 1. Initialize ncurses
    NcursesLib ncurses;
    ncurses.init_ncurses();

    // 2. Build the main menu
    MainMenu menu;
    menu.add_page("Open My Page", new MyPage());
    
    menu.add_separator();
    menu.add_action("Save Config");
    menu.add_action("Exit");

    // 3. Run the menu loop
    menu.run();

    // 4. Cleanup
    ncurses.end_ncurses();
    return 0;
}
```

## Test

If you plan to test the UI of ncurses, compile with -DTESTUI or use the build script in the root directory to build the test UI, as that build script already includes the flag.

Note that compiling the menu library to test, Test is **REQUIRED** to be compiled as an object.

`$ROOT/installer/lib/ncurses/test/testing.cpp` is a test program for the ncurses menu.

`$ROOT/installer/lib/ncurses/test/simulation_test.hpp` is a test header file for the ncurses menu. This is to also help compile the menu lib and addresses the absence of logic of detecting hardware, this is served as a logic placeholder since the code of the TUI is dependant on logic of other libraries for HarukaInstaller. Testing only the UI without logic can likely lead to segmentation fault errors. So, simulation_test.hpp is needed, as a simulation of a hardware.

If no testing is needed, do not invoke -DTESTUI. 