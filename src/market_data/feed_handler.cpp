#include "feed_handler.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <stdexcept>

namespace hft {
namespace market_data {

FeedHandler::FeedHandler(const FeedConfig& config, const SymbolMap& symbol_map)
    : config_(config)
    , parser_(symbol_map)
    , socket_fd_(-1)
    , running_(false)
    , subscribe_all_(false)
{
    stats_.start_time = core::TimestampUtil::now();
}

FeedHandler::~FeedHandler() {
    stop();
}

bool FeedHandler::start() {
    if (running_) {
        return false;
    }
    
    // Create and configure socket
    if (!create_socket()) {
        handle_error("Failed to create socket");
        return false;
    }
    
    if (!join_multicast_group()) {
        handle_error("Failed to join multicast group");
        close_socket();
        return false;
    }
    
    // Start threads
    running_ = true;
    receive_thread_ = std::thread(&FeedHandler::receive_loop, this);
    processing_thread_ = std::thread(&FeedHandler::processing_loop, this);
    
    if (connection_callback_) {
        connection_callback_(true);
    }
    
    return true;
}

void FeedHandler::stop() {
    if (!running_) {
        return;
    }
    
    running_ = false;
    
    // Wake up processing thread
    queue_cv_.notify_all();
    
    // Join threads
    if (receive_thread_.joinable()) {
        receive_thread_.join();
    }
    if (processing_thread_.joinable()) {
        processing_thread_.join();
    }
    
    close_socket();
    
    if (connection_callback_) {
        connection_callback_(false);
    }
}

void FeedHandler::subscribe(const std::string& symbol) {
    std::lock_guard<std::mutex> lock(subscription_mutex_);
    subscribed_symbols_.insert(symbol);
    subscribe_all_ = false;
}

void FeedHandler::unsubscribe(const std::string& symbol) {
    std::lock_guard<std::mutex> lock(subscription_mutex_);
    subscribed_symbols_.erase(symbol);
}

void FeedHandler::subscribe_all() {
    std::lock_guard<std::mutex> lock(subscription_mutex_);
    subscribe_all_ = true;
    subscribed_symbols_.clear();
}

bool FeedHandler::is_subscribed(const std::string& symbol) const {
    std::lock_guard<std::mutex> lock(subscription_mutex_);
    if (subscribe_all_) {
        return true;
    }
    return subscribed_symbols_.find(symbol) != subscribed_symbols_.end();
}

FeedStatistics FeedHandler::get_statistics() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return stats_;
}

void FeedHandler::reset_statistics() {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    stats_ = FeedStatistics();
    stats_.start_time = core::TimestampUtil::now();
}

bool FeedHandler::get_latest_tick(const std::string& symbol, Tick& tick) const {
    std::lock_guard<std::mutex> lock(latest_ticks_mutex_);
    auto it = latest_ticks_.find(symbol);
    if (it != latest_ticks_.end()) {
        tick = it->second;
        return true;
    }
    return false;
}

bool FeedHandler::create_socket() {
    // Create UDP socket
    socket_fd_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd_ < 0) {
        return false;
    }
    
    // Set socket to non-blocking mode
    int flags = fcntl(socket_fd_, F_GETFL, 0);
    fcntl(socket_fd_, F_SETFL, flags | O_NONBLOCK);
    
