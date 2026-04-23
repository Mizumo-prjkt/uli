#pragma once
// sessionlock.hpp - Handles unsaved changes prompting for page navigation
#include "pages/page.hpp"
#include "../popups.hpp"

class SessionLock {
public:
    // Checks if the given page has pending changes. If it does, prompts the user.
    // Returns true if navigation can proceed (either no changes, or user chose to discard).
    // Returns false if navigation should be cancelled.
    static bool check_and_prompt(Page* page) {
        if (page && page->has_pending_changes()) {
            bool proceed = YesNoPopup::show(
                "Unsaved Changes",
                "You have unsaved changes in this section.",
                "Proceed without saving and discard changes?"
            );
            if (proceed) {
                page->discard_pending_changes();
                return true;
            }
            return false;
        }
        return true;
    }
};
