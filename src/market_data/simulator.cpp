#include "simulator.h"
#include <cmath>
#include <fstream>
#include <thread>
#include <chrono>

namespace hft {
namespace market_data {

// ============================================================================
// MarketSimulator Implementation
// ============================================================================

MarketSimulator::MarketSimulator(const std::string& symbol, const SimulationConfig& config)
    : symbol_(symbol)
    , config_(config)
    , current_price_(config.base_price)
    , running_(false)
    , rng_(std::random_device{}())
    , price_dist_(0.0, 1.0)
    , trade_interval_dist_(config.lambda_trades)
    , trade_size_dist_(static_cast<int>(config.mean_trade_size))
    , ticks_generated_(0)
    , trades_generated_(0)
{
    update_prices();
}

MarketSimulator::~MarketSimulator() {
    stop();
}

void MarketSimulator::start() {
    if (running_) {
        return;
    }
    
    running_ = true;
    simulation_thread_ = std::thread(&MarketSimulator::simulation_loop, this);
}

void MarketSimulator::stop() {
    if (!running_) {
        return;
    }
    
    running_ = false;
    if (simulation_thread_.joinable()) {
        simulation_thread_.join();
    }
}

void MarketSimulator::set_price(double price) {
    current_price_ = price;
    update_prices();
}

Tick MarketSimulator::generate_quote_update() {
    update_prices();
    
    // Randomly choose bid or ask update
    bool is_bid = (rng_() % 2) == 0;
    
    ticks_generated_++;
    return is_bid ? create_bid_tick() : create_ask_tick();
}

Tick MarketSimulator::generate_trade() {
    // Randomly choose aggressor side
    auto aggressor = (rng_() % 2) == 0 ? core::Side::BUY : core::Side::SELL;
    
    trades_generated_++;
    return create_trade_tick(aggressor);
}

void MarketSimulator::simulation_loop() {
    auto tick_interval = std::chrono::duration<double>(1.0 / config_.tick_rate_hz);
    auto next_tick_time = std::chrono::steady_clock::now();
    auto next_trade_time = next_tick_time + std::chrono::duration<double>(trade_interval_dist_(rng_));
    
    while (running_) {
        auto now = std::chrono::steady_clock::now();
        
        // Check if we should generate a trade
        if (now >= next_trade_time) {
            auto trade = generate_trade();
            if (tick_callback_) {
                tick_callback_(trade);
            }
            
            // Schedule next trade
            next_trade_time = now + std::chrono::duration<double>(trade_interval_dist_(rng_));
        }
        
        // Check if we should generate a quote update
        if (now >= next_tick_time) {
            auto quote = generate_quote_update();
            if (tick_callback_) {
                tick_callback_(quote);
            }
            
            next_tick_time += tick_interval;
        }
        
        // Sleep until next event
        auto next_event = std::min(next_tick_time, next_trade_time);
        if (next_event > now) {
            std::this_thread::sleep_until(next_event);
        }
    }
}

void MarketSimulator::update_prices() {
    // Brownian motion price update
    double increment = generate_brownian_increment();
    current_price_ *= (1.0 + increment);
    
    // Update bid/ask around current price
    double half_spread = current_price_ * (config_.spread_bps / 10000.0) / 2.0;
    bid_price_ = current_price_ - half_spread;
    ask_price_ = current_price_ + half_spread;
}

double MarketSimulator::generate_brownian_increment() {
    // Geometric Brownian Motion increment
    // dS = μ*S*dt + σ*S*dW
    // For simulation, we use dt = 1 second / tick_rate
    
    double dt = 1.0 / config_.tick_rate_hz;
    double drift = 0.0;  // Assume zero drift for simplicity
    double diffusion = config_.volatility * std::sqrt(dt) * price_dist_(rng_);
    
    return drift + diffusion;
}

Tick MarketSimulator::create_bid_tick() {
    Tick tick;
    tick.symbol = symbol_;
    tick.timestamp = core::TimestampUtil::now();
    tick.price = core::double_to_price(bid_price_);
    tick.volume = generate_quote_volume();
    tick.side = core::Side::BUY;
    tick.is_trade = false;
    tick.exchange_id = 1;
    
    return tick;
}

Tick MarketSimulator::create_ask_tick() {
    Tick tick;
    tick.symbol = symbol_;
    tick.timestamp = core::TimestampUtil::now();
    tick.price = core::double_to_price(ask_price_);
    tick.volume = generate_quote_volume();
    tick.side = core::Side::SELL;
    tick.is_trade = false;
    tick.exchange_id = 1;
    
    return tick;
}

Tick MarketSimulator::create_trade_tick(core::Side aggressor_side) {
    Tick tick;
    tick.symbol = symbol_;
    tick.timestamp = core::TimestampUtil::now();
    
    // Trade occurs at ask if buy aggressor, bid if sell aggressor
    if (aggressor_side == core::Side::BUY) {
        tick.price = core::double_to_price(ask_price_);
    } else {
        tick.price = core::double_to_price(bid_price_);
    }
    
    tick.volume = generate_trade_volume();
    tick.side = aggressor_side;
    tick.is_trade = true;
    tick.exchange_id = 1;
    
    return tick;
}

core::Volume MarketSimulator::generate_quote_volume() {
    // Generate volume between 50 and 500
    return 50 + (rng_() % 450);
}

core::Volume MarketSimulator::generate_trade_volume() {
    int volume = trade_size_dist_(rng_);
    return std::max(1, volume);  // At least 1
}

// ============================================================================
// HistoricalReplayer Implementation
// ============================================================================

HistoricalReplayer::HistoricalReplayer(const std::string& filename)
    : filename_(filename)
{}

bool HistoricalReplayer::load() {
    std::ifstream file(filename_, std::ios::binary);
    if (!file) {
        return false;
    }
    
    // Read file size
    file.seekg(0, std::ios::end);
    size_t file_size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    // Read all data
    std::vector<uint8_t> buffer(file_size);
    file.read(reinterpret_cast<char*>(buffer.data()), file_size);
    file.close();
    
    // Parse ticks (requires SymbolMap and TickParser)
    // For now, just store raw data
    // In real implementation, would parse here
    
    return true;
}

void HistoricalReplayer::replay(TickCallback callback, double speed_multiplier) {
    if (ticks_.empty()) {
        return;
    }
    
    auto start_time = std::chrono::steady_clock::now();
    core::Timestamp first_tick_time = ticks_[0].timestamp;
    
    for (const auto& tick : ticks_) {
        // Calculate how long to wait
        core::Timestamp elapsed_sim = tick.timestamp - first_tick_time;
        double elapsed_real_us = elapsed_sim / speed_multiplier;
        
        auto target_time = start_time + std::chrono::microseconds(
            static_cast<int64_t>(elapsed_real_us)
        );
        
        std::this_thread::sleep_until(target_time);
        
        callback(tick);
    }
}

void HistoricalReplayer::replay_fast(TickCallback callback) {
    for (const auto& tick : ticks_) {
        callback(tick);
    }
}

} // namespace market_data
} // namespace hft
