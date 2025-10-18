#ifndef HFT_ORDERBOOK_ORDER_BOOK_H
#define HFT_ORDERBOOK_ORDER_BOOK_H

#include "order.h"
#include "../core/types.h"
#include <map>
#include <vector>
#include <memory>
#include <functional>

namespace hft {
namespace orderbook {

struct PriceLevel {
    core::Price price;
    core::Volume total_volume;
    std::vector<OrderPtr> orders;
    
    PriceLevel() : price(0), total_volume(0) { }
    PriceLevel(core::Price p) : price(p), total_volume(0) { }
};

struct Trade {
    core::OrderId buy_order_id;
    core::OrderId sell_order_id;
    core::Price price;
    core::Volume quantity;
    core::Timestamp timestamp;
};

class OrderBook {
public:
    OrderBook(const std::string& symbol);
    
    bool add_order(OrderPtr order);
    bool cancel_order(core::OrderId order_id);
    bool modify_order(core::OrderId order_id, core::Price new_price, core::Volume new_quantity);
    OrderPtr get_order(core::OrderId order_id) const;
    
    core::Price get_best_bid() const;
    core::Price get_best_ask() const;
    core::Price get_mid_price() const;
    core::Volume get_bid_volume_at_level(core::Price price) const;
    core::Volume get_ask_volume_at_level(core::Price price) const;
    core::Volume get_total_bid_volume() const;
    core::Volume get_total_ask_volume() const;
    
    std::vector<PriceLevel> get_bid_levels(size_t depth = 10) const;
    std::vector<PriceLevel> get_ask_levels(size_t depth = 10) const;
    
    core::Price get_spread() const;
    
    size_t get_order_count() const { return orders_.size(); }
    size_t get_bid_level_count() const { return bids_.size(); }
    size_t get_ask_level_count() const { return asks_.size(); }
    
    const std::vector<Trade>& get_trades() const { return trades_; }
    
    const std::string& get_symbol() const { return symbol_; }
    
private:
    std::string symbol_;
    
    std::map<core::Price, PriceLevel, std::greater<core::Price>> bids_;
    std::map<core::Price, PriceLevel, std::less<core::Price>> asks_;
    
    std::map<core::OrderId, OrderPtr> orders_;
    
    std::vector<Trade> trades_;
    
    void match_order(OrderPtr order);
    void match_market_order(OrderPtr order);
    void match_limit_order(OrderPtr order);
    
    void add_to_book(OrderPtr order);
    void remove_from_book(OrderPtr order);
    void execute_trade(OrderPtr buy_order, OrderPtr sell_order, 
                      core::Price price, core::Volume quantity);
};

using OrderBookPtr = std::shared_ptr<OrderBook>;

} // namespace orderbook
} // namespace hft

#endif // HFT_ORDERBOOK_ORDER_BOOK_H
