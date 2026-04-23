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

```cpp
#include "ncurseslib.hpp"

int main() {
    NcursesLib ncurseslib;
    ncurseslib.init_ncurses();
    ncurseslib.print_string("Hello World");
    ncurseslib.refresh();
    ncurseslib.getch();
    ncurseslib.end_ncurses();
    return 0;
}
```

## Test

If you plan to test the UI of ncurses, compile with -DTESTUI or use the build script in the root directory to build the test UI, as that build script already includes the flag.

Note that compiling the menu library to test, Test is **REQUIRED** to be compiled as an object.

`$ROOT/installer/lib/ncurses/test/testing.cpp` is a test program for the ncurses menu.

`$ROOT/installer/lib/ncurses/test/simulation_test.hpp` is a test header file for the ncurses menu. This is to also help compile the menu lib and addresses the absence of logic of detecting hardware, this is served as a logic placeholder since the code of the TUI is dependant on logic of other libraries for HarukaInstaller. Testing only the UI without logic can likely lead to segmentation fault errors. So, simulation_test.hpp is needed, as a simulation of a hardware.

If no testing is needed, do not invoke -DTESTUI. 