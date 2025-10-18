#include "advanced_orders.h"
#include <algorithm>
#include <stdexcept>

namespace hft {
namespace orderbook {

// ============================================================================
// StopLossOrder Implementation
// ============================================================================

StopLossOrder::StopLossOrder(core::OrderId id, const std::string& symbol,
                             core::Side side, core::Price stop_price, core::Volume quantity)
    : AdvancedOrder(id, symbol)
    , side_(side)
    , stop_price_(stop_price)
    , quantity_(quantity)
    , triggered_(false)
{}

bool StopLossOrder::should_trigger(const OrderBook& book) const {
    if (triggered_) return false;
    
    if (side_ == core::Side::SELL) {
        // Stop loss sell triggers when market falls to/below stop
        auto best_bid = book.get_best_bid();
        return best_bid > 0 && best_bid <= stop_price_;
    } else {
        // Stop loss buy triggers when market rises to/above stop
        auto best_ask = book.get_best_ask();
        return best_ask > 0 && best_ask >= stop_price_;
    }
}

OrderPtr StopLossOrder::create_execution_order() {
    triggered_ = true;
    return std::make_shared<Order>(
        id_, symbol_, side_, core::OrderType::MARKET, 0, quantity_
    );
}

void StopLossOrder::update(const OrderBook& book) {
    // Stop loss doesn't need updates once triggered
}

// ============================================================================
// StopLimitOrder Implementation
// ============================================================================

StopLimitOrder::StopLimitOrder(core::OrderId id, const std::string& symbol,
                               core::Side side, core::Price stop_price,
                               core::Price limit_price, core::Volume quantity)
    : AdvancedOrder(id, symbol)
    , side_(side)
    , stop_price_(stop_price)
    , limit_price_(limit_price)
    , quantity_(quantity)
    , triggered_(false)
{}

bool StopLimitOrder::should_trigger(const OrderBook& book) const {
    if (triggered_) return false;
    
    if (side_ == core::Side::SELL) {
        auto best_bid = book.get_best_bid();
        return best_bid > 0 && best_bid <= stop_price_;
    } else {
        auto best_ask = book.get_best_ask();
        return best_ask > 0 && best_ask >= stop_price_;
    }
}

OrderPtr StopLimitOrder::create_execution_order() {
    triggered_ = true;
    return std::make_shared<Order>(
        id_, symbol_, side_, core::OrderType::LIMIT, limit_price_, quantity_
    );
}

void StopLimitOrder::update(const OrderBook& book) {
    // Stop limit doesn't need updates once triggered
}

// ============================================================================
// TrailingStopOrder Implementation
// ============================================================================

TrailingStopOrder::TrailingStopOrder(core::OrderId id, const std::string& symbol,
                                     core::Side side, core::Price trail_distance, 
                                     core::Volume quantity)
    : AdvancedOrder(id, symbol)
    , side_(side)
    , trail_distance_(trail_distance)
    , quantity_(quantity)
    , current_stop_(0)
    , best_price_(0)
    , triggered_(false)
{}

bool TrailingStopOrder::should_trigger(const OrderBook& book) const {
    if (triggered_) return false;
    if (current_stop_ == 0) return false;
    
    if (side_ == core::Side::SELL) {
        auto best_bid = book.get_best_bid();
        return best_bid > 0 && best_bid <= current_stop_;
    } else {
        auto best_ask = book.get_best_ask();
        return best_ask > 0 && best_ask >= current_stop_;
    }
}

OrderPtr TrailingStopOrder::create_execution_order() {
    triggered_ = true;
    return std::make_shared<Order>(
        id_, symbol_, side_, core::OrderType::MARKET, 0, quantity_
    );
}

void TrailingStopOrder::update(const OrderBook& book) {
    if (triggered_) return;
    
    if (side_ == core::Side::SELL) {
        // Trailing stop sell - follows price up
        auto best_bid = book.get_best_bid();
        if (best_bid > best_price_) {
            best_price_ = best_bid;
            current_stop_ = best_price_ - trail_distance_;
        }
    } else {
        // Trailing stop buy - follows price down
        auto best_ask = book.get_best_ask();
        if (best_ask < best_price_ || best_price_ == 0) {
            best_price_ = best_ask;
            current_stop_ = best_price_ + trail_distance_;
        }
    }
}

// ============================================================================
// IcebergOrder Implementation
// ============================================================================

IcebergOrder::IcebergOrder(core::OrderId id, const std::string& symbol,
                           core::Side side, core::Price price,
                           core::Volume total_quantity, core::Volume display_quantity)
    : AdvancedOrder(id, symbol)
    , side_(side)
    , price_(price)
    , total_quantity_(total_quantity)
    , display_quantity_(display_quantity)
    , remaining_quantity_(total_quantity)
    , current_order_(nullptr)
{}

OrderPtr IcebergOrder::create_execution_order() {
    auto qty = std::min(display_quantity_, remaining_quantity_);
    current_order_ = std::make_shared<Order>(
        id_, symbol_, side_, core::OrderType::LIMIT, price_, qty
    );
    return current_order_;
}

void IcebergOrder::update(const OrderBook& book) {
    // Check if current slice needs replenishment
    if (current_order_ && current_order_->get_status() == core::OrderStatus::FILLED) {
        on_fill(current_order_->get_filled_quantity());
    }
}

void IcebergOrder::on_fill(core::Volume filled_qty) {
    remaining_quantity_ -= filled_qty;
    current_order_.reset();
}

// ============================================================================
// PeggedOrder Implementation
// ============================================================================

PeggedOrder::PeggedOrder(core::OrderId id, const std::string& symbol,
                         core::Side side, core::Price offset, core::Volume quantity)
    : AdvancedOrder(id, symbol)
    , side_(side)
    , offset_(offset)
    , quantity_(quantity)
    , current_price_(0)
    , filled_(false)
{}

OrderPtr PeggedOrder::create_execution_order() {
    return std::make_shared<Order>(
        id_, symbol_, side_, core::OrderType::LIMIT, current_price_, quantity_
    );
}

void PeggedOrder::update(const OrderBook& book) {
    if (filled_) return;
    
    core::Price new_price;
    if (side_ == core::Side::BUY) {
        // Peg to best bid + offset
        new_price = book.get_best_bid() + offset_;
    } else {
        // Peg to best ask - offset
        new_price = book.get_best_ask() - offset_;
    }
    
    if (new_price != current_price_) {
        current_price_ = new_price;
        // In real implementation, would cancel and replace order
    }
}

// ============================================================================
// IOCOrder Implementation
// ============================================================================

IOCOrder::IOCOrder(core::OrderId id, const std::string& symbol,
                   core::Side side, core::Price price, core::Volume quantity)
    : AdvancedOrder(id, symbol)
    , side_(side)
    , price_(price)
    , quantity_(quantity)
    , executed_(false)
{}

OrderPtr IOCOrder::create_execution_order() {
    executed_ = true;
    auto order = std::make_shared<Order>(
        id_, symbol_, side_, core::OrderType::LIMIT, price_, quantity_
    );
    // Mark as IOC - any unfilled portion should be cancelled immediately
    return order;
}

void IOCOrder::update(const OrderBook& book) {
    // IOC executes once
}

// ============================================================================
// FOKOrder Implementation
// ============================================================================

FOKOrder::FOKOrder(core::OrderId id, const std::string& symbol,
                   core::Side side, core::Price price, core::Volume quantity)
    : AdvancedOrder(id, symbol)
    , side_(side)
    , price_(price)
    , quantity_(quantity)
    , checked_(false)
    , can_fill_(false)
{}

bool FOKOrder::should_trigger(const OrderBook& book) const {
    if (checked_) return false;
    
    // Check if entire quantity can be filled
    core::Volume available = 0;
    if (side_ == core::Side::BUY) {
        auto levels = book.get_ask_levels(10);
        for (const auto& level : levels) {
            if (level.price <= price_) {
                available += level.total_volume;
                if (available >= quantity_) {
                    return true;
                }
            } else {
                break;
            }
        }
    } else {
        auto levels = book.get_bid_levels(10);
        for (const auto& level : levels) {
            if (level.price >= price_) {
                available += level.total_volume;
                if (available >= quantity_) {
                    return true;
                }
            } else {
                break;
            }
        }
    }
    
    return false;
}

OrderPtr FOKOrder::create_execution_order() {
    checked_ = true;
    can_fill_ = true;
    return std::make_shared<Order>(
        id_, symbol_, side_, core::OrderType::LIMIT, price_, quantity_
    );
}

void FOKOrder::update(const OrderBook& book) {
    if (!checked_) {
        checked_ = true;
        can_fill_ = should_trigger(book);
    }
}

// ============================================================================
// BracketOrder Implementation
// ============================================================================

BracketOrder::BracketOrder(core::OrderId id, const std::string& symbol,
                           core::Side side, core::Price entry_price, core::Volume quantity,
                           core::Price stop_loss, core::Price take_profit)
    : AdvancedOrder(id, symbol)
    , side_(side)
    , entry_price_(entry_price)
    , quantity_(quantity)
    , stop_loss_price_(stop_loss)
    , take_profit_price_(take_profit)
    , parent_filled_(false)
    , exited_(false)
    , parent_order_(nullptr)
{}

bool BracketOrder::should_trigger(const OrderBook& book) const {
    if (!parent_filled_ || exited_) return false;
    
    // Check if stop loss or take profit should trigger
    if (side_ == core::Side::BUY) {
        // Long position: check if we hit stop loss (below) or take profit (above)
        auto best_bid = book.get_best_bid();
        return best_bid <= stop_loss_price_ || best_bid >= take_profit_price_;
    } else {
        // Short position
        auto best_ask = book.get_best_ask();
        return best_ask >= stop_loss_price_ || best_ask <= take_profit_price_;
    }
}

OrderPtr BracketOrder::create_execution_order() {
    if (!parent_filled_) {
        // Create parent entry order
        parent_order_ = std::make_shared<Order>(
            id_, symbol_, side_, core::OrderType::LIMIT, entry_price_, quantity_
        );
        return parent_order_;
    } else if (!exited_) {
        // Create exit order (opposite side)
        exited_ = true;
        auto exit_side = (side_ == core::Side::BUY) ? core::Side::SELL : core::Side::BUY;
        return std::make_shared<Order>(
            id_ + 1, symbol_, exit_side, core::OrderType::MARKET, 0, quantity_
        );
    }
    
    return nullptr;
}

void BracketOrder::update(const OrderBook& book) {
    if (parent_order_ && parent_order_->get_status() == core::OrderStatus::FILLED) {
        parent_filled_ = true;
    }
}

bool BracketOrder::is_complete() const {
    return exited_;
}

void BracketOrder::on_parent_fill() {
    parent_filled_ = true;
}

// ============================================================================
// OCOOrder Implementation
// ============================================================================

OCOOrder::OCOOrder(core::OrderId id, const std::string& symbol,
                   OrderPtr order1, OrderPtr order2)
    : AdvancedOrder(id, symbol)
    , order1_(order1)
    , order2_(order2)
    , completed_(false)
{}

bool OCOOrder::should_trigger(const OrderBook& book) const {
    return !completed_;
}

OrderPtr OCOOrder::create_execution_order() {
    // Return first order initially
    // In real implementation, both would be submitted
    return order1_;
}

void OCOOrder::update(const OrderBook& book) {
    if (order1_->get_status() == core::OrderStatus::FILLED) {
        // Cancel order2
        completed_ = true;
    } else if (order2_->get_status() == core::OrderStatus::FILLED) {
        // Cancel order1
        completed_ = true;
    }
}

// ============================================================================
// TWAPOrder Implementation
// ============================================================================

TWAPOrder::TWAPOrder(core::OrderId id, const std::string& symbol,
                     core::Side side, core::Price limit_price, core::Volume total_quantity,
                     core::Timestamp start_time, core::Timestamp end_time, int num_slices)
    : AdvancedOrder(id, symbol)
    , side_(side)
    , limit_price_(limit_price)
    , total_quantity_(total_quantity)
    , start_time_(start_time)
    , end_time_(end_time)
    , num_slices_(num_slices)
    , current_slice_(0)
    , remaining_quantity_(total_quantity)
    , next_slice_time_(start_time)
{
    slice_size_ = total_quantity_ / num_slices_;
}

bool TWAPOrder::should_trigger(const OrderBook& book) const {
    if (is_complete()) return false;
    
    auto now = core::TimestampUtil::now();
    return now >= next_slice_time_ && now <= end_time_;
}

OrderPtr TWAPOrder::create_execution_order() {
    auto qty = std::min(slice_size_, remaining_quantity_);
    
    auto order = std::make_shared<Order>(
        id_ + current_slice_, symbol_, side_, 
        core::OrderType::LIMIT, limit_price_, qty
    );
    
    current_slice_++;
    remaining_quantity_ -= qty;
    
    // Calculate next slice time
    auto slice_interval = (end_time_ - start_time_) / num_slices_;
    next_slice_time_ += slice_interval;
    
    return order;
}

bool TWAPOrder::is_complete() const {
    return remaining_quantity_ == 0 || current_slice_ >= num_slices_;
}

void TWAPOrder::update(const OrderBook& book) {
    // TWAP updates happen based on time, checked in should_trigger
}

// ============================================================================
// AdvancedOrderManager Implementation
// ============================================================================

AdvancedOrderManager::AdvancedOrderManager() {}

void AdvancedOrderManager::add_order(AdvancedOrderPtr order) {
    orders_[order->get_id()] = order;
}

void AdvancedOrderManager::cancel_order(core::OrderId order_id) {
    orders_.erase(order_id);
}

void AdvancedOrderManager::update_all(const OrderBook& book) {
    for (auto& [id, order] : orders_) {
        if (!order->is_complete()) {
            order->update(book);
        }
    }
}

std::vector<OrderPtr> AdvancedOrderManager::get_triggered_orders(const OrderBook& book) {
    std::vector<OrderPtr> triggered;
    
    for (auto& [id, order] : orders_) {
        if (!order->is_complete() && order->should_trigger(book)) {
            auto execution_order = order->create_execution_order();
            if (execution_order) {
                triggered.push_back(execution_order);
            }
        }
    }
    
    return triggered;
}

AdvancedOrderPtr AdvancedOrderManager::get_order(core::OrderId order_id) const {
    auto it = orders_.find(order_id);
    return it != orders_.end() ? it->second : nullptr;
}

std::vector<AdvancedOrderPtr> AdvancedOrderManager::get_active_orders() const {
    std::vector<AdvancedOrderPtr> active;
    
    for (const auto& [id, order] : orders_) {
        if (!order->is_complete()) {
            active.push_back(order);
        }
    }
    
    return active;
}

} // namespace orderbook
} // namespace hft
