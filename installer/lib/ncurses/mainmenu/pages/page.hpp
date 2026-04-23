#pragma once
// page.hpp - Base interface for all content pages in HarukaInstaller TUI
#include <ncurses.h>
#include <string>

class Page {
public:
    virtual ~Page() = default;

    // Render this page's content into the given window.
    // The window is pre-cleared and has its background set.
    virtual void render(WINDOW* win) = 0;

    // Handle a keypress while this page has focus.
    // Return true if the input was consumed, false to let MainMenu handle it.
    virtual bool handle_input(WINDOW* win, int ch) = 0;

    // The page title shown in the content panel header.
    virtual std::string title() const = 0;

    // Session Locking interface
    // Return true if the page has unsaved changes that should block navigation.
    virtual bool has_pending_changes() const { return false; }
    
    // Discard any pending changes, resetting the page to a clean state.
    virtual void discard_pending_changes() {}
};
