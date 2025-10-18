#include "regulatory_limits.h"
#include <cmath>
#include <sstream>

namespace hft {
namespace risk {

ExchangeLimits CMELimits::get_futures_limits(const std::string& contract) {
    if (contract == "ES") return get_es_limits();
    if (contract == "GE") return get_ge_limits();
    if (contract == "CL") return get_cl_limits();
    
    ExchangeLimits limits;
    limits.exchange = core::Exchange::CME;
    limits.asset_class = core::AssetClass::FUTURES;
    return limits;
}

ExchangeLimits CMELimits::get_options_limits(const std::string& contract) {
    (void) contract;
    ExchangeLimits limits;
    limits.exchange = core::Exchange::CME;
    limits.asset_class = core::AssetClass::OPTIONS;
    
    limits.max_position = 50000;
    limits.max_gamma = 10000.0;
    limits.max_vega = 50000.0;
    
    return limits;
}

ExchangeLimits CMELimits::get_es_limits() {
    ExchangeLimits limits;
    limits.exchange = core::Exchange::CME;
    limits.asset_class = core::AssetClass::FUTURES;
    
    limits.max_position = 20000;
    limits.max_order_size = 500;
    limits.max_notional_usd = 50000000.0;
    
    limits.max_delta = 40000.0;
    limits.max_delta_ratio = 0.05;
    
    limits.max_daily_loss_usd = 500000.0;
    limits.max_drawdown_pct = 0.10;
    
    limits.max_orders_per_second = 2000;
    limits.max_cancels_per_second = 1800;
    limits.max_cancel_ratio = 0.97;
    
    limits.price_deviation_pct = 0.05;
    limits.circuit_breaker_enabled = true;
    
    return limits;
}

ExchangeLimits CMELimits::get_ge_limits() {
    ExchangeLimits limits;
    limits.exchange = core::Exchange::CME;
    limits.asset_class = core::AssetClass::FUTURES;
    
    limits.max_position = 100000;
    limits.max_order_size = 5000;
    limits.max_notional_usd = 25000000.0;
    
    limits.max_delta = 100000.0;
    limits.max_delta_ratio = 0.10;
    
    limits.max_daily_loss_usd = 250000.0;
    limits.max_drawdown_pct = 0.08;
    
    limits.max_orders_per_second = 3000;
    limits.max_cancels_per_second = 2700;
    limits.max_cancel_ratio = 0.97;
    
    limits.price_deviation_pct = 0.10;
    limits.circuit_breaker_enabled = true;
    
    return limits;
}

ExchangeLimits CMELimits::get_cl_limits() {
    ExchangeLimits limits;
    limits.exchange = core::Exchange::CME;
    limits.asset_class = core::AssetClass::FUTURES;
    
    limits.max_position = 10000;
    limits.max_order_size = 500;
    limits.max_notional_usd = 100000000.0;
    
    limits.max_delta = 20000.0;
    limits.max_delta_ratio = 0.05;
    
    limits.max_daily_loss_usd = 1000000.0;
    limits.max_drawdown_pct = 0.15;
    
    limits.max_orders_per_second = 1500;
    limits.max_cancels_per_second = 1350;
    limits.max_cancel_ratio = 0.97;
    
    limits.price_deviation_pct = 0.07;
    limits.circuit_breaker_enabled = true;
    
    return limits;
}

int64_t CMELimits::get_accountability_level(const std::string& contract) {
    if (contract == "ES") return 20000;
    if (contract == "GE") return 100000;
    if (contract == "CL") return 10000;
    return 5000;
}

ExchangeLimits NASDAQLimits::get_equity_limits(const std::string& symbol) {
    (void) symbol;
    return get_large_cap_limits();
}

ExchangeLimits NASDAQLimits::get_etf_limits(const std::string& symbol) {
    (void) symbol;
    ExchangeLimits limits = get_large_cap_limits();
    
    limits.max_position = 2000000;
    limits.max_order_size = 100000;
    
    return limits;
}

ExchangeLimits NASDAQLimits::get_large_cap_limits() {
    ExchangeLimits limits;
    limits.exchange = core::Exchange::NASDAQ;
    limits.asset_class = core::AssetClass::EQUITY;
    
    limits.max_position = 1000000;
    limits.max_order_size = 50000;
    limits.max_notional_usd = 100000000.0;
    
    limits.max_delta = 1000000.0;
    limits.max_delta_ratio = 0.05;
    
    limits.max_daily_loss_usd = 1000000.0;
    limits.max_drawdown_pct = 0.10;
    
    limits.max_orders_per_second = 500;
    limits.max_cancels_per_second = 450;
    limits.max_cancel_ratio = 0.95;
    
    limits.price_deviation_pct = 0.05;
    limits.circuit_breaker_enabled = true;
    
    return limits;
}

ExchangeLimits NASDAQLimits::get_small_cap_limits() {
    ExchangeLimits limits;
    limits.exchange = core::Exchange::NASDAQ;
    limits.asset_class = core::AssetClass::EQUITY;
    
    limits.max_position = 500000;
    limits.max_order_size = 25000;
    limits.max_notional_usd = 25000000.0;
    
    limits.max_delta = 500000.0;
    limits.max_delta_ratio = 0.10;
    
    limits.max_daily_loss_usd = 250000.0;
    limits.max_drawdown_pct = 0.15;
    
    limits.max_orders_per_second = 300;
    limits.max_cancels_per_second = 270;
    limits.max_cancel_ratio = 0.95;
    
    limits.price_deviation_pct = 0.10;
    limits.circuit_breaker_enabled = true;
    
    return limits;
}

double NASDAQLimits::get_luld_threshold(const std::string& symbol, double reference_price) {
    (void) symbol;
    if (reference_price >= 3.0) {
        return reference_price * 0.05;
    } else if (reference_price >= 0.75) {
        return std::min(0.15, reference_price * 0.20);
    } else {
        return std::min(0.15, reference_price * 0.20);
    }
}

RegulatoryChecker::RegulatoryChecker() {
    default_limits_.max_position = 10000;
    default_limits_.max_order_size = 1000;
    default_limits_.max_delta = 10000.0;
}

void RegulatoryChecker::set_limits(const std::string& symbol, const ExchangeLimits& limits) {
    symbol_limits_[symbol] = limits;
}

const ExchangeLimits& RegulatoryChecker::get_limits(const std::string& symbol) const {
    auto it = symbol_limits_.find(symbol);
    if (it != symbol_limits_.end()) {
        return it->second;
    }
    return default_limits_;
}

bool RegulatoryChecker::check_order(const orderbook::OrderPtr& order, 
                                    int64_t current_position,
                                    double current_delta) {
    const auto& limits = get_limits(order->get_symbol());
    
    if (!check_order_size_limit(order->get_symbol(), order->get_quantity())) {
        return false;
    }
    
    int64_t position_change = (order->get_side() == core::Side::BUY) ? 
                             order->get_quantity() : -static_cast<int64_t>(order->get_quantity());
    int64_t proposed_position = current_position + position_change;
    
    if (!check_position_limit(order->get_symbol(), proposed_position)) {
        return false;
    }
    
    if (limits.asset_class == core::AssetClass::FUTURES) {
        double delta_change = position_change;
        double proposed_delta = current_delta + delta_change;
        
        if (!check_delta_limit(order->get_symbol(), proposed_delta)) {
            return false;
        }
    }
    
    double notional = core::price_to_double(order->get_price()) * order->get_quantity();
    if (!check_notional_limit(order->get_symbol(), notional)) {
        return false;
    }
    
    return true;
}

bool RegulatoryChecker::check_delta_limit(const std::string& symbol, double proposed_delta) {
    const auto& limits = get_limits(symbol);
    
    if (std::abs(proposed_delta) > limits.max_delta) {
        std::stringstream ss;
        ss << "Delta limit exceeded: " << std::abs(proposed_delta) 
           << " > " << limits.max_delta;
        set_violation(ss.str());
        return false;
    }
    
    return true;
}

bool RegulatoryChecker::check_position_limit(const std::string& symbol, int64_t proposed_position) {
    const auto& limits = get_limits(symbol);
    
    if (std::abs(proposed_position) > limits.max_position) {
        std::stringstream ss;
        ss << "Position limit exceeded: " << std::abs(proposed_position)
           << " > " << limits.max_position;
        set_violation(ss.str());
        return false;
    }
    
    return true;
}

bool RegulatoryChecker::check_order_size_limit(const std::string& symbol, core::Volume order_size) {
    const auto& limits = get_limits(symbol);
    
    if (order_size > static_cast<core::Volume>(limits.max_order_size)) {
        std::stringstream ss;
        ss << "Order size limit exceeded: " << order_size 
           << " > " << limits.max_order_size;
        set_violation(ss.str());
        return false;
    }
    
    return true;
}

bool RegulatoryChecker::check_notional_limit(const std::string& symbol, double notional_value) {
    const auto& limits = get_limits(symbol);
    
    if (std::abs(notional_value) > limits.max_notional_usd) {
        std::stringstream ss;
        ss << "Notional limit exceeded: $" << std::abs(notional_value)
           << " > $" << limits.max_notional_usd;
        set_violation(ss.str());
        return false;
    }
    
    return true;
}

bool RegulatoryChecker::check_price_deviation(const std::string& symbol, 
                                              core::Price proposed_price,
                                              core::Price reference_price) {
    const auto& limits = get_limits(symbol);
    
    if (!limits.circuit_breaker_enabled || reference_price == 0) {
        return true;
    }
    
    double deviation = std::abs(core::price_to_double(proposed_price - reference_price)) / 
                      core::price_to_double(reference_price);
    
    if (deviation > limits.price_deviation_pct) {
        std::stringstream ss;
        ss << "Price deviation exceeded (circuit breaker): " << (deviation * 100)
           << "% > " << (limits.price_deviation_pct * 100) << "%";
        set_violation(ss.str());
        return false;
    }
    
    return true;
}

bool RegulatoryChecker::check_order_rate(const std::string& symbol, int orders_in_last_second) {
    const auto& limits = get_limits(symbol);
    
    if (orders_in_last_second > limits.max_orders_per_second) {
        std::stringstream ss;
        ss << "Order rate limit exceeded: " << orders_in_last_second
           << " > " << limits.max_orders_per_second << " orders/sec";
        set_violation(ss.str());
        return false;
    }
    
    return true;
}

bool RegulatoryChecker::check_cancel_rate(const std::string& symbol, 
                                         int cancels_in_last_second,
                                         int orders_in_last_period) {
    const auto& limits = get_limits(symbol);
    
    if (cancels_in_last_second > limits.max_cancels_per_second) {
        std::stringstream ss;
        ss << "Cancel rate limit exceeded: " << cancels_in_last_second
           << " > " << limits.max_cancels_per_second << " cancels/sec";
        set_violation(ss.str());
        return false;
    }
    
    if (orders_in_last_period > 0) {
        double cancel_ratio = static_cast<double>(cancels_in_last_second) / orders_in_last_period;
        
        if (cancel_ratio > limits.max_cancel_ratio) {
            std::stringstream ss;
            ss << "Cancel ratio exceeded: " << (cancel_ratio * 100)
               << "% > " << (limits.max_cancel_ratio * 100) << "%";
            set_violation(ss.str());
            return false;
        }
    }
    
    return true;
}

} // namespace risk
} // namespace hft
