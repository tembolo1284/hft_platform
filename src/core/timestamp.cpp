#include "timestamp.h"
#include <chrono>
#include <iomanip>
#include <sstream>

namespace hft {
namespace core {

Timestamp TimestampUtil::now() {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
}

uint64_t TimestampUtil::to_micros(Timestamp ts) {
    return ts / 1000;
}

uint64_t TimestampUtil::to_millis(Timestamp ts) {
    return ts / 1000000;
}

int64_t TimestampUtil::diff_nanos(Timestamp t1, Timestamp t2) {
    return static_cast<int64_t>(t1) - static_cast<int64_t>(t2);
}

int64_t TimestampUtil::diff_micros(Timestamp t1, Timestamp t2) {
    return diff_nanos(t1, t2) / 1000;
}

std::string TimestampUtil::to_string(Timestamp ts) {
    auto time_point = std::chrono::system_clock::time_point(std::chrono::nanoseconds(ts));
    auto time_t = std::chrono::system_clock::to_time_t(time_point);
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    
    auto nanos = ts % 1000000000;
    ss << "." << std::setfill('0') << std::setw(9) << nanos;
    
    return ss.str();
}

Timestamp TimestampUtil::from_string(const std::string& str) {
    return now();
}

} // namespace core
} // namespace hft
