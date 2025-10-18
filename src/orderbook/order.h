#ifndef HFT_ORDERBOOK_ORDER_H
#define HFT_ORDERBOOK_ORDER_H

#include "../core/types.h"
#include "../core/timestamp.h"
#include <string>
#include <memory>

namespace hft {
namespace orderbook {

class Order {
public:
    Order(core::OrderId id, const std::string& symbol, core::Side side,
          core::OrderType type, core::Price price, core::Volume quantity,
          core::TimeInForce tif = core::TimeInForce::DAY);
    
    core::OrderId get_id() const { return id_; }
    const std::string& get_symbol() const { return symbol_; }
    core::Side get_side() const { return side_; }
    core::OrderType get_type() const { return type_; }
    core::Price get_price() const { return price_; }
    core::Volume get_quantity() const { return quantity_; }
    core::Volume get_filled_quantity() const { return filled_quantity_; }
    core::Volume get_remaining_quantity() const { return quantity_ - filled_quantity_; }
    core::OrderStatus get_status() const { return status_; }
    core::TimeInForce get_time_in_force() const { return time_in_force_; }
    core::Timestamp get_timestamp() const { return timestamp_; }
    
    core::Volume get_display_quantity() const { return display_quantity_; }
    core::Price get_stop_price() const { return stop_price_; }
    core::Price get_trailing_amount() const { return trailing_amount_; }
    core::Price get_peg_offset() const { return peg_offset_; }
    
    void set_display_quantity(core::Volume qty) { display_quantity_ = qty; }
    void set_stop_price(core::Price price) { stop_price_ = price; }
    void set_trailing_amount(core::Price amount) { trailing_amount_ = amount; }
    void set_peg_offset(core::Price offset) { peg_offset_ = offset; }
    
    void set_price(core::Price price) { price_ = price; }
    void set_quantity(core::Volume quantity) { quantity_ = quantity; }
    void add_fill(core::Volume quantity);
    void set_status(core::OrderStatus status) { status_ = status; }
    
    bool is_active() const;
    bool is_fillable() const;
    
    std::string to_string() const;
    
private:
    core::OrderId id_;
    std::string symbol_;
    core::Side side_;
    core::OrderType type_;
    core::Price price_;
    core::Volume quantity_;
    core::Volume filled_quantity_;
    core::OrderStatus status_;
    core::TimeInForce time_in_force_;
    core::Timestamp timestamp_;
    
    core::Volume display_quantity_;
    core::Price stop_price_;
    core::Price trailing_amount_;
    core::Price peg_offset_;
};

using OrderPtr = std::shared_ptr<Order>;

} // namespace orderbook
} // namespace hft

#endif // HFT_ORDERBOOK_ORDER_H
