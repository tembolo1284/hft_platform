#include <gtest/gtest.h>
#include "../src/orderbook/order_book.h"

using namespace hft;

class OrderBookTest : public ::testing::Test {
protected:
    void SetUp() override {
        book = std::make_shared<orderbook::OrderBook>("TEST");
    }

    std::shared_ptr<orderbook::OrderBook> book;
};

TEST_F(OrderBookTest, BasicMatching) {
    auto buy = std::make_shared<orderbook::Order>(
        1, "TEST", core::Side::BUY, core::OrderType::LIMIT,
        core::double_to_price(100.0), 10
    );
    
    auto sell = std::make_shared<orderbook::Order>(
        2, "TEST", core::Side::SELL, core::OrderType::LIMIT,
        core::double_to_price(100.0), 10
    );
    
    book->add_order(buy);
    book->add_order(sell);
    
    EXPECT_EQ(buy->get_status(), core::OrderStatus::FILLED);
    EXPECT_EQ(sell->get_status(), core::OrderStatus::FILLED);
    EXPECT_EQ(book->get_trades().size(), 1);
}

TEST_F(OrderBookTest, PriceTimePriority) {
    auto buy1 = std::make_shared<orderbook::Order>(
        1, "TEST", core::Side::BUY, core::OrderType::LIMIT,
        core::double_to_price(100.0), 5
    );
    
    auto buy2 = std::make_shared<orderbook::Order>(
        2, "TEST", core::Side::BUY, core::OrderType::LIMIT,
        core::double_to_price(101.0), 5
    );
    
    book->add_order(buy1);
    book->add_order(buy2);
    
    EXPECT_EQ(book->get_best_bid(), core::double_to_price(101.0));
}

TEST_F(OrderBookTest, PartialFills) {
    auto buy = std::make_shared<orderbook::Order>(
        1, "TEST", core::Side::BUY, core::OrderType::LIMIT,
        core::double_to_price(100.0), 100
    );
    
    auto sell1 = std::make_shared<orderbook::Order>(
        2, "TEST", core::Side::SELL, core::OrderType::LIMIT,
        core::double_to_price(100.0), 30
    );
    
    auto sell2 = std::make_shared<orderbook::Order>(
        3, "TEST", core::Side::SELL, core::OrderType::LIMIT,
        core::double_to_price(100.0), 40
    );
    
    book->add_order(buy);
    book->add_order(sell1);
    
    EXPECT_EQ(buy->get_status(), core::OrderStatus::PARTIALLY_FILLED);
    EXPECT_EQ(buy->get_filled_quantity(), 30);
    EXPECT_EQ(buy->get_remaining_quantity(), 70);
    
    book->add_order(sell2);
    
    EXPECT_EQ(buy->get_filled_quantity(), 70);
    EXPECT_EQ(buy->get_remaining_quantity(), 30);
}

TEST_F(OrderBookTest, OrderCancellation) {
    auto order = std::make_shared<orderbook::Order>(
        1, "TEST", core::Side::BUY, core::OrderType::LIMIT,
        core::double_to_price(100.0), 10
    );
    
    book->add_order(order);
    EXPECT_EQ(order->get_status(), core::OrderStatus::NEW);
    
    bool cancelled = book->cancel_order(1);
    EXPECT_TRUE(cancelled);
    EXPECT_EQ(order->get_status(), core::OrderStatus::CANCELLED);
}

TEST_F(OrderBookTest, SpreadCalculation) {
    auto buy = std::make_shared<orderbook::Order>(
        1, "TEST", core::Side::BUY, core::OrderType::LIMIT,
        core::double_to_price(99.50), 10
    );
    
    auto sell = std::make_shared<orderbook::Order>(
        2, "TEST", core::Side::SELL, core::OrderType::LIMIT,
        core::double_to_price(100.50), 10
    );
    
    book->add_order(buy);
    book->add_order(sell);
    
    auto spread = book->get_spread();
    EXPECT_DOUBLE_EQ(core::price_to_double(spread), 1.0);
    
    auto mid = book->get_mid_price();
    EXPECT_DOUBLE_EQ(core::price_to_double(mid), 100.0);
}

TEST_F(OrderBookTest, MarketOrder) {
    auto limit_sell = std::make_shared<orderbook::Order>(
        1, "TEST", core::Side::SELL, core::OrderType::LIMIT,
        core::double_to_price(100.0), 50
    );
    book->add_order(limit_sell);
    
    auto market_buy = std::make_shared<orderbook::Order>(
        2, "TEST", core::Side::BUY, core::OrderType::MARKET,
        core::double_to_price(0.0), 30
    );
    book->add_order(market_buy);
    
    EXPECT_EQ(market_buy->get_status(), core::OrderStatus::FILLED);
    EXPECT_EQ(market_buy->get_filled_quantity(), 30);
    EXPECT_EQ(limit_sell->get_filled_quantity(), 30);
}

