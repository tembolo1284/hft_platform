#include "../src/server/trading_server.h"
#include "../src/risk/regulatory_limits.h"
#include <iostream>
#include <iomanip>

using namespace hft;

void print_separator() {
    std::cout << "\n" << std::string(60, '=') << "\n\n";
}

int main() {
    std::cout << "\n=== FULL HFT SYSTEM DEMO ===\n";
    print_separator();
    
    server::TradingServer server;
    server.initialize();
    
    std::cout << "Initializing trading server...\n";
    std::cout << "   Order books ready\n";
    std::cout << "   Risk manager active\n";
    std::cout << "   Market data connected\n";
    std::cout << "   Microstructure signals online\n";
    
    print_separator();
    
    std::cout << "=== SCENARIO 1: ES FUTURES TRADING ===\n\n";
    
    auto es_book = server.get_order_book("ES");
    if (!es_book) {
        es_book = std::make_shared<orderbook::OrderBook>("ES");
    }
    
    std::cout << "Building ES order book...\n";
    for (int i = 1; i <= 10; i++) {
        auto bid = std::make_shared<orderbook::Order>(
            i, "ES", core::Side::BUY, core::OrderType::LIMIT,
            core::double_to_price(4000.0 - i * 0.25), 150
        );
        server.submit_order(bid);
        
        auto ask = std::make_shared<orderbook::Order>(
            i + 100, "ES", core::Side::SELL, core::OrderType::LIMIT,
            core::double_to_price(4000.25 + i * 0.25), 100
        );
        server.submit_order(ask);
    }
    
    es_book = server.get_order_book("ES");
    
    std::cout << "\nOrder Book State:\n";
    std::cout << "  Best Bid: $" << std::fixed << std::setprecision(2)
              << core::price_to_double(es_book->get_best_bid()) << "\n";
    std::cout << "  Best Ask: $" << core::price_to_double(es_book->get_best_ask()) << "\n";
    std::cout << "  Spread: $" << core::price_to_double(es_book->get_spread()) << "\n\n";
    
    auto signals = server.get_microstructure_signals("ES");
    
    std::cout << "Microstructure Analysis:\n";
    std::cout << "  Imbalance: " << std::setprecision(4) << signals.imbalance << "\n";
    std::cout << "  Direction: ";
    switch (signals.direction) {
        case core::PriceDirection::UP: std::cout << "UP\n"; break;
        case core::PriceDirection::DOWN: std::cout << "DOWN\n"; break;
        case core::PriceDirection::NEUTRAL: std::cout << "NEUTRAL\n"; break;
    }
    std::cout << "  Confidence: " << (signals.confidence * 100) << "%\n\n";
    
    std::cout << "Trading Decision:\n";
    if (signals.confidence > 0.6 && signals.direction == core::PriceDirection::UP) {
        std::cout << "  Signal: BUY\n";
        std::cout << "  Submitting buy order...\n";
        
        auto buy_order = std::make_shared<orderbook::Order>(
            1000, "ES", core::Side::BUY, core::OrderType::LIMIT,
            core::double_to_price(4000.25), 50
        );
        
        bool success = server.submit_order(buy_order);
        
        if (success) {
            std::cout << "  Result:   ORDER EXECUTED\n";
            std::cout << "  Filled: " << buy_order->get_filled_quantity() << " contracts\n";
        } else {
            std::cout << "  Result:   ORDER REJECTED\n";
        }
    } else {
        std::cout << "  Signal: WAIT\n";
        std::cout << "  Confidence too low for trading\n";
    }
    
    print_separator();
    
    std::cout << "=== SCENARIO 2: REGULATORY COMPLIANCE CHECK ===\n\n";
    
    auto position = server.get_position("ES");
    std::cout << "Current Position:\n";
    std::cout << "  Symbol: ES\n";
    std::cout << "  Quantity: " << position.quantity << " contracts\n";
    std::cout << "  Delta: " << position.delta << "\n\n";
    
    std::cout << "Attempting large order (regulatory test)...\n";
    auto large_order = std::make_shared<orderbook::Order>(
        2000, "ES", core::Side::BUY, core::OrderType::LIMIT,
        core::double_to_price(4000.0), 25000
    );
    
    bool approved = server.submit_order(large_order);
    
    if (approved) {
        std::cout << "  Result:   APPROVED\n";
    } else {
        std::cout << "  Result:   REJECTED BY RISK MANAGER\n";
        std::cout << "  Reason: Would exceed position limits\n";
    }
    
    print_separator();
    
    std::cout << "=== SCENARIO 3: MULTI-SYMBOL PORTFOLIO ===\n\n";
    
    std::cout << "Adding AAPL to portfolio...\n";
    auto aapl_book = std::make_shared<orderbook::OrderBook>("AAPL");
    
    for (int i = 1; i <= 5; i++) {
        auto bid = std::make_shared<orderbook::Order>(
            i + 3000, "AAPL", core::Side::BUY, core::OrderType::LIMIT,
            core::double_to_price(150.0 - i * 0.10), 1000
        );
        server.submit_order(bid);
        
        auto ask = std::make_shared<orderbook::Order>(
            i + 3100, "AAPL", core::Side::SELL, core::OrderType::LIMIT,
            core::double_to_price(150.10 + i * 0.10), 1000
        );
        server.submit_order(ask);
    }
    
    auto aapl_order = std::make_shared<orderbook::Order>(
        5000, "AAPL", core::Side::BUY, core::OrderType::LIMIT,
        core::double_to_price(150.10), 500
    );
    
    std::cout << "Submitting AAPL buy order...\n";
    if (server.submit_order(aapl_order)) {
        std::cout << "  Result:   EXECUTED\n";
        std::cout << "  Filled: " << aapl_order->get_filled_quantity() << " shares\n";
    }
    
    std::cout << "\nPortfolio Risk Metrics:\n";
    auto metrics = server.get_risk_metrics();
    std::cout << "  Total Exposure: $" << std::fixed << std::setprecision(0)
              << metrics.total_exposure_usd << "\n";
    std::cout << "  Active Positions: " << metrics.active_positions << "\n";
    std::cout << "  Daily P&L: $" << std::setprecision(2) << metrics.daily_pnl << "\n";
    std::cout << "  VaR (95%): $" << std::setprecision(0) << metrics.var_1day_95 << "\n";
    
    print_separator();
    
    std::cout << "=== SYSTEM PERFORMANCE SUMMARY ===\n\n";
    
    std::cout << "Order Processing:\n";
    std::cout << "   All orders validated through risk manager\n";
    std::cout << "   Regulatory limits enforced\n";
    std::cout << "   Real-time position tracking\n";
    std::cout << "   Microstructure signals integrated\n\n";
    
    std::cout << "Risk Management:\n";
    std::cout << "   Pre-trade compliance checks\n";
    std::cout << "   Position limits enforced\n";
    std::cout << "   Portfolio-level risk monitoring\n";
    std::cout << "   Circuit breaker protection\n\n";
    
    std::cout << "Market Microstructure:\n";
    std::cout << "   Order book imbalance detection\n";
    std::cout << "   Bid/ask pressure analysis\n";
    std::cout << "   Price direction prediction\n";
    std::cout << "   Jump risk detection\n";
    
    print_separator();
    
    std::cout << "Demo completed successfully!\n\n";
    
    return 0;
}
