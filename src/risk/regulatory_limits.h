#ifndef HFT_RISK_REGULATORY_LIMITS_H
#define HFT_RISK_REGULATORY_LIMITS_H

#include "../core/types.h"
#include "../orderbook/order.h"
#include <map>
#include <string>
#include <vector>

namespace hft {
namespace risk {

enum class LimitType {
    POSITION_LIMIT,
    ORDER_SIZE_LIMIT,
    NOTIONAL_LIMIT,
    DELTA_LIMIT,
    GAMMA_LIMIT,
    VEGA_LIMIT,
    DAILY_LOSS_LIMIT,
    ORDER_RATE_LIMIT,
    CANCEL_RATE_LIMIT
};

struct ExchangeLimits {
    core::Exchange exchange;
    core::AssetClass asset_class;
    
    int64_t max_position;
    int64_t max_order_size;
    double max_notional_usd;
    
    double max_delta;
    double max_delta_ratio;
    
    double max_gamma;
    double max_vega;
    
    double max_daily_loss_usd;
    double max_drawdown_pct;
    
    int max_orders_per_second;
    int max_cancels_per_second;
    double max_cancel_ratio;
    
    double price_deviation_pct;
    bool circuit_breaker_enabled;
    
    ExchangeLimits() 
        : exchange(core::Exchange::CME)
        , asset_class(core::AssetClass::FUTURES)
        , max_position(10000)
        , max_order_size(1000)
        , max_notional_usd(10000000.0)
        , max_delta(5000.0)
        , max_delta_ratio(0.05)
        , max_gamma(1000.0)
        , max_vega(10000.0)
        , max_daily_loss_usd(100000.0)
        , max_drawdown_pct(0.10)
        , max_orders_per_second(1000)
        , max_cancels_per_second(800)
        , max_cancel_ratio(0.97)
        , price_deviation_pct(0.10)
        , circuit_breaker_enabled(true)
    {}
};

class CMELimits {
public:
    static ExchangeLimits get_futures_limits(const std::string& contract);
    static ExchangeLimits get_options_limits(const std::string& contract);
    static ExchangeLimits get_es_limits();
    static ExchangeLimits get_ge_limits();
    static ExchangeLimits get_cl_limits();
    static int64_t get_accountability_level(const std::string& contract);
};

class NASDAQLimits {
public:
    static ExchangeLimits get_equity_limits(const std::string& symbol);
    static ExchangeLimits get_etf_limits(const std::string& symbol);
    static ExchangeLimits get_large_cap_limits();
    static ExchangeLimits get_small_cap_limits();
    static double get_luld_threshold(const std::string& symbol, double reference_price);
};

class RegulatoryChecker {
public:
    RegulatoryChecker();
    
    void set_limits(const std::string& symbol, const ExchangeLimits& limits);
    const ExchangeLimits& get_limits(const std::string& symbol) const;
    
    bool check_order(const orderbook::OrderPtr& order, int64_t current_position,
                    double current_delta = 0.0);
    bool check_delta_limit(const std::string& symbol, double proposed_delta);
    bool check_position_limit(const std::string& symbol, int64_t proposed_position);
    bool check_order_size_limit(const std::string& symbol, core::Volume order_size);
    bool check_notional_limit(const std::string& symbol, double notional_value);
    bool check_price_deviation(const std::string& symbol, core::Price proposed_price,
                              core::Price reference_price);
    bool check_order_rate(const std::string& symbol, int orders_in_last_second);
    bool check_cancel_rate(const std::string& symbol, int cancels_in_last_second,
                          int orders_in_last_period);
    
    std::string get_last_violation() const { return last_violation_; }
    
private:
    std::map<std::string, ExchangeLimits> symbol_limits_;
    ExchangeLimits default_limits_;
    std::string last_violation_;
    
    void set_violation(const std::string& reason) { last_violation_ = reason; }
};

} // namespace risk
} // namespace hft

#endif // HFT_RISK_REGULATORY_LIMITS_H
