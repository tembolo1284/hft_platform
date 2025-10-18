#include "risk_manager.h"
#include <cmath>
#include <algorithm>

namespace hft {
namespace risk {

RiskManager::RiskManager()
    : max_portfolio_delta_(100000.0)
    , max_leverage_(3.0)
    , max_var_(500000.0)
{}

void RiskManager::set_regulatory_checker(std::shared_ptr<RegulatoryChecker> checker) {
    regulatory_checker_ = checker;
}

bool RiskManager::check_order(const orderbook::OrderPtr& order) {
    if (regulatory_checker_) {
        auto it = positions_.find(order->get_symbol());
        int64_t current_position = (it != positions_.end()) ? it->second.quantity : 0;
        double current_delta = (it != positions_.end()) ? it->second.delta : 0.0;
        
        if (!regulatory_checker_->check_order(order, current_position, current_delta)) {
            return false;
        }
    }
    
    return check_portfolio_limits(order);
}

void RiskManager::on_fill(const orderbook::OrderPtr& order, core::Volume fill_quantity, 
                         core::Price fill_price) {
    int64_t quantity_change = (order->get_side() == core::Side::BUY) ? 
                              fill_quantity : -static_cast<int64_t>(fill_quantity);
    
    update_position(order->get_symbol(), quantity_change, fill_price);
}

const Position& RiskManager::get_position(const std::string& symbol) const {
    static Position empty_position;
    auto it = positions_.find(symbol);
    return (it != positions_.end()) ? it->second : empty_position;
}

void RiskManager::update_position_prices(const std::string& symbol, core::Price market_price) {
    auto it = positions_.find(symbol);
    if (it != positions_.end()) {
        Position& pos = it->second;
        double market_price_double = core::price_to_double(market_price);
        
        pos.unrealized_pnl = (market_price_double - pos.average_price) * pos.quantity;
        pos.last_update = core::TimestampUtil::now();
    }
}

RiskMetrics RiskManager::calculate_metrics() const {
    RiskMetrics metrics;
    metrics.timestamp = core::TimestampUtil::now();
    metrics.active_positions = positions_.size();
    
    double total_exposure = 0.0;
    double daily_pnl = 0.0;
    
    for (const auto& [symbol, pos] : positions_) {
        total_exposure += std::abs(pos.average_price * pos.quantity);
        daily_pnl += pos.realized_pnl + pos.unrealized_pnl;
    }
    
    metrics.total_exposure_usd = total_exposure;
    metrics.daily_pnl = daily_pnl;
    
    metrics.var_1day_95 = total_exposure * 0.02;
    metrics.max_drawdown = 0.0;
    metrics.sharpe_ratio = 0.0;
    metrics.current_leverage = total_exposure / 1000000.0;
    
    return metrics;
}

double RiskManager::get_total_delta(const std::string& symbol) const {
    auto it = positions_.find(symbol);
    return (it != positions_.end()) ? it->second.delta : 0.0;
}

double RiskManager::get_portfolio_delta() const {
    double total_delta = 0.0;
    for (const auto& [symbol, pos] : positions_) {
        total_delta += pos.delta;
    }
    return total_delta;
}

bool RiskManager::check_portfolio_limits(const orderbook::OrderPtr& order) {
    int64_t quantity_change = (order->get_side() == core::Side::BUY) ? 
                              order->get_quantity() : -static_cast<int64_t>(order->get_quantity());
    
    double proposed_portfolio_delta = get_portfolio_delta() + quantity_change;
    
    if (std::abs(proposed_portfolio_delta) > max_portfolio_delta_) {
        return false;
    }
    
    return true;
}

void RiskManager::update_position(const std::string& symbol, int64_t quantity_change, 
                                  core::Price fill_price) {
    Position& pos = positions_[symbol];
    pos.symbol = symbol;
    
    double fill_price_double = core::price_to_double(fill_price);
    
    if ((pos.quantity > 0 && quantity_change > 0) || (pos.quantity < 0 && quantity_change < 0)) {
        double old_value = pos.average_price * std::abs(pos.quantity);
        double new_value = fill_price_double * std::abs(quantity_change);
        double total_quantity = std::abs(pos.quantity + quantity_change);
        
        if (total_quantity > 0) {
            pos.average_price = (old_value + new_value) / total_quantity;
        }
    } else if ((pos.quantity > 0 && quantity_change < 0) || (pos.quantity < 0 && quantity_change > 0)) {
        int64_t closed_quantity = std::min(std::abs(pos.quantity), std::abs(quantity_change));
        double pnl_per_share = fill_price_double - pos.average_price;
        
        if (pos.quantity < 0) {
            pnl_per_share = -pnl_per_share;
        }
        
        pos.realized_pnl += pnl_per_share * closed_quantity;
    }
    
    pos.quantity += quantity_change;
    pos.delta = pos.quantity;
    pos.last_update = core::TimestampUtil::now();
    
    if (pos.quantity == 0) {
        positions_.erase(symbol);
    }
}

} // namespace risk
} // namespace hft
