#ifndef HFT_SIGNALS_MICROSTRUCTURE_SIGNALS_H
#define HFT_SIGNALS_MICROSTRUCTURE_SIGNALS_H

#include "../orderbook/order_book.h"
#include "../core/types.h"
#include <vector>
#include <cmath>

namespace hft {
namespace signals {

struct MicrostructureData {
    double imbalance;
    double bid_pressure;
    double ask_pressure;
    double net_pressure;
    core::PriceDirection direction;
    double confidence;
    core::Price spread;
    double spread_ratio;
    core::Timestamp timestamp;
};

class MicrostructureSignals {
public:
    MicrostructureSignals(size_t depth = 10);
    
    double calculate_imbalance(const orderbook::OrderBook& book);
    double calculate_bid_pressure(const orderbook::OrderBook& book);
    double calculate_ask_pressure(const orderbook::OrderBook& book);
    double calculate_net_pressure(const orderbook::OrderBook& book);
    core::PriceDirection predict_direction(const orderbook::OrderBook& book);
    double calculate_confidence(const orderbook::OrderBook& book);
    MicrostructureData analyze(const orderbook::OrderBook& book);
    double calculate_vwap(const orderbook::OrderBook& book, core::Side side);
    double calculate_depth_imbalance(const orderbook::OrderBook& book, int tick_distance);
    bool detect_imminent_jump(const orderbook::OrderBook& book);
    double calculate_order_flow(const orderbook::OrderBook& book);
    
    void set_depth(size_t depth) { depth_ = depth; }
    void set_imbalance_threshold(double threshold) { imbalance_threshold_ = threshold; }
    void set_pressure_weight_decay(double decay) { pressure_weight_decay_ = decay; }
    
private:
    size_t depth_;
    double imbalance_threshold_;
    double pressure_weight_decay_;
    
    double calculate_weighted_volume(const std::vector<orderbook::PriceLevel>& levels,
                                    core::Price mid_price, bool is_bid_side);
    double calculate_level_weight(core::Price level_price, core::Price mid_price, 
                                 bool is_bid_side);
};

} // namespace signals
} // namespace hft

#endif // HFT_SIGNALS_MICROSTRUCTURE_SIGNALS_H
