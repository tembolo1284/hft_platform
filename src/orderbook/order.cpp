#include "order.h"
#include <sstream>

namespace hft {
namespace orderbook {

Order::Order(core::OrderId id, const std::string& symbol, core::Side side,
             core::OrderType type, core::Price price, core::Volume quantity,
             core::TimeInForce tif)
    : id_(id)
    , symbol_(symbol)
    , side_(side)
    , type_(type)
    , price_(price)
    , quantity_(quantity)
    , filled_quantity_(0)
    , status_(core::OrderStatus::NEW)
    , time_in_force_(tif)
    , timestamp_(core::TimestampUtil::now())
    , display_quantity_(0)
    , stop_price_(0)
    , trailing_amount_(0)
    , peg_offset_(0)
{}

void Order::add_fill(core::Volume quantity) {
    filled_quantity_ += quantity;
    
    if (filled_quantity_ >= quantity_) {
        status_ = core::OrderStatus::FILLED;
    } else {
        status_ = core::OrderStatus::PARTIALLY_FILLED;
    }
}

bool Order::is_active() const {
    return status_ == core::OrderStatus::NEW || 
           status_ == core::OrderStatus::PARTIALLY_FILLED;
}

bool Order::is_fillable() const {
    return is_active() && get_remaining_quantity() > 0;
}

std::string Order::to_string() const {
    std::stringstream ss;
    ss << "Order[id=" << id_
       << ", symbol=" << symbol_
       << ", side=" << core::side_to_string(side_)
       << ", type=" << core::order_type_to_string(type_)
       << ", price=" << core::price_to_double(price_)
       << ", qty=" << quantity_
       << ", filled=" << filled_quantity_
       << ", status=" << static_cast<int>(status_)
       << "]";
    return ss.str();
}

} // namespace orderbook
} // namespace hft
