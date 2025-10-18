#include <gtest/gtest.h>
#include "../src/risk/regulatory_limits.h"
#include "../src/signals/microstructure_signals.h"
#include "../src/orderbook/order_book.h"

using namespace hft;

class CMEScenariosTest : public ::testing::Test {
protected:
    void SetUp() override {
        checker = std::make_shared<risk::RegulatoryChecker>();
        book = std::make_shared<orderbook::OrderBook>("ES");
        micro = std::make_shared<signals::MicrostructureSignals>(10);
    }

    std::shared_ptr<risk::RegulatoryChecker> checker;
    std::shared_ptr<orderbook::OrderBook> book;
    std::shared_ptr<signals::MicrostructureSignals> micro;
};

TEST_F(CMEScenariosTest, ESNormalTrading) {
    auto limits = risk::CMELimits::get_es_limits();
    checker->set_limits("ES", limits);
    
    int64_t position = 5000;
    double delta = 5000.0;
    
    auto order = std::make_shared<orderbook::Order>(
        1, "ES", core::Side::BUY, core::OrderType::LIMIT,
        core::double_to_price(4000.0), 500
    );
    
    EXPECT_TRUE(checker->check_order(order, position, delta));
}

TEST_F(CMEScenariosTest, ESVelocityLogic) {
    auto limits = risk::CMELimits::get_es_limits();
    checker->set_limits("ES", limits);
    
    core::Price reference = core::double_to_price(4000.0);
    core::Price spike = core::double_to_price(4220.0);
    
    EXPECT_FALSE(checker->check_price_deviation("ES", spike, reference));
}

TEST_F(CMEScenariosTest, ESRateLimits) {
    auto limits = risk::CMELimits::get_es_limits();
    checker->set_limits("ES", limits);
    
    EXPECT_TRUE(checker->check_order_rate("ES", 1800));
    EXPECT_FALSE(checker->check_order_rate("ES", 2500));
    
    EXPECT_TRUE(checker->check_cancel_rate("ES", 1700, 2000));
    EXPECT_FALSE(checker->check_cancel_rate("ES", 1950, 2000));
}

TEST_F(CMEScenariosTest, CLCrudeLimits) {
    auto limits = risk::CMELimits::get_cl_limits();
    checker->set_limits("CL", limits);
    
    int64_t position = 8000;
    
    auto order = std::make_shared<orderbook::Order>(
        1, "CL", core::Side::BUY, core::OrderType::LIMIT,
        core::double_to_price(75.0), 500
    );
    
    EXPECT_TRUE(checker->check_order(order, position));
    EXPECT_EQ(limits.max_position, 10000);
}

TEST_F(CMEScenariosTest, MicrostructureIntegration) {
    auto limits = risk::CMELimits::get_es_limits();
    checker->set_limits("ES", limits);
    
    for (int i = 1; i <= 10; i++) {
        book->add_order(std::make_shared<orderbook::Order>(
            i, "ES", core::Side::BUY, core::OrderType::LIMIT,
            core::double_to_price(4000.0 - i * 0.25), 100
        ));
        
        book->add_order(std::make_shared<orderbook::Order>(
            i + 100, "ES", core::Side::SELL, core::OrderType::LIMIT,
            core::double_to_price(4000.25 + i * 0.25), 100
        ));
    }
    
    auto signals = micro->analyze(*book);
    
    EXPECT_GE(signals.confidence, 0.0);
    EXPECT_LE(signals.confidence, 1.0);
    
    auto order = std::make_shared<orderbook::Order>(
        1000, "ES", core::Side::BUY, core::OrderType::LIMIT,
        core::double_to_price(4000.25), 100
    );
    
    EXPECT_TRUE(checker->check_order(order, 0));
}

