#include "../src/signals/microstructure_signals.h"
#include "../src/orderbook/order_book.h"
#include <iostream>
#include <iomanip>

using namespace hft;

void print_separator() {
    std::cout << "\n" << std::string(60, '=') << "\n\n";
}

int main() {
    std::cout << "\n=== MICROSTRUCTURE SIGNALS DEMO ===\n";
    print_separator();
    
    orderbook::OrderBook book("ES");
    signals::MicrostructureSignals micro_signals(10);
    
    std::cout << "Building order book for E-mini S&P 500 (ES)...\n\n";
    
    for (int i = 1; i <= 10; i++) {
        auto bid = std::make_shared<orderbook::Order>(
            i, "ES", core::Side::BUY, core::OrderType::LIMIT,
            core::double_to_price(4000.0 - i * 0.25), 100 + i * 10
        );
        book.add_order(bid);
        
        auto ask = std::make_shared<orderbook::Order>(
            i + 100, "ES", core::Side::SELL, core::OrderType::LIMIT,
            core::double_to_price(4000.25 + i * 0.25), 80 + i * 8
        );
        book.add_order(ask);
    }
    
    std::cout << "ORDER BOOK STATE:\n";
    std::cout << "  Best Bid: $" << std::fixed << std::setprecision(2)
              << core::price_to_double(book.get_best_bid()) << "\n";
    std::cout << "  Best Ask: $" << core::price_to_double(book.get_best_ask()) << "\n";
    std::cout << "  Spread: $" << core::price_to_double(book.get_spread()) << "\n";
    std::cout << "  Mid Price: $" << core::price_to_double(book.get_mid_price()) << "\n";
    std::cout << "  Total Bid Volume: " << book.get_total_bid_volume() << "\n";
    std::cout << "  Total Ask Volume: " << book.get_total_ask_volume() << "\n";
    
    print_separator();
    
    auto signals = micro_signals.analyze(book);
    
    std::cout << "MICROSTRUCTURE ANALYSIS:\n\n";
    
    std::cout << "Order Book Imbalance:\n";
    std::cout << "  Value: " << std::setprecision(4) << signals.imbalance << "\n";
    std::cout << "  Interpretation: ";
    if (signals.imbalance > 0.3) {
        std::cout << "Strong bid pressure (buyers dominant)\n";
    } else if (signals.imbalance < -0.3) {
        std::cout << "Strong ask pressure (sellers dominant)\n";
    } else {
        std::cout << "Balanced book\n";
    }
    std::cout << "\n";
    
    std::cout << "Pressure Analysis:\n";
    std::cout << "  Bid Pressure: " << std::setprecision(2) << signals.bid_pressure << "\n";
    std::cout << "  Ask Pressure: " << signals.ask_pressure << "\n";
    std::cout << "  Net Pressure: " << signals.net_pressure << "\n";
    std::cout << "  (Weighted by distance from mid price)\n\n";
    
    std::cout << "Price Direction Prediction:\n";
    std::cout << "  Direction: ";
    switch (signals.direction) {
        case core::PriceDirection::UP:
            std::cout << "UP ↗\n";
            break;
        case core::PriceDirection::DOWN:
            std::cout << "DOWN ↘\n";
            break;
        case core::PriceDirection::NEUTRAL:
            std::cout << "NEUTRAL →\n";
            break;
    }
    std::cout << "  Confidence: " << std::setprecision(1) << (signals.confidence * 100) << "%\n\n";
    
    std::cout << "Spread Metrics:\n";
    std::cout << "  Absolute Spread: $" << std::setprecision(2) 
              << core::price_to_double(signals.spread) << "\n";
    std::cout << "  Spread Ratio: " << std::setprecision(4) 
              << (signals.spread_ratio * 10000) << " bps\n\n";
    
    double vwap_bid = micro_signals.calculate_vwap(book, core::Side::BUY);
    double vwap_ask = micro_signals.calculate_vwap(book, core::Side::SELL);
    
    std::cout << "VWAP Analysis:\n";
    std::cout << "  Bid VWAP: $" << std::setprecision(2) << vwap_bid << "\n";
    std::cout << "  Ask VWAP: $" << vwap_ask << "\n";
    std::cout << "  VWAP Mid: $" << ((vwap_bid + vwap_ask) / 2.0) << "\n\n";
    
    bool jump_risk = micro_signals.detect_imminent_jump(book);
    std::cout << "Jump Risk Detection:\n";
    std::cout << "  Imminent Jump: " << (jump_risk ? "YES " : "NO ") << "\n";
    if (jump_risk) {
        std::cout << "  Warning: High probability of rapid price movement!\n";
    }
    std::cout << "\n";
    
    double order_flow = micro_signals.calculate_order_flow(book);
    std::cout << "Order Flow:\n";
    std::cout << "  Value: " << std::setprecision(4) << order_flow << "\n";
    std::cout << "  (Based on recent trade aggressor detection)\n\n";
    
    print_separator();
    
    std::cout << "TRADING SIGNAL SUMMARY:\n\n";
    
    if (signals.confidence > 0.6) {
        if (signals.direction == core::PriceDirection::UP) {
            std::cout << "     STRONG BUY SIGNAL\n";
            std::cout << "     Bid imbalance: " << std::setprecision(2) 
                      << (signals.imbalance * 100) << "%\n";
            std::cout << "     Net pressure: +" << signals.net_pressure << "\n";
            std::cout << "     Confidence: " << (signals.confidence * 100) << "%\n";
        } else if (signals.direction == core::PriceDirection::DOWN) {
            std::cout << "     STRONG SELL SIGNAL\n";
            std::cout << "     Ask imbalance: " << std::setprecision(2) 
                      << (-signals.imbalance * 100) << "%\n";
            std::cout << "     Net pressure: " << signals.net_pressure << "\n";
            std::cout << "     Confidence: " << (signals.confidence * 100) << "%\n";
        }
    } else {
        std::cout << "  ⚪ NO CLEAR SIGNAL\n";
        std::cout << "     Market appears balanced\n";
        std::cout << "     Wait for stronger conviction\n";
    }
    
    print_separator();
    
    std::cout << "Demo completed successfully!\n\n";
    
    return 0;
}
