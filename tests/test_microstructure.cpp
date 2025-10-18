#include <gtest/gtest.h>
#include "../src/signals/microstructure_signals.h"
#include "../src/orderbook/order_book.h"

using namespace hft;

class MicrostructureTest : public ::testing::Test {
protected:
    void SetUp() override {
        book = std::make_shared<orderbook::OrderBook>("TEST");
        micro = std::make_shared<signals::MicrostructureSignals>(10);
    }

    std::shared_ptr<orderbook::OrderBook> book;
    std::shared_ptr<signals::MicrostructureSignals> micro;
};

TEST_F(MicrostructureTest, ImbalanceCalculation) {
    for (int i = 1; i <= 5; i++) {
        book->add_order(std::make_shared<orderbook::Order>(
            i, "TEST", core::Side::BUY, core::OrderType::LIMIT,
            core::double_to_price(100.0 - i), 200
        ));
        
        book->add_order(std::make_shared<orderbook::Order>(
            i + 100, "TEST", core::Side::SELL, core::OrderType::LIMIT,
            core::double_to_price(101.0 + i), 100
        ));
    }
    
    double imbalance = micro->calculate_imbalance(*book);
    
    EXPECT_GT(imbalance, 0.3);
    EXPECT_LT(imbalance, 1.0);
}

TEST_F(MicrostructureTest, PressureCalculation) {
    for (int i = 1; i <= 10; i++) {
        book->add_order(std::make_shared<orderbook::Order>(
            i, "TEST", core::Side::BUY, core::OrderType::LIMIT,
            core::double_to_price(100.0 - i * 0.1), 100 * i
        ));
        
        book->add_order(std::make_shared<orderbook::Order>(
            i + 100, "TEST", core::Side::SELL, core::OrderType::LIMIT,
            core::double_to_price(100.1 + i * 0.1), 50 * i
        ));
    }
    
    double bid_pressure = micro->calculate_bid_pressure(*book);
    double ask_pressure = micro->calculate_ask_pressure(*book);
    double net_pressure = micro->calculate_net_pressure(*book);
    
    EXPECT_GT(bid_pressure, 0);
    EXPECT_GT(ask_pressure, 0);
    EXPECT_GT(net_pressure, 0);
}

TEST_F(MicrostructureTest, DirectionPrediction) {
    for (int i = 1; i <= 10; i++) {
        book->add_order(std::make_shared<orderbook::Order>(
            i, "TEST", core::Side::BUY, core::OrderType::LIMIT,
            core::double_to_price(100.0 - i * 0.1), 500
        ));
        
        book->add_order(std::make_shared<orderbook::Order>(
            i + 100, "TEST", core::Side::SELL, core::OrderType::LIMIT,
            core::double_to_price(100.1 + i * 0.1), 100
        ));
    }
    
    auto direction = micro->predict_direction(*book);
    double confidence = micro->calculate_confidence(*book);
    
    EXPECT_EQ(direction, core::PriceDirection::UP);
    EXPECT_GT(confidence, 0.0);
    EXPECT_LE(confidence, 1.0);
}

TEST_F(MicrostructureTest, VWAPCalculation) {
    book->add_order(std::make_shared<orderbook::Order>(
        1, "TEST", core::Side::BUY, core::OrderType::LIMIT,
        core::double_to_price(100.0), 100
    ));
    
    book->add_order(std::make_shared<orderbook::Order>(
        2, "TEST", core::Side::BUY, core::OrderType::LIMIT,
        core::double_to_price(99.0), 200
    ));
    
    double vwap = micro->calculate_vwap(*book, core::Side::BUY);
    double expected = (100.0 * 100 + 99.0 * 200) / 300.0;
    
    EXPECT_NEAR(vwap, expected, 0.01);
}

TEST_F(MicrostructureTest, JumpDetection) {
    book->add_order(std::make_shared<orderbook::Order>(
        1, "TEST", core::Side::BUY, core::OrderType::LIMIT,
        core::double_to_price(100.0), 1000
    ));
    
    book->add_order(std::make_shared<orderbook::Order>(
        2, "TEST", core::Side::SELL, core::OrderType::LIMIT,
        core::double_to_price(101.0), 10
    ));
    
    bool jump_detected = micro->detect_imminent_jump(*book);
    
    EXPECT_TRUE(jump_detected);
}

TEST_F(MicrostructureTest, FullAnalysis) {
    for (int i = 1; i <= 10; i++) {
        book->add_order(std::make_shared<orderbook::Order>(
            i, "TEST", core::Side::BUY, core::OrderType::LIMIT,
            core::double_to_price(100.0 - i * 0.1), 100
        ));
        
        book->add_order(std::make_shared<orderbook::Order>(
            i + 100, "TEST", core::Side::SELL, core::OrderType::LIMIT,
            core::double_to_price(100.1 + i * 0.1), 100
        ));
    }
    
    auto data = micro->analyze(*book);
    
    EXPECT_GE(data.imbalance, -1.0);
    EXPECT_LE(data.imbalance, 1.0);
    EXPECT_GE(data.bid_pressure, 0.0);
    EXPECT_GE(data.ask_pressure, 0.0);
    EXPECT_GE(data.confidence, 0.0);
    EXPECT_LE(data.confidence, 1.0);
    EXPECT_GT(data.timestamp, 0);
}

