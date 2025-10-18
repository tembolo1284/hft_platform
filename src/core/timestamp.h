#ifndef HFT_CORE_TIMESTAMP_H
#define HFT_CORE_TIMESTAMP_H

#include <cstdint>
#include <chrono>
#include <string>

namespace hft {
namespace core {

using Timestamp = uint64_t;

class TimestampUtil {
public:
    static Timestamp now();
    static uint64_t to_micros(Timestamp ts);
    static uint64_t to_millis(Timestamp ts);
    static int64_t diff_nanos(Timestamp t1, Timestamp t2);
    static int64_t diff_micros(Timestamp t1, Timestamp t2);
    static std::string to_string(Timestamp ts);
    static Timestamp from_string(const std::string& str);
};

} // namespace core
} // namespace hft

#endif // HFT_CORE_TIMESTAMP_H
