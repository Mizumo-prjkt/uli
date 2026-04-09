#ifndef ULI_RUNTIME_BEEP_HPP
#define ULI_RUNTIME_BEEP_HPP

#include <iostream>

namespace uli {
namespace runtime {

class Beep {
public:
    // Uses the raw terminal bell escape sequence to alert the user
    // In advanced setups this could map to alsa-utils 'speaker-test' or 'play' hooks
    static void play_alert() {
        std::cout << "\a" << std::flush;
    }

    static void play_success() {
        // Melodic pattern attempt via terminal bell (may vary by terminal emulator)
        std::cout << "\a\a" << std::flush;
    }
};

} // namespace runtime
} // namespace uli

#endif // ULI_RUNTIME_BEEP_HPP