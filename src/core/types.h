#ifndef HFT_CORE_TYPES_H
#define HFT_CORE_TYPES_H

#include <cstdint>
#include <string>

namespace hft {
namespace core {

using Price = int64_t;
using Volume = uint64_t;
using OrderId = uint64_t;

enum class Side {
    BUY,
    SELL
};

enum class OrderType {
    MARKET,
    LIMIT,
    STOP,
    STOP_LIMIT,
    ICEBERG,
    TWAP,
    VWAP,
    PEG,
    TRAILING_STOP,
    FILL_OR_KILL,
    IMMEDIATE_OR_CANCEL
};

enum class OrderStatus {
    NEW,
    PARTIALLY_FILLED,
    FILLED,
    CANCELLED,
    REJECTED,
    EXPIRED
};

enum class TimeInForce {
    DAY,
    GTC,
    IOC,
    FOK,
    GTD
};

enum class Exchange {
    CME,
    NASDAQ,
    NYSE,
    CBOE
};

enum class AssetClass {
    EQUITY,
    FUTURES,
    OPTIONS,
    FX,
    CRYPTO
};

enum class PriceDirection {
    UP,
    DOWN,
    NEUTRAL
};

inline std::string side_to_string(Side side) {
    return side == Side::BUY ? "BUY" : "SELL";
}

inline std::string order_type_to_string(OrderType type) {
    switch (type) {
        case OrderType::MARKET: return "MARKET";
        case OrderType::LIMIT: return "LIMIT";
        case OrderType::STOP: return "STOP";
        case OrderType::STOP_LIMIT: return "STOP_LIMIT";
        case OrderType::ICEBERG: return "ICEBERG";
        case OrderType::TWAP: return "TWAP";
        case OrderType::VWAP: return "VWAP";
        case OrderType::PEG: return "PEG";
        case OrderType::TRAILING_STOP: return "TRAILING_STOP";
        case OrderType::FILL_OR_KILL: return "FILL_OR_KILL";
        case OrderType::IMMEDIATE_OR_CANCEL: return "IMMEDIATE_OR_CANCEL";
        default: return "UNKNOWN";
    }
}

inline std::string exchange_to_string(Exchange exchange) {
    switch (exchange) {
        case Exchange::CME: return "CME";
        case Exchange::NASDAQ: return "NASDAQ";
        case Exchange::NYSE: return "NYSE";
        case Exchange::CBOE: return "CBOE";
        default: return "UNKNOWN";
    }
}

inline std::string asset_class_to_string(AssetClass asset_class) {
    switch (asset_class) {
        case AssetClass::EQUITY: return "EQUITY";
        case AssetClass::FUTURES: return "FUTURES";
        case AssetClass::OPTIONS: return "OPTIONS";
        case AssetClass::FX: return "FX";
        case AssetClass::CRYPTO: return "CRYPTO";
        default: return "UNKNOWN";
    }
}

inline Price double_to_price(double price) {
    return static_cast<Price>(price * 10000);
}

inline double price_to_double(Price price) {
    return static_cast<double>(price) / 10000.0;
}

} // namespace core
} // namespace hft

#endif // HFT_CORE_TYPES_H
