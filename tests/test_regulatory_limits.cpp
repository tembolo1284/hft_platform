#include <gtest/gtest.h>
#include "../src/risk/regulatory_limits.h"
#include "../src/orderbook/order.h"

using namespace hft;

class RegulatoryLimitsTest : public ::testing::Test {
protected:
    void SetUp() override {
        checker = std::make_shared<risk::RegulatoryChecker>();
    }

    std::shared_ptr<risk::RegulatoryChecker> checker;
};

TEST_F(RegulatoryLimitsTest, PositionLimits) {
    auto limits = risk::CMELimits::get_es_limits();
    checker->set_limits("ES", limits);
    
    int64_t current_position = 15000;
    
    auto small_order = std::make_shared<orderbook::Order>(
        1, "ES", core::Side::BUY, core::OrderType::LIMIT,
        core::double_to_price(4000.0), 100
    );
    
    EXPECT_TRUE(checker->check_order(small_order, current_position));
    
    auto large_order = std::make_shared<orderbook::Order>(
        2, "ES", core::Side::BUY, core::OrderType::LIMIT,
        core::double_to_price(4000.0), 10000
    );
    
    EXPECT_FALSE(checker->check_order(large_order, current_position));
}

TEST_F(RegulatoryLimitsTest, OrderSizeLimits) {
    auto limits = risk::CMELimits::get_es_limits();
    checker->set_limits("ES", limits);
    
    EXPECT_TRUE(checker->check_order_size_limit("ES", 400));
    EXPECT_FALSE(checker->check_order_size_limit("ES", 1000));
}

TEST_F(RegulatoryLimitsTest, DeltaLimits) {
    auto limits = risk::CMELimits::get_es_limits();
    checker->set_limits("ES", limits);
    
    EXPECT_TRUE(checker->check_delta_limit("ES", 30000.0));
    EXPECT_FALSE(checker->check_delta_limit("ES", 50000.0));
}

TEST_F(RegulatoryLimitsTest, CircuitBreaker) {
    auto limits = risk::CMELimits::get_es_limits();
    checker->set_limits("ES", limits);
    
    core::Price reference = core::double_to_price(4000.0);
    core::Price acceptable = core::double_to_price(4100.0);
    core::Price unacceptable = core::double_to_price(4300.0);
    
    EXPECT_TRUE(checker->check_price_deviation("ES", acceptable, reference));
    EXPECT_FALSE(checker->check_price_deviation("ES", unacceptable, reference));
}

TEST_F(RegulatoryLimitsTest, RateLimits) {
    auto limits = risk::CMELimits::get_es_limits();
    checker->set_limits("ES", limits);
    
    EXPECT_TRUE(checker->check_order_rate("ES", 1500));
    EXPECT_FALSE(checker->check_order_rate("ES", 2500));
}

TEST_F(RegulatoryLimitsTest, CancelRatio) {
    auto limits = risk::CMELimits::get_es_limits();
    checker->set_limits("ES", limits);
    
    EXPECT_TRUE(checker->check_cancel_rate("ES", 800, 1000));
    EXPECT_FALSE(checker->check_cancel_rate("ES", 980, 1000));
}

TEST_F(RegulatoryLimitsTest, NASDAQLULDBands) {
    auto limits = risk::NASDAQLimits::get_large_cap_limits();
    checker->set_limits("AAPL", limits);
    
    core::Price reference = core::double_to_price(150.0);
    core::Price within_band = core::double_to_price(157.0);
    core::Price outside_band = core::double_to_price(158.5);
    
    EXPECT_TRUE(checker->check_price_deviation("AAPL", within_band, reference));
    EXPECT_FALSE(checker->check_price_deviation("AAPL", outside_band, reference));
}

TEST_F(RegulatoryLimitsTest, CMEAccountabilityLevels) {
    EXPECT_EQ(risk::CMELimits::get_accountability_level("ES"), 20000);
    EXPECT_EQ(risk::CMELimits::get_accountability_level("GE"), 100000);
    EXPECT_EQ(risk::CMELimits::get_accountability_level("CL"), 10000);
}
