#include "market_data.h"

namespace hft {
namespace market_data {

MarketDataManager::MarketDataManager() {}

void MarketDataManager::subscribe(const std::string& symbol) {
    quotes_[symbol] = Quote();
    trades_[symbol] = Trade();
}

void MarketDataManager::unsubscribe(const std::string& symbol) {
    quotes_.erase(symbol);
    trades_.erase(symbol);
}

Quote MarketDataManager::get_quote(const std::string& symbol) const {
    auto it = quotes_.find(symbol);
    return (it != quotes_.end()) ? it->second : Quote();
}

Trade MarketDataManager::get_last_trade(const std::string& symbol) const {
    auto it = trades_.find(symbol);
    return (it != trades_.end()) ? it->second : Trade();
}

void MarketDataManager::update_quote(const Quote& quote) {
    quotes_[quote.symbol] = quote;
}

void MarketDataManager::update_trade(const Trade& trade) {
    trades_[trade.symbol] = trade;
}

} // namespace market_data
} // namespace hft
