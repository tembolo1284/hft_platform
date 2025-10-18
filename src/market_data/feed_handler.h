#ifndef HFT_MARKET_DATA_FEED_HANDLER_H
#define HFT_MARKET_DATA_FEED_HANDLER_H

#include "tick_parser.h"
#include "../core/types.h"
#include <functional>
#include <thread>
#include <atomic>
#include <string>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>

namespace hft {
namespace market_data {

// Callback types for market data events
using TickCallback = std::function<void(const Tick&)>;
using ErrorCallback = std::function<void(const std::string&)>;
using ConnectionCallback = std::function<void(bool connected)>;

// Feed handler configuration
struct FeedConfig {
    std::string multicast_group;     // Multicast group IP (e.g., "239.1.1.1")
    uint16_t multicast_port;         // Multicast port
    std::string interface_ip;        // Local interface IP to bind to
    size_t buffer_size;              // Receive buffer size in bytes
    bool enable_timestamping;        // Add kernel timestamps to packets
    int max_queue_size;              // Max size of internal tick queue
    
    FeedConfig()
        : multicast_group("239.1.1.1")
        , multicast_port(9000)
        , interface_ip("0.0.0.0")
        , buffer_size(65536)  // 64KB
        , enable_timestamping(true)
        , max_queue_size(10000)
    {}
};

// Statistics for monitoring feed health
struct FeedStatistics {
    uint64_t packets_received;
    uint64_t packets_dropped;
    uint64_t ticks_processed;
    uint64_t parse_errors;
    uint64_t queue_overflows;
    core::Timestamp last_packet_time;
    core::Timestamp start_time;
    
    FeedStatistics()
        : packets_received(0)
        , packets_dropped(0)
        , ticks_processed(0)
        , parse_errors(0)
        , queue_overflows(0)
        , last_packet_time(0)
        , start_time(0)
    {}
    
    double get_uptime_seconds() const {
        if (start_time == 0) return 0.0;
        auto now = core::TimestampUtil::now();
        return (now - start_time) / 1000000.0;
    }
    
    double get_packet_rate() const {
        double uptime = get_uptime_seconds();
        return uptime > 0 ? packets_received / uptime : 0.0;
    }
    
    double get_tick_rate() const {
        double uptime = get_uptime_seconds();
        return uptime > 0 ? ticks_processed / uptime : 0.0;
    }
};

// UDP Multicast feed handler
class FeedHandler {
public:
    FeedHandler(const FeedConfig& config, const SymbolMap& symbol_map);
    ~FeedHandler();
    
    // Start/stop the feed
    bool start();
    void stop();
    bool is_running() const { return running_; }
    
    // Subscribe to symbols (empty = all symbols)
    void subscribe(const std::string& symbol);
    void unsubscribe(const std::string& symbol);
    void subscribe_all();
    bool is_subscribed(const std::string& symbol) const;
    
    // Set callbacks
    void set_tick_callback(TickCallback callback) { tick_callback_ = callback; }
    void set_error_callback(ErrorCallback callback) { error_callback_ = callback; }
    void set_connection_callback(ConnectionCallback callback) { connection_callback_ = callback; }
    
    // Get statistics
    FeedStatistics get_statistics() const;
    void reset_statistics();
    
    // Get latest tick for symbol
    bool get_latest_tick(const std::string& symbol, Tick& tick) const;
    
private:
    FeedConfig config_;
    TickParser parser_;
    
    // Network socket
    int socket_fd_;
    
    // Threading
    std::atomic<bool> running_;
    std::thread receive_thread_;
    std::thread processing_thread_;
    
    // Tick queue for decoupling receive from processing
    std::queue<Tick> tick_queue_;
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    
    // Subscription management
    std::set<std::string> subscribed_symbols_;
    mutable std::mutex subscription_mutex_;
    bool subscribe_all_;
    
    // Latest ticks cache
    std::map<std::string, Tick> latest_ticks_;
    mutable std::mutex latest_ticks_mutex_;
    
    // Statistics
    FeedStatistics stats_;
    mutable std::mutex stats_mutex_;
    
    // Callbacks
    TickCallback tick_callback_;
    ErrorCallback error_callback_;
    ConnectionCallback connection_callback_;
    
    // Thread functions
    void receive_loop();
    void processing_loop();
    
    // Network setup
    bool create_socket();
    bool join_multicast_group();
    void close_socket();
    
    // Packet processing
    void process_packet(const uint8_t* data, size_t length, core::Timestamp receive_time);
    void process_tick(const Tick& tick);
    
    // Error handling
    void handle_error(const std::string& message);
    
    // Statistics updates
    void update_stats_packet_received();
    void update_stats_tick_processed();
    void update_stats_parse_error();
    void update_stats_queue_overflow();
};

} // namespace market_data
} // namespace hft

#endif // HFT_MARKET_DATA_FEED_HANDLER_H
