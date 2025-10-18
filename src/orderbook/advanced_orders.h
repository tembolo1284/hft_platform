#ifndef HFT_ORDERBOOK_ADVANCED_ORDERS_H
#define HFT_ORDERBOOK_ADVANCED_ORDERS_H

#include "order.h"
#include "order_book.h"
#include "../core/types.h"
#include <memory>
#include <functional>

namespace hft {
namespace orderbook {

// Advanced order types beyond basic LIMIT and MARKET

enum class AdvancedOrderType {
    STOP_LOSS,              // Trigger market order when price hits stop
    STOP_LIMIT,             // Trigger limit order when price hits stop
    TRAILING_STOP,          // Stop that trails price by fixed distance
    ICEBERG,                // Display only partial quantity
    PEGGED,                 // Peg to best bid/ask with offset
    TIME_IN_FORCE_IOC,      // Immediate or Cancel
    TIME_IN_FORCE_FOK,      // Fill or Kill
    BRACKET,                // Parent order with stop loss and take profit
    OCO,                    // One Cancels Other
    TWAP                    // Time-Weighted Average Price
};

// Base class for advanced order behavior
class AdvancedOrder {
public:
    virtual ~AdvancedOrder() = default;
    
    virtual bool should_trigger(const OrderBook& book) const = 0;
    virtual OrderPtr create_execution_order() = 0;
    virtual bool is_complete() const = 0;
    virtual void update(const OrderBook& book) = 0;
    
    core::OrderId get_id() const { return id_; }
    const std::string& get_symbol() const { return symbol_; }
    
protected:
    AdvancedOrder(core::OrderId id, const std::string& symbol)
        : id_(id), symbol_(symbol) {}
    
    core::OrderId id_;
    std::string symbol_;
};

using AdvancedOrderPtr = std::shared_ptr<AdvancedOrder>;

// Stop Loss Order - triggers market sell when price falls below stop
class StopLossOrder : public AdvancedOrder {
public:
    StopLossOrder(core::OrderId id, const std::string& symbol,
                  core::Side side, core::Price stop_price, core::Volume quantity);
    
    bool should_trigger(const OrderBook& book) const override;
    OrderPtr create_execution_order() override;
    bool is_complete() const override { return triggered_; }
    void update(const OrderBook& book) override;
    
private:
    core::Side side_;
    core::Price stop_price_;
    core::Volume quantity_;
    bool triggered_;
};

// Stop Limit Order - triggers limit order when price hits stop
class StopLimitOrder : public AdvancedOrder {
public:
    StopLimitOrder(core::OrderId id, const std::string& symbol,
                   core::Side side, core::Price stop_price, 
                   core::Price limit_price, core::Volume quantity);
    
    bool should_trigger(const OrderBook& book) const override;
    OrderPtr create_execution_order() override;
    bool is_complete() const override { return triggered_; }
    void update(const OrderBook& book) override;
    
private:
    core::Side side_;
    core::Price stop_price_;
    core::Price limit_price_;
    core::Volume quantity_;
    bool triggered_;
};

// Trailing Stop - stop that moves with favorable price movement
class TrailingStopOrder : public AdvancedOrder {
public:
    TrailingStopOrder(core::OrderId id, const std::string& symbol,
                      core::Side side, core::Price trail_distance, core::Volume quantity);
    
    bool should_trigger(const OrderBook& book) const override;
    OrderPtr create_execution_order() override;
    bool is_complete() const override { return triggered_; }
    void update(const OrderBook& book) override;
    
private:
    core::Side side_;
    core::Price trail_distance_;
    core::Volume quantity_;
    core::Price current_stop_;
    core::Price best_price_;
    bool triggered_;
};

// Iceberg Order - displays only partial quantity, replenishes as filled
class IcebergOrder : public AdvancedOrder {
public:
    IcebergOrder(core::OrderId id, const std::string& symbol,
                 core::Side side, core::Price price, 
                 core::Volume total_quantity, core::Volume display_quantity);
    
    bool should_trigger(const OrderBook& book) const override { return !is_complete(); }
    OrderPtr create_execution_order() override;
    bool is_complete() const override { return remaining_quantity_ == 0; }
    void update(const OrderBook& book) override;
    void on_fill(core::Volume filled_qty);
    
private:
    core::Side side_;
    core::Price price_;
    core::Volume total_quantity_;
    core::Volume display_quantity_;
    core::Volume remaining_quantity_;
    OrderPtr current_order_;
};

// Pegged Order - price automatically adjusts to maintain offset from best bid/ask
class PeggedOrder : public AdvancedOrder {
public:
    PeggedOrder(core::OrderId id, const std::string& symbol,
                core::Side side, core::Price offset, core::Volume quantity);
    
