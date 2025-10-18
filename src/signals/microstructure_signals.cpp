#include "microstructure_signals.h"
#include <algorithm>
#include <cmath>

namespace hft {
namespace signals {

MicrostructureSignals::MicrostructureSignals(size_t depth)
    : depth_(depth)
    , imbalance_threshold_(0.3)
    , pressure_weight_decay_(0.9)
{}

double MicrostructureSignals::calculate_imbalance(const orderbook::OrderBook& book) {
    auto bid_volume = book.get_total_bid_volume();
    auto ask_volume = book.get_total_ask_volume();
    
    if (bid_volume == 0 && ask_volume == 0) {
        return 0.0;
    }
    
    double total = static_cast<double>(bid_volume + ask_volume);
    double imbalance = (static_cast<double>(bid_volume) - static_cast<double>(ask_volume)) / total;
    
    return imbalance;
}

double MicrostructureSignals::calculate_bid_pressure(const orderbook::OrderBook& book) {
    auto mid_price = book.get_mid_price();
    if (mid_price == 0) return 0.0;
    
    auto bid_levels = book.get_bid_levels(depth_);
    return calculate_weighted_volume(bid_levels, mid_price, true);
}

double MicrostructureSignals::calculate_ask_pressure(const orderbook::OrderBook& book) {
    auto mid_price = book.get_mid_price();
    if (mid_price == 0) return 0.0;
    
    auto ask_levels = book.get_ask_levels(depth_);
    return calculate_weighted_volume(ask_levels, mid_price, false);
}

double MicrostructureSignals::calculate_net_pressure(const orderbook::OrderBook& book) {
    return calculate_bid_pressure(book) - calculate_ask_pressure(book);
}

core::PriceDirection MicrostructureSignals::predict_direction(const orderbook::OrderBook& book) {
    double imbalance = calculate_imbalance(book);
    double net_pressure = calculate_net_pressure(book);
    
    double signal = 0.6 * imbalance + 0.4 * net_pressure;
    
    if (signal > imbalance_threshold_) {
        return core::PriceDirection::UP;
    } else if (signal < -imbalance_threshold_) {
        return core::PriceDirection::DOWN;
    } else {
        return core::PriceDirection::NEUTRAL;
    }
}

double MicrostructureSignals::calculate_confidence(const orderbook::OrderBook& book) {
    double imbalance = std::abs(calculate_imbalance(book));
    double net_pressure = std::abs(calculate_net_pressure(book));
    
    auto spread = book.get_spread();
    auto mid_price = book.get_mid_price();
    double spread_ratio = (mid_price > 0) ? 
        static_cast<double>(spread) / static_cast<double>(mid_price) : 0.0;
    
    double spread_confidence = std::max(0.0, 1.0 - spread_ratio * 1000);
    
    double confidence = (0.4 * imbalance + 0.4 * net_pressure + 0.2 * spread_confidence);
    
    return std::min(1.0, std::max(0.0, confidence));
}

MicrostructureData MicrostructureSignals::analyze(const orderbook::OrderBook& book) {
    MicrostructureData data;
    
    data.imbalance = calculate_imbalance(book);
    data.bid_pressure = calculate_bid_pressure(book);
    data.ask_pressure = calculate_ask_pressure(book);
    data.net_pressure = calculate_net_pressure(book);
    data.direction = predict_direction(book);
    data.confidence = calculate_confidence(book);
    data.spread = book.get_spread();
    
    auto mid_price = book.get_mid_price();
    data.spread_ratio = (mid_price > 0) ? 
        static_cast<double>(data.spread) / static_cast<double>(mid_price) : 0.0;
    
    data.timestamp = core::TimestampUtil::now();
    
    return data;
}

double MicrostructureSignals::calculate_vwap(const orderbook::OrderBook& book, core::Side side) {
    auto levels = (side == core::Side::BUY) ? 
                  book.get_bid_levels(depth_) : 
                  book.get_ask_levels(depth_);
    
    double total_value = 0.0;
    core::Volume total_volume = 0;
    
    for (const auto& level : levels) {
        total_value += core::price_to_double(level.price) * level.total_volume;
        total_volume += level.total_volume;
    }
    
    return (total_volume > 0) ? total_value / total_volume : 0.0;
}

double MicrostructureSignals::calculate_depth_imbalance(const orderbook::OrderBook& book, 
                                                        int tick_distance) {
    auto mid_price = book.get_mid_price();
    if (mid_price == 0) return 0.0;
    
    core::Price bid_price = mid_price - tick_distance;
    core::Price ask_price = mid_price + tick_distance;
    
    auto bid_volume = book.get_bid_volume_at_level(bid_price);
    auto ask_volume = book.get_ask_volume_at_level(ask_price);
    
    if (bid_volume == 0 && ask_volume == 0) return 0.0;
    
    double total = static_cast<double>(bid_volume + ask_volume);
    return (static_cast<double>(bid_volume) - static_cast<double>(ask_volume)) / total;
}

bool MicrostructureSignals::detect_imminent_jump(const orderbook::OrderBook& book) {
    double imbalance = std::abs(calculate_imbalance(book));
    double net_pressure = std::abs(calculate_net_pressure(book));
    
    auto bid_levels = book.get_bid_level_count();
    auto ask_levels = book.get_ask_level_count();
    
    bool thin_book = (bid_levels < 3 || ask_levels < 3);
    
    return (imbalance > 0.7 && net_pressure > 0.6 && thin_book);
}

double MicrostructureSignals::calculate_order_flow(const orderbook::OrderBook& book) {
    auto trades = book.get_trades();
    if (trades.empty()) return 0.0;
    
    size_t lookback = std::min(trades.size(), size_t(100));
    
    int64_t buy_volume = 0;
    int64_t sell_volume = 0;
    
    for (size_t i = trades.size() - lookback; i < trades.size(); ++i) {
        const auto& trade = trades[i];
        
        if (trade.price >= book.get_mid_price()) {
            buy_volume += trade.quantity;
        } else {
            sell_volume += trade.quantity;
        }
    }
    
    if (buy_volume == 0 && sell_volume == 0) return 0.0;
    
    double total = static_cast<double>(buy_volume + sell_volume);
    return (static_cast<double>(buy_volume) - static_cast<double>(sell_volume)) / total;
}

double MicrostructureSignals::calculate_weighted_volume(
    const std::vector<orderbook::PriceLevel>& levels,
    core::Price mid_price,
    bool is_bid_side) {
    
    double weighted_volume = 0.0;
    
    for (const auto& level : levels) {
        double weight = calculate_level_weight(level.price, mid_price, is_bid_side);
        weighted_volume += weight * level.total_volume;
    }
    
    return weighted_volume;
}

double MicrostructureSignals::calculate_level_weight(core::Price level_price, 
                                                     core::Price mid_price,
                                                     bool is_bid_side) {
    double distance = std::abs(static_cast<double>(level_price - mid_price));
    double mid_price_double = core::price_to_double(mid_pr
