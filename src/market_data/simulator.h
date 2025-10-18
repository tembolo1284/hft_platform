#ifndef HFT_MARKET_DATA_SIMULATOR_H
#define HFT_MARKET_DATA_SIMULATOR_H

#include "tick_parser.h"
#include "../core/types.h"
#include <functional>
#include <thread>
#include <atomic>
#include <random>
#include <vector>

namespace hft {
namespace market_data {

// Simulated market behavior parameters
struct SimulationConfig {
    double base_price;               // Starting price
    double volatility;               // Price volatility (annualized)
    double tick_rate_hz;             // Ticks per second
    double spread_bps;               // Bid-ask spread in basis points
    int depth_levels;                // Number of price levels to simulate
    double mean_trade_size;          // Average trade size
    double lambda_trades;            // Trade arrival rate (Poisson)
    
    SimulationConfig()
        : base_price(100.0)
        , volatility(0.20)           // 20% annual volatility
        , tick_rate_hz(100.0)        // 100 updates per second
        , spread_bps(1.0)            // 1 basis point spread
        , depth_levels(10)
        , mean_trade_size(100.0)
        , lambda_trades(10.0)        // 10 trades per second on average
    {}
};

// Market simulator for testing and backtesting
class MarketSimulator {
public:
    MarketSimulator(const std::string& symbol, const SimulationConfig& config);
    ~MarketSimulator();
    
    // Start/stop simulation
    void start();
    void stop();
    bool is_running() const { return running_; }
    
    // Set tick callback
    void set_tick_callback(TickCallback callback) { tick_callback_ = callback; }
    
    // Manual tick generation (for replay scenarios)
    Tick generate_quote_update();
    Tick generate_trade();
    
    // Set current price (for controlled testing)
    void set_price(double price);
    double get_current_price() const { return current_price_; }
    
    // Statistics
    uint64_t get_ticks_generated() const { return ticks_generated_; }
    uint64_t get_trades_generated() const { return trades_generated_; }
    
private:
    std::string symbol_;
    SimulationConfig config_;
    
    // Current market state
    double current_price_;
    double bid_price_;
    double ask_price_;
    
    // Threading
    std::atomic<bool> running_;
    std::thread simulation_thread_;
    
    // Random number generation
    std::mt19937_64 rng_;
    std::normal_distribution<double> price_dist_;
    std::exponential_distribution<double> trade_interval_dist_;
    std::poisson_distribution<int> trade_size_dist_;
    
    // Statistics
    std::atomic<uint64_t> ticks_generated_;
    std::atomic<uint64_t> trades_generated_;
    
    // Callback
    TickCallback tick_callback_;
    
    // Simulation loop
    void simulation_loop();
    
    // Price generation
    void update_prices();
    double generate_brownian_increment();
    
    // Tick generation
    Tick create_bid_tick();
    Tick create_ask_tick();
    Tick create_trade_tick(core::Side aggressor_side);
    
    // Volume generation
    core::Volume generate_quote_volume();
    core::Volume generate_trade_volume();
};

// Replay historical data from file
class HistoricalReplayer {
public:
    HistoricalReplayer(const std::string& filename);
    
    // Load ticks from file
    bool load();
    
    // Replay at original speed or accelerated
    void replay(TickCallback callback, double speed_multiplier = 1.0);
    void replay_fast(TickCallback callback);  // Replay as fast as possible
    
    // Get loaded ticks
    const std::vector<Tick>& get_ticks() const { return ticks_; }
    size_t get_tick_count() const { return ticks_.size(); }
    
private:
    std::string filename_;
    std::vector<Tick> ticks_;
};

} // namespace market_data
} // namespace hft

#endif // HFT_MARKET_DATA_SIMULATOR_H