    bool should_trigger(const OrderBook& book) const override { return true; }
    OrderPtr create_execution_order() override;
    bool is_complete() const override { return filled_; }
    void update(const OrderBook& book) override;
    
private:
    core::Side side_;
    core::Price offset_;
    core::Volume quantity_;
    core::Price current_price_;
    bool filled_;
};

// IOC (Immediate or Cancel) - fill immediately or cancel unfilled portion
class IOCOrder : public AdvancedOrder {
public:
    IOCOrder(core::OrderId id, const std::string& symbol,
             core::Side side, core::Price price, core::Volume quantity);
    
    bool should_trigger(const OrderBook& book) const override { return !executed_; }
    OrderPtr create_execution_order() override;
    bool is_complete() const override { return executed_; }
    void update(const OrderBook& book) override;
    
private:
    core::Side side_;
    core::Price price_;
    core::Volume quantity_;
    bool executed_;
};

// FOK (Fill or Kill) - fill entire quantity immediately or cancel
class FOKOrder : public AdvancedOrder {
public:
    FOKOrder(core::OrderId id, const std::string& symbol,
             core::Side side, core::Price price, core::Volume quantity);
    
    bool should_trigger(const OrderBook& book) const override;
    OrderPtr create_execution_order() override;
    bool is_complete() const override { return checked_; }
    void update(const OrderBook& book) override;
    
private:
    core::Side side_;
    core::Price price_;
    core::Volume quantity_;
    bool checked_;
    bool can_fill_;
};

// Bracket Order - parent order with automatic stop loss and take profit
class BracketOrder : public AdvancedOrder {
public:
    BracketOrder(core::OrderId id, const std::string& symbol,
                 core::Side side, core::Price entry_price, core::Volume quantity,
                 core::Price stop_loss, core::Price take_profit);
    
    bool should_trigger(const OrderBook& book) const override;
    OrderPtr create_execution_order() override;
    bool is_complete() const override;
    void update(const OrderBook& book) override;
    void on_parent_fill();
    
private:
    core::Side side_;
    core::Price entry_price_;
    core::Volume quantity_;
    core::Price stop_loss_price_;
    core::Price take_profit_price_;
    
    bool parent_filled_;
    bool exited_;
    OrderPtr parent_order_;
};

// OCO (One Cancels Other) - two orders where filling one cancels the other
class OCOOrder : public AdvancedOrder {
public:
    OCOOrder(core::OrderId id, const std::string& symbol,
             OrderPtr order1, OrderPtr order2);
    
    bool should_trigger(const OrderBook& book) const override;
    OrderPtr create_execution_order() override;
    bool is_complete() const override { return completed_; }
    void update(const OrderBook& book) override;
    
private:
    OrderPtr order1_;
    OrderPtr order2_;
    bool completed_;
};

// TWAP (Time-Weighted Average Price) - splits order over time
class TWAPOrder : public AdvancedOrder {
public:
    TWAPOrder(core::OrderId id, const std::string& symbol,
              core::Side side, core::Price limit_price, core::Volume total_quantity,
              core::Timestamp start_time, core::Timestamp end_time, int num_slices);
    
    bool should_trigger(const OrderBook& book) const override;
    OrderPtr create_execution_order() override;
    bool is_complete() const override;
    void update(const OrderBook& book) override;
    
private:
    core::Side side_;
    core::Price limit_price_;
    core::Volume total_quantity_;
    core::Timestamp start_time_;
    core::Timestamp end_time_;
    int num_slices_;
    
    int current_slice_;
    core::Volume slice_size_;
    core::Volume remaining_quantity_;
    core::Timestamp next_slice_time_;
};

// Manager for advanced orders
class AdvancedOrderManager {
public:
    AdvancedOrderManager();
    
    void add_order(AdvancedOrderPtr order);
    void cancel_order(core::OrderId order_id);
    
    void update_all(const OrderBook& book);
    std::vector<OrderPtr> get_triggered_orders(const OrderBook& book);
    
    AdvancedOrderPtr get_order(core::OrderId order_id) const;
    std::vector<AdvancedOrderPtr> get_active_orders() const;
    
private:
    std::map<core::OrderId, AdvancedOrderPtr> orders_;
};

} // namespace orderbook
} // namespace hft

#endif // HFT_ORDERBOOK_ADVANCED_ORDERS_H
