#include "trading_server.h"

namespace hft {
namespace server {

TradingServer::TradingServer()
    : risk_manager_(std::make_shared<risk::RiskManager>())
    , market_data_(std::make_shared<market_data::MarketDataManager>())
    , microstructure_(std::make_shared<signals::MicrostructureSignals>())
{}

void TradingServer::initialize() {
    // Initialize server components
}

bool TradingServer::submit_order(orderbook::OrderPtr order) {
    if (!risk_manager_->check_order(order)) {
        return false;
    }
    
    auto book = get_or_create_order_book(order->get_symbol());
    
    bool success = book->add_order(order);
    
    if (success && order->get_filled_quantity() > 0) {
        risk_manager_->on_fill(order, order->get_filled_quantity(), order->get_price());
    }
    
    return success;
}

bool TradingServer::cancel_order(core::OrderId order_id, const std::string& symbol) {
    auto it = order_books_.find(symbol);
    if (it == order_books_.end()) {
        return false;
    }
    
    return it->second->cancel_order(order_id);
}

bool TradingServer::modify_order(core::OrderId order_id, const std::string& symbol,
                                core::Price new_price, core::Volume new_quantity) {
    auto it = order_books_.find(symbol);
    if (it == order_books_.end()) {
        return false;
    }
    
    return it->second->modify_order(order_id, new_price, new_quantity);
}

market_data::Quote TradingServer::get_quote(const std::string& symbol) const {
    return market_data_->get_quote(symbol);
}

risk::RiskMetrics TradingServer::get_risk_metrics() const {
    return risk_manager_->calculate_metrics();
}

risk::Position TradingServer::get_position(const std::string& symbol) const {
    return risk_manager_->get_position(symbol);
}

signals::MicrostructureData TradingServer::get_microstructure_signals(const std::string& symbol) {
    auto book = get_order_book(symbol);
    if (!book) {
        return signals::MicrostructureData();
    }
    
    return microstructure_->analyze(*book);
}

orderbook::OrderBookPtr TradingServer::get_order_book(const std::string& symbol) {
    auto it = order_books_.find(symbol);
    return (it != order_books_.end()) ? it->second : nullptr;
}

orderbook::OrderBookPtr TradingServer::get_or_create_order_book(const std::string& symbol) {
    auto it = order_books_.find(symbol);
    if (it == order_books_.end()) {
        auto book = std::make_shared<orderbook::OrderBook>(symbol);
        order_books_[symbol] = book;
        return book;
    }
    return it->second;
}

} // namespace server
} // namespace hft
