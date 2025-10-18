#ifndef HFT_SERVER_TRADING_SERVER_H
#define HFT_SERVER_TRADING_SERVER_H

#include "../orderbook/order_book.h"
#include "../risk/risk_manager.h"
#include "../market_data/market_data.h"
#include "../signals/microstructure_signals.h"
#include <map>
#include <string>
#include <memory>

namespace hft {
namespace server {

class TradingServer {
public:
    TradingServer();
    
    void initialize();
    
    bool submit_order(orderbook::OrderPtr order);
    bool cancel_order(core::OrderId order_id, const std::string& symbol);
    bool modify_order(core::OrderId order_id, const std::string& symbol,
                     core::Price new_price, core::Volume new_quantity);
    
    market_data::Quote get_quote(const std::string& symbol) const;
    
    risk::RiskMetrics get_risk_metrics() const;
    risk::Position get_position(const std::string& symbol) const;
    
    signals::MicrostructureData get_microstructure_signals(const std::string& symbol);
    
    orderbook::OrderBookPtr get_order_book(const std::string& symbol);
    
private:
    std::map<std::string, orderbook::OrderBookPtr> order_books_;
    std::shared_ptr<risk::RiskManager> risk_manager_;
    std::shared_ptr<market_data::MarketDataManager> market_data_;
    std::shared_ptr<signals::MicrostructureSignals> microstructure_;
    
    orderbook::OrderBookPtr get_or_create_order_book(const std::string& symbol);
};

} // namespace server
} // namespace hft

#endif // HFT_SERVER_TRADING_SERVER_H
