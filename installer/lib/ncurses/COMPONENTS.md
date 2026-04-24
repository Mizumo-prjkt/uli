# HarukaInstaller TUI Component Guide

This document provides a detailed overview of the ncurses-based UI components and instructions on how to leverage or port this codebase to other projects.

## 1. Core Components

### `NcursesLib` (The Foundation)
`ncurseslib.hpp/cpp` is a low-level wrapper around the ncurses library. 
- **Initialization**: Use `init_ncurses()` and `end_ncurses()`. It is recommended to use the `NcursesLib` object in a RAII fashion.
- **Color Pairs**: Pre-defines a color palette (e.g., `CP_NORMAL`, `CP_HIGHLIGHT`, `CP_POPUP_WINDOW`).
- **Input Helpers**: `text_input()` provides a robust field editor within ncurses windows.

### `MainMenu` (The Layout Engine)
`mainmenu.hpp/cpp` implements the primary TUI layout:
- **Structure**: Title bar (Top), Sidebar (Left), Content Panel (Right), and Status Bar (Bottom).
- **Navigation**: Automatically handles `UP/DOWN` for menu selection and `ENTER/TAB` for switching focus between the sidebar and content.
- **Modularity**: You add functionality by simply attaching `Page` objects via `add_page()`.

### `Page` (The Content Template)
Every screen in the installer is a class inheriting from `Page`.
- `render(WINDOW* win)`: Your drawing logic.
- `handle_input(WINDOW* win, int ch)`: Your interaction logic. Return `true` if the key was consumed.

---

## 2. Advanced UI Features

### `IsolateBuffer` (Visual Integrity)
`isolatebuffer.hpp` is an RAII-style tool that captures a snapshot of the current terminal screen.
- **Usage**: Instantiate it at the start of a function that opens a modal window (like a popup).
- **Benefit**: When the object is destroyed, the background is restored pixel-for-pixel, preventing "ghosting" or redraw artifacts in nested menus.

### Modal Popups (`popups.hpp`)
Standardized dialogs for common tasks:
- `ListSelectPopup`: Single or multi-selection with real-time search.
- `FormPopup`: Multi-field input forms with support for dynamic/conditional fields (e.g., Btrfs options appearing only when 'btrfs' is selected).
- `YesNoPopup`: Simple confirmation dialogs.
- `InputPopup`: Single-line text input.

---

## 3. Portability & Reuse

If you wish to use this TUI framework in your own project, follow these guidelines:

### Essential Files (The "Portable Core")
The following files are loosely coupled and can be moved to any C++17 project:
1. `ncurseslib.hpp / ncurseslib.cpp`
2. `mainmenu/isolatebuffer.hpp`
3. `popups.hpp` (Requires `ncurseslib.hpp` and `isolatebuffer.hpp`)

### Files Requiring Minor Changes
- **`datastore.hpp`**: This installer uses a singleton `DataStore` to manage state. In your project, you should replace references to `DataStore::instance()` with your own state management logic.
- **`mainmenu/mainmenu.cpp`**: You may want to adjust the `sidebar_width` or the `CP_` color pairs to match your own branding.

### Integration Steps
1. **Copy the Core**: Move the essential files listed above into your project.
2. **Setup CMake**: Ensure you link against `ncursesw` (wide character support) and set the include paths for the headers.
3. **Implement your Pages**: Create classes that inherit from `Page` and implement your custom logic.
4. **Initialize**:
   ```cpp
   NcursesLib n;
   n.init_ncurses();
   MainMenu m;
   m.add_page("My Tool", new MyPage());
   m.run();
   ```

## 4. Best Practices
- **Never hardcode sizes**: Use `getmaxyx()` inside `render()` and `handle_input()` to ensure your UI is responsive to terminal resizing.
- **Use IsolateBuffer**: Always wrap popup calls in an `IsolateBuffer` scope to ensure the parent window remains clean.
- **Wide Characters**: Always link against `ncursesw` and use `std::string` with UTF-8 to support international characters in the installer.