    // Allow address reuse
    int reuse = 1;
    if (setsockopt(socket_fd_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        close(socket_fd_);
        socket_fd_ = -1;
        return false;
    }
    
    // Set receive buffer size
    int buffer_size = config_.buffer_size;
    if (setsockopt(socket_fd_, SOL_SOCKET, SO_RCVBUF, &buffer_size, sizeof(buffer_size)) < 0) {
        // Non-fatal, continue
    }
    
    // Bind to port
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(config_.multicast_port);
    
    if (bind(socket_fd_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(socket_fd_);
        socket_fd_ = -1;
        return false;
    }
    
    return true;
}

bool FeedHandler::join_multicast_group() {
    struct ip_mreq mreq;
    
    // Set multicast group address
    if (inet_pton(AF_INET, config_.multicast_group.c_str(), &mreq.imr_multiaddr) <= 0) {
        return false;
    }
    
    // Set local interface address
    if (config_.interface_ip == "0.0.0.0") {
        mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    } else {
        if (inet_pton(AF_INET, config_.interface_ip.c_str(), &mreq.imr_interface) <= 0) {
            return false;
        }
    }
    
    // Join multicast group
    if (setsockopt(socket_fd_, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
        return false;
    }
    
    return true;
}

void FeedHandler::close_socket() {
    if (socket_fd_ >= 0) {
        close(socket_fd_);
        socket_fd_ = -1;
    }
}

void FeedHandler::receive_loop() {
    std::vector<uint8_t> buffer(config_.buffer_size);
    
    while (running_) {
        // Receive packet
        struct sockaddr_in sender_addr;
        socklen_t sender_len = sizeof(sender_addr);
        
        ssize_t bytes_received = recvfrom(
            socket_fd_,
            buffer.data(),
            buffer.size(),
            0,
            (struct sockaddr*)&sender_addr,
            &sender_len
        );
        
        if (bytes_received < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // No data available, sleep briefly
                std::this_thread::sleep_for(std::chrono::microseconds(100));
                continue;
            }
            
            handle_error("Error receiving packet: " + std::string(strerror(errno)));
            continue;
        }
        
        if (bytes_received == 0) {
            continue;
        }
        
        // Get receive timestamp (as close to arrival as possible)
        auto receive_time = core::TimestampUtil::now();
        
        // Process packet
        process_packet(buffer.data(), bytes_received, receive_time);
        update_stats_packet_received();
    }
}

void FeedHandler::processing_loop() {
    while (running_) {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        
        // Wait for ticks in queue
        queue_cv_.wait(lock, [this] { 
            return !tick_queue_.empty() || !running_; 
        });
        
        if (!running_) {
            break;
        }
        
        // Process all available ticks
        while (!tick_queue_.empty()) {
            Tick tick = tick_queue_.front();
            tick_queue_.pop();
            lock.unlock();
            
            process_tick(tick);
            update_stats_tick_processed();
            
            lock.lock();
        }
    }
}

void FeedHandler::process_packet(const uint8_t* data, size_t length, core::Timestamp receive_time) {
    // Parse all ticks in packet
    auto ticks = parser_.parse_buffer(data, length);
    
    if (ticks.empty()) {
        update_stats_parse_error();
        return;
    }
    
    // Add ticks to queue
    std::lock_guard<std::mutex> lock(queue_mutex_);
    
    for (auto& tick : ticks) {
        // Override timestamp if kernel timestamping not enabled
        if (!config_.enable_timestamping) {
            tick.timestamp = receive_time;
        }
        
        // Check queue size
        if (static_cast<int>(tick_queue_.size()) >= config_.max_queue_size) {
            update_stats_queue_overflow();
            continue;
        }
        
        tick_queue_.push(tick);
    }
    
    // Notify processing thread
    queue_cv_.notify_one();
}

void FeedHandler::process_tick(const Tick& tick) {
    // Check subscription
    if (!is_subscribed(tick.symbol)) {
        return;
    }
    
    // Update latest tick cache
    {
        std::lock_guard<std::mutex> lock(latest_ticks_mutex_);
        latest_ticks_[tick.symbol] = tick;
    }
    
    // Invoke callback
    if (tick_callback_) {
        tick_callback_(tick);
    }
}

void FeedHandler::handle_error(const std::string& message) {
    if (error_callback_) {
        error_callback_(message);
    }
}

void FeedHandler::update_stats_packet_received() {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    stats_.packets_received++;
    stats_.last_packet_time = core::TimestampUtil::now();
}

void FeedHandler::update_stats_tick_processed() {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    stats_.ticks_processed++;
}

void FeedHandler::update_stats_parse_error() {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    stats_.parse_errors++;
}

void FeedHandler::update_stats_queue_overflow() {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    stats_.queue_overflows++;
}

} // namespace market_data
} // namespace hft
