#include <gtest/gtest.h>
#include "../src/risk/regulatory_limits.h"
#include "../src/signals/microstructure_signals.h"
#include "../src/orderbook/order_book.h"

using namespace hft;

class NASDAQScenariosTest : public ::testing::Test {
protected:
    void SetUp() override {
        checker = std::make_shared<risk::RegulatoryChecker>();
        book = std::make_shared<orderbook::OrderBook>("AAPL");
        micro = std::make_shared<signals::MicrostructureSignals>(10);
    }

    std::shared_ptr<risk::RegulatoryChecker> checker;
    std::shared_ptr<orderbook::OrderBook> book;
    std::shared_ptr<signals::MicrostructureSignals> micro;
};

TEST_F(NASDAQScenariosTest, LargeCapTrading) {
    auto limits = risk::NASDAQLimits::get_large_cap_limits();
    checker->set_limits("AAPL", limits);
    
    int64_t position = 500000;
    
    auto order = std::make_shared<orderbook::Order>(
        1, "AAPL", core::Side::BUY, core::OrderType::LIMIT,
        core::double_to_price(150.0), 10000
    );
    
    EXPECT_TRUE(checker->check_order(order, position));
}

TEST_F(NASDAQScenariosTest, LULDBands) {
    auto limits = risk::NASDAQLimits::get_large_cap_limits();
    checker->set_limits("AAPL", limits);
    
    core::Price reference = core::double_to_price(150.0);
    core::Price within_band = core::double_to_price(157.0);
    core::Price outside_band = core::double_to_price(158.5);
    
    EXPECT_TRUE(checker->check_price_deviation("AAPL", within_band, reference));
    EXPECT_FALSE(checker->check_price_deviation("AAPL", outside_band, reference));
}

TEST_F(NASDAQScenariosTest, SmallCapLimits) {
    auto limits = risk::NASDAQLimits::get_small_cap_limits();
    checker->set_limits("SMCAP", limits);
    
    EXPECT_EQ(limits.max_position, 500000);
    EXPECT_DOUBLE_EQ(limits.price_deviation_pct, 0.10);
}

TEST_F(NASDAQScenariosTest, RateLimits) {
    auto limits = risk::NASDAQLimits::get_large_cap_limits();
    checker->set_limits("AAPL", limits);
    
    EXPECT_TRUE(checker->check_order_rate("AAPL", 400));
    EXPECT_FALSE(checker->check_order_rate("AAPL", 600));
}

TEST_F(NASDAQScenariosTest, ETFTrading) {
    auto limits = risk::NASDAQLimits::get_etf_limits("SPY");
    checker->set_limits("SPY", limits);
    
    EXPECT_EQ(limits.max_position, 2000000);
    
    auto order = std::make_shared<orderbook::Order>(
        1, "SPY", core::Side::BUY, core::OrderType::LIMIT,
        core::double_to_price(450.0), 50000
    );
    
    EXPECT_TRUE(checker->check_order(order, 1000000));
}

TEST_F(NASDAQScenariosTest, MicrostructureLULDIntegration) {
    auto limits = risk::NASDAQLimits::get_large_cap_limits();
    checker->set_limits("AAPL", limits);
    
    for (int i = 1; i <= 10; i++) {
        book->add_order(std::make_shared<orderbook::Order>(
            i, "AAPL", core::Side::BUY, core::OrderType::LIMIT,
            core::double_to_price(150.0 - i * 0.05), 1000
        ));
        
        book->add_order(std::make_shared<orderbook::Order>(
            i + 100, "AAPL", core::Side::SELL, core::OrderType::LIMIT,
            core::double_to_price(150.05 + i * 0.05), 1000
        ));
    }
    
    auto signals = micro->analyze(*book);
    
    EXPECT_GE(signals.confidence, 0.0);
    EXPECT_LE(signals.confidence, 1.0);
    
    core::Price reference = core::double_to_price(150.0);
    core::Price proposed = book->get_best_ask();
    
    EXPECT_TRUE(checker->check_price_deviation("AAPL", proposed, reference));
}

