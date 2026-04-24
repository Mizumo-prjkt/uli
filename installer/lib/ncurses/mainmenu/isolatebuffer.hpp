#pragma once
#include <ncurses.h>

class IsolateBuffer {
    WINDOW* snapshot_;
public:
    IsolateBuffer() {
        snapshot_ = dupwin(curscr);
    }
    ~IsolateBuffer() {
        if (snapshot_) {
            overwrite(snapshot_, stdscr);
            touchwin(stdscr);
            refresh();
            delwin(snapshot_);
        }
    }
    void restore() {
        if (snapshot_) {
            overwrite(snapshot_, stdscr);
            touchwin(stdscr);
            refresh();
        }
    }
};
