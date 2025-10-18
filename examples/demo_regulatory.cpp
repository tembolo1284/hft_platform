#include "../src/risk/regulatory_limits.h"
#include "../src/orderbook/order.h"
#include <iostream>
#include <iomanip>

using namespace hft;

void print_separator() {
    std::cout << "\n" << std::string(60, '=') << "\n\n";
}

void test_cme_es_limits() {
    std::cout << "=== CME E-MINI S&P 500 (ES) LIMITS ===\n\n";
    
    risk::RegulatoryChecker checker;
    auto limits = risk::CMELimits::get_es_limits();
    checker.set_limits("ES", limits);
    
    std::cout << "CME ES Regulatory Limits:\n";
    std::cout << "  Max Position: " << limits.max_position << " contracts\n";
    std::cout << "  Max Order Size: " << limits.max_order_size << " contracts\n";
    std::cout << "  Max Delta: " << limits.max_delta << "\n";
    std::cout << "  Max Orders/sec: " << limits.max_orders_per_second << "\n";
    std::cout << "  Circuit Breaker: ±" << (limits.price_deviation_pct * 100) << "%\n\n";
    
    int64_t position = 15000;
    double delta = 15000.0;
    
    std::cout << "Current Position: " << position << " contracts\n";
    std::cout << "Current Delta: " << delta << "\n\n";
    
    std::cout << "Test 1: Normal order (within limits)\n";
    auto order1 = std::make_shared<orderbook::Order>(
        1, "ES", core::Side::BUY, core::OrderType::LIMIT,
        core::double_to_price(4000.0), 100
    );
    
    if (checker.check_order(order1, position, delta)) {
        std::cout << "  Result:   APPROVED\n";
        std::cout << "  New position would be: " << (position + 100) << "\n";
    } else {
        std::cout << "  Result:   REJECTED\n";
        std::cout << "  Reason: " << checker.get_last_violation() << "\n";
    }
    
    std::cout << "\nTest 2: Large order (exceeds position limit)\n";
    auto order2 = std::make_shared<orderbook::Order>(
        2, "ES", core::Side::BUY, core::OrderType::LIMIT,
        core::double_to_price(4000.0), 6000
    );
    
    if (checker.check_order(order2, position, delta)) {
        std::cout << "  Result:   APPROVED\n";
    } else {
        std::cout << "  Result:   REJECTED\n";
        std::cout << "  Reason: " << checker.get_last_violation() << "\n";
        std::cout << "  Position would exceed accountability level of 20,000\n";
    }
    
    std::cout << "\nTest 3: Circuit breaker (velocity logic)\n";
    core::Price reference = core::double_to_price(4000.0);
    core::Price proposed = core::double_to_price(4220.0);
    
    if (checker.check_price_deviation("ES", proposed, reference)) {
        std::cout << "  Result: ✓ APPROVED\n";
    } else {
        std::cout << "  Result: ✗ TRADING HALTED\n";
        std::cout << "  Reason: " << checker.get_last_violation() << "\n";
        std::cout << "  Price moved: " 
                  << ((core::price_to_double(proposed - reference) / core::price_to_double(reference)) * 100)
                  << "%\n";
    }
}

void test_nasdaq_limits() {
    std::cout << "\n=== NASDAQ LARGE CAP LIMITS ===\n\n";
    
    risk::RegulatoryChecker checker;
    auto limits = risk::NASDAQLimits::get_large_cap_limits();
    checker.set_limits("AAPL", limits);
    
    std::cout << "NASDAQ Large Cap Limits (e.g., AAPL):\n";
    std::cout << "  Max Position: " << limits.max_position << " shares\n";
    std::cout << "  Max Order Size: " << limits.max_order_size << " shares\n";
    std::cout << "  Max Orders/sec: " << limits.max_orders_per_second << "\n";
    std::cout << "  LULD Threshold: ±5%\n\n";
    
    int64_t position = 500000;
    
    std::cout << "Current Position: " << position << " shares\n\n";
    
    std::cout << "Test 1: Normal equity order\n";
    auto order1 = std::make_shared<orderbook::Order>(
        1, "AAPL", core::Side::BUY, core::OrderType::LIMIT,
        core::double_to_price(150.0), 10000
    );
    
    if (checker.check_order(order1, position)) {
        std::cout << "  Result:   APPROVED\n";
    } else {
        std::cout << "  Result:   REJECTED\n";
        std::cout << "  Reason: " << checker.get_last_violation() << "\n";
    }
    
    std::cout << "\nTest 2: LULD band violation\n";
    core::Price reference = core::double_to_price(150.0);
    core::Price proposed = core::double_to_price(158.5);
    
    if (checker.check_price_deviation("AAPL", proposed, reference)) {
        std::cout << "  Result: ✓ APPROVED\n";
    } else {
        std::cout << "  Result:   TRADING PAUSE\n";
        std::cout << "  Reason: " << checker.get_last_violation() << "\n";
        std::cout << "  LULD bands exceeded, 5-minute trading pause\n";
    }
    
    std::cout << "\nTest 3: Rate limiting\n";
    int orders_last_second = 600;
    
    if (checker.check_order_rate("AAPL", orders_last_second)) {
        std::cout << "  Result:   APPROVED\n";
    } else {
        std::cout << "  Result:   THROTTLED\n";
        std::cout << "  Reason: " << checker.get_last_violation() << "\n";
    }
}

void test_cancel_ratio() {
    std::cout << "\n=== CANCEL RATIO MONITORING ===\n\n";
    
    risk::RegulatoryChecker checker;
    auto limits = risk::CMELimits::get_es_limits();
    checker.set_limits("ES", limits);
    
    std::cout << "Monitoring order-to-cancel ratio for ES...\n";
    std::cout << "Max cancel ratio: " << (limits.max_cancel_ratio * 100) << "%\n\n";
    
    std::cout << "Scenario 1: Acceptable cancel ratio\n";
    int orders_sent = 1000;
    int cancels = 800;
    
    if (checker.check_cancel_rate("ES", cancels, orders_sent)) {
        std::cout << "  Cancels: " << cancels << "\n";
        std::cout << "  Orders: " << orders_sent << "\n";
        std::cout << "  Ratio: " << ((double)cancels / orders_sent * 100) << "%\n";
        std::cout << "  Result:   APPROVED\n";
    } else {
        std::cout << "  Result:   REJECTED\n";
        std::cout << "  Reason: " << checker.get_last_violation() << "\n";
    }
    
    std::cout << "\nScenario 2: Excessive cancel ratio\n";
    cancels = 980;
    
    if (checker.check_cancel_rate("ES", cancels, orders_sent)) {
        std::cout << "  Result: ✓ APPROVED\n";
    } else {
        std::cout << "  Cancels: " << cancels << "\n";
        std::cout << "  Orders: " << orders_sent << "\n";
        std::cout << "  Ratio: " << ((double)cancels / orders_sent * 100) << "%\n";
        std::cout << "  Result:   REJECTED\n";
        std::cout << "  Reason: " << checker.get_last_violation() << "\n";
        std::cout << "  Action: Throttle or block further orders\n";
    }
}

int main() {
    std::cout << "\n=== REGULATORY LIMITS DEMO ===\n";
    print_separator();
    
    test_cme_es_limits();
    print_separator();
    
    test_nasdaq_limits();
    print_separator();
    
    test_cancel_ratio();
    print_separator();
    
    std::cout << "Demo completed successfully!\n\n";
    
    return 0;
}
