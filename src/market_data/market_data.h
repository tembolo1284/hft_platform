#ifndef HFT_MARKET_DATA_MARKET_DATA_H
#define HFT_MARKET_DATA_MARKET_DATA_H

#include "../core/types.h"
#include "../core/timestamp.h"
#include <string>
#include <map>

namespace hft {
namespace market_data {

struct Quote {
    std::string symbol;
    core::Price bid;
    core::Price ask;
    core::Volume bid_size;
    core::Volume ask_size;
    core::Timestamp timestamp;
};

struct Trade {
    std::string symbol;
    core::Price price;
    core::Volume volume;
    core::Timestamp timestamp;
};

class MarketDataManager {
public:
    MarketDataManager();
    
    void subscribe(const std::string& symbol);
    void unsubscribe(const std::string& symbol);
    
    Quote get_quote(const std::string& symbol) const;
    Trade get_last_trade(const std::string& symbol) const;
    
    void update_quote(const Quote& quote);
    void update_trade(const Trade& trade);
    
private:
    std::map<std::string, Quote> quotes_;
    std::map<std::string, Trade> trades_;
};

} // namespace market_data
} // namespace hft

#endif // HFT_MARKET_DATA_MARKET_DATA_H
