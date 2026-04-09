#ifndef ULI_RUNTIME_BLACKBOX_HPP
#define ULI_RUNTIME_BLACKBOX_HPP

#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <iomanip>
#include <ctime>
#include <mutex>

namespace uli {
namespace runtime {

class BlackBox {
private:
    static std::string get_log_path() {
        return "/tmp/uli_blackbox.log";
    }

    static std::string get_timestamp() {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
        
        std::stringstream ss;
        ss << "[" << std::put_time(std::localtime(&time), "%H:%M:%S") 
           << "." << std::setfill('0') << std::setw(3) << ms.count() << "] ";
        return ss.str();
    }

public:
    static void log(const std::string& message) {
        static std::mutex log_mutex;
        std::lock_guard<std::mutex> lock(log_mutex);
        
        std::ofstream log_file(get_log_path(), std::ios::app);
        if (log_file.is_open()) {
            log_file << get_timestamp() << message << std::endl;
            log_file.flush();
            log_file.close();
        }
    }

    static void init() {
        std::ofstream log_file(get_log_path(), std::ios::trunc);
        if (log_file.is_open()) {
            log_file << "=== ULI BLACKBOX SESSION STARTED ===" << std::endl;
            log_file.flush();
            log_file.close();
        }
    }
};

} // namespace runtime
} // namespace uli

#endif // ULI_RUNTIME_BLACKBOX_HPP
