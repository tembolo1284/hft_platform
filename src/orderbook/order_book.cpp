#include "order_book.h"
#include <algorithm>
#include <stdexcept>

namespace hft {
namespace orderbook {

OrderBook::OrderBook(const std::string& symbol)
    : symbol_(symbol)
{}

bool OrderBook::add_order(OrderPtr order) {
    if (!order || order->get_symbol() != symbol_) {
        return false;
    }
    
    orders_[order->get_id()] = order;
    
    match_order(order);
    
    if (order->is_fillable()) {
        add_to_book(order);
    }
    
    return true;
}

bool OrderBook::cancel_order(core::OrderId order_id) {
    auto it = orders_.find(order_id);
    if (it == orders_.end()) {
        return false;
    }
    
    auto order = it->second;
    if (!order->is_active()) {
        return false;
    }
    
    remove_from_book(order);
    order->set_status(core::OrderStatus::CANCELLED);
    
    return true;
}

bool OrderBook::modify_order(core::OrderId order_id, core::Price new_price, core::Volume new_quantity) {
    auto it = orders_.find(order_id);
    if (it == orders_.end()) {
        return false;
    }
    
    auto order = it->second;
    if (!order->is_active()) {
        return false;
    }
    
    remove_from_book(order);
    order->set_price(new_price);
    order->set_quantity(new_quantity);
    
    match_order(order);
    if (order->is_fillable()) {
        add_to_book(order);
    }
    
    return true;
}

OrderPtr OrderBook::get_order(core::OrderId order_id) const {
    auto it = orders_.find(order_id);
    return it != orders_.end() ? it->second : nullptr;
}

core::Price OrderBook::get_best_bid() const {
    return bids_.empty() ? 0 : bids_.begin()->first;
}

core::Price OrderBook::get_best_ask() const {
    return asks_.empty() ? 0 : asks_.begin()->first;
}

core::Price OrderBook::get_mid_price() const {
    auto best_bid = get_best_bid();
    auto best_ask = get_best_ask();
    
    if (best_bid == 0 || best_ask == 0) {
        return std::max(best_bid, best_ask);
    }
    
    return (best_bid + best_ask) / 2;
}

core::Volume OrderBook::get_bid_volume_at_level(core::Price price) const {
    auto it = bids_.find(price);
    return it != bids_.end() ? it->second.total_volume : 0;
}

core::Volume OrderBook::get_ask_volume_at_level(core::Price price) const {
    auto it = asks_.find(price);
    return it != asks_.end() ? it->second.total_volume : 0;
}

core::Volume OrderBook::get_total_bid_volume() const {
    core::Volume total = 0;
    for (const auto& [price, level] : bids_) {
        total += level.total_volume;
    }
    return total;
}

core::Volume OrderBook::get_total_ask_volume() const {
    core::Volume total = 0;
    for (const auto& [price, level] : asks_) {
        total += level.total_volume;
    }
    return total;
}

std::vector<PriceLevel> OrderBook::get_bid_levels(size_t depth) const {
    std::vector<PriceLevel> levels;
    size_t count = 0;
    
    for (const auto& [price, level] : bids_) {
        if (count >= depth) break;
        levels.push_back(level);
        count++;
    }
    
    return levels;
}

std::vector<PriceLevel> OrderBook::get_ask_levels(size_t depth) const {
    std::vector<PriceLevel> levels;
    size_t count = 0;
    
    for (const auto& [price, level] : asks_) {
        if (count >= depth) break;
        levels.push_back(level);
        count++;
    }
    
    return levels;
}

core::Price OrderBook::get_spread() const {
    auto best_bid = get_best_bid();
    auto best_ask = get_best_ask();
    
    if (best_bid == 0 || best_ask == 0) {
        return 0;
    }
    
    return best_ask - best_bid;
}

void OrderBook::match_order(OrderPtr order) {
    if (order->get_type() == core::OrderType::MARKET) {
        match_market_order(order);
    } else if (order->get_type() == core::OrderType::LIMIT) {
        match_limit_order(order);
    }
}

void OrderBook::match_market_order(OrderPtr order) {
    if (order->get_side() == core::Side::BUY) {
        // Market buy order - match against asks
        for (auto& [price, level] : asks_) {
            if (!order->is_fillable()) break;
            
            for (auto it = level.orders.begin(); it != level.orders.end(); ) {
                if (!order->is_fillable()) break;
                
                auto opposite_order = *it;
                auto fill_quantity = std::min(order->get_remaining_quantity(), 
                                             opposite_order->get_remaining_quantity());
                
                execute_trade(order, opposite_order, opposite_order->get_price(), fill_quantity);
                
                if (!opposite_order->is_fillable()) {
                    it = level.orders.erase(it);
                    level.total_volume -= opposite_order->get_quantity();
                } else {
                    ++it;
                }
            }
        }
    } else {
        // Market sell order - match against bids
        for (auto& [price, level] : bids_) {
            if (!order->is_fillable()) break;
            
            for (auto it = level.orders.begin(); it != level.orders.end(); ) {
                if (!order->is_fillable()) break;
                
                auto opposite_order = *it;
                auto fill_quantity = std::min(order->get_remaining_quantity(), 
                                             opposite_order->get_remaining_quantity());
                
                execute_trade(opposite_order, order, opposite_order->get_price(), fill_quantity);
                
                if (!opposite_order->is_fillable()) {
                    it = level.orders.erase(it);
                    level.total_volume -= opposite_order->get_quantity();
                } else {
                    ++it;
                }
            }
        }
    }
}

void OrderBook::match_limit_order(OrderPtr order) {
    if (order->get_side() == core::Side::BUY) {
        // Limit buy order - match against asks
        for (auto& [price, level] : asks_) {
            if (!order->is_fillable()) break;
            
            // Check if price is acceptable
            if (price > order->get_price()) break;
            
            for (auto it = level.orders.begin(); it != level.orders.end(); ) {
                if (!order->is_fillable()) break;
                
                auto opposite_order = *it;
                auto fill_quantity = std::min(order->get_remaining_quantity(), 
                                             opposite_order->get_remaining_quantity());
                
                execute_trade(order, opposite_order, opposite_order->get_price(), fill_quantity);
                
                if (!opposite_order->is_fillable()) {
                    it = level.orders.erase(it);
                    level.total_volume -= opposite_order->get_quantity();
                } else {
                    ++it;
                }
            }
        }
    } else {
        // Limit sell order - match against bids
        for (auto& [price, level] : bids_) {
            if (!order->is_fillable()) break;
            
            // Check if price is acceptable
            if (price < order->get_price()) break;
            
            for (auto it = level.orders.begin(); it != level.orders.end(); ) {
                if (!order->is_fillable()) break;
                
                auto opposite_order = *it;
                auto fill_quantity = std::min(order->get_remaining_quantity(), 
                                             opposite_order->get_remaining_quantity());
                
                execute_trade(opposite_order, order, opposite_order->get_price(), fill_quantity);
                
                if (!opposite_order->is_fillable()) {
                    it = level.orders.erase(it);
                    level.total_volume -= opposite_order->get_quantity();
                } else {
                    ++it;
                }
            }
        }
    }
}

void OrderBook::add_to_book(OrderPtr order) {
    auto price = order->get_price();
    
    if (order->get_side() == core::Side::BUY) {
        auto it = bids_.find(price);
        if (it == bids_.end()) {
            bids_[price] = PriceLevel(price);
            it = bids_.find(price);
        }
        
        it->second.orders.push_back(order);
        it->second.total_volume += order->get_remaining_quantity();
    } else {
        auto it = asks_.find(price);
        if (it == asks_.end()) {
            asks_[price] = PriceLevel(price);
            it = asks_.find(price);
        }
        
        it->second.orders.push_back(order);
        it->second.total_volume += order->get_remaining_quantity();
    }
}

void OrderBook::remove_from_book(OrderPtr order) {
    auto price = order->get_price();
    
    if (order->get_side() == core::Side::BUY) {
        auto it = bids_.find(price);
        if (it != bids_.end()) {
            auto& orders = it->second.orders;
            orders.erase(
                std::remove(orders.begin(), orders.end(), order),
                orders.end()
            );
            
            it->second.total_volume -= order->get_remaining_quantity();
            
            if (orders.empty()) {
                bids_.erase(it);
            }
        }
    } else {
        auto it = asks_.find(price);
        if (it != asks_.end()) {
            auto& orders = it->second.orders;
            orders.erase(
                std::remove(orders.begin(), orders.end(), order),
                orders.end()
            );
            
            it->second.total_volume -= order->get_remaining_quantity();
            
            if (orders.empty()) {
                asks_.erase(it);
            }
        }
    }
}

void OrderBook::execute_trade(OrderPtr buy_order, OrderPtr sell_order,
                              core::Price price, core::Volume quantity) {
    buy_order->add_fill(quantity);
    sell_order->add_fill(quantity);
    
    Trade trade {
        buy_order->get_id(),
        sell_order->get_id(),
        price,
        quantity,
        core::TimestampUtil::now()
    };
    
    trades_.push_back(trade);
}

} // namespace orderbook
} // namespace hft
