#ifndef HFT_RISK_RISK_MANAGER_H
#define HFT_RISK_RISK_MANAGER_H

#include "regulatory_limits.h"
#include "../orderbook/order.h"
#include "../core/types.h"
#include <map>
#include <string>

namespace hft {
namespace risk {

struct Position {
    std::string symbol;
    int64_t quantity;
    double average_price;
    double realized_pnl;
    double unrealized_pnl;
    double delta;
    double gamma;
    double vega;
    core::Timestamp last_update;
    
    Position() 
        : quantity(0), average_price(0.0), realized_pnl(0.0), 
          unrealized_pnl(0.0), delta(0.0), gamma(0.0), vega(0.0),
          last_update(0)
    {}
};

struct RiskMetrics {
    double total_exposure_usd;
    double var_1day_95;
    double max_drawdown;
    double sharpe_ratio;
    double current_leverage;
    int active_positions;
    double daily_pnl;
    core::Timestamp timestamp;
};

class RiskManager {
public:
    RiskManager();
    
    void set_regulatory_checker(std::shared_ptr<RegulatoryChecker> checker);
    
    bool check_order(const orderbook::OrderPtr& order);
    
    void on_fill(const orderbook::OrderPtr& order, core::Volume fill_quantity, core::Price fill_price);
    
    const Position& get_position(const std::string& symbol) const;
    void update_position_prices(const std::string& symbol, core::Price market_price);
    
    RiskMetrics calculate_metrics() const;
    double get_total_delta(const std::string& symbol) const;
    double get_portfolio_delta() const;
    
    void set_max_portfolio_delta(double delta) { max_portfolio_delta_ = delta; }
    void set_max_leverage(double leverage) { max_leverage_ = leverage; }
    void set_max_var(double var) { max_var_ = var; }
    
    const std::map<std::string, Position>& get_positions() const { return positions_; }
    
private:
    std::shared_ptr<RegulatoryChecker> regulatory_checker_;
    std::map<std::string, Position> positions_;
    
    double max_portfolio_delta_;
    double max_leverage_;
    double max_var_;
    
    bool check_portfolio_limits(const orderbook::OrderPtr& order);
    
    void update_position(const std::string& symbol, int64_t quantity_change, 
                        core::Price fill_price);
};

} // namespace risk
} // namespace hft

#endif // HFT_RISK_RISK_MANAGER_H
