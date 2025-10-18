#include "tick_parser.h"
#include <arpa/inet.h>
#include <algorithm>

namespace hft {
namespace market_data {

// ============================================================================
// SymbolMap Implementation
// ============================================================================

SymbolMap::SymbolMap() {}

void SymbolMap::add_symbol(uint32_t id, const std::string& symbol) {
    id_to_symbol_[id] = symbol;
    symbol_to_id_[symbol] = id;
}

std::string SymbolMap::get_symbol(uint32_t id) const {
    auto it = id_to_symbol_.find(id);
    return it != id_to_symbol_.end() ? it->second : "";
}

uint32_t SymbolMap::get_id(const std::string& symbol) const {
    auto it = symbol_to_id_.find(symbol);
    return it != symbol_to_id_.end() ? it->second : 0;
}

bool SymbolMap::has_symbol(uint32_t id) const {
    return id_to_symbol_.find(id) != id_to_symbol_.end();
}

bool SymbolMap::has_id(const std::string& symbol) const {
    return symbol_to_id_.find(symbol) != symbol_to_id_.end();
}

// ============================================================================
// TickParser Implementation
// ============================================================================

TickParser::TickParser(const SymbolMap& symbol_map)
    : symbol_map_(symbol_map)
{}

bool TickParser::parse_tick(const uint8_t* data, size_t length, Tick& tick) {
    if (length < sizeof(BinaryTick)) {
        return false;
    }
    
    const BinaryTick* binary_tick = reinterpret_cast<const BinaryTick*>(data);
    
    // Convert from network byte order
    uint32_t symbol_id = ntoh_uint32(binary_tick->symbol_id);
    
    // Lookup symbol
    tick.symbol = symbol_map_.get_symbol(symbol_id);
    if (tick.symbol.empty()) {
        return false; // Unknown symbol
    }
    
    tick.timestamp = ntoh_uint64(binary_tick->timestamp);
    tick.price = ntoh_int64(binary_tick->price);
    tick.volume = ntoh_uint32(binary_tick->volume);
    tick.exchange_id = ntoh_uint16(binary_tick->exchange_id);
    
    // Parse side
    uint8_t side_val = binary_tick->side;
    if (side_val == 0) {
        tick.side = core::Side::BUY;
        tick.is_trade = false;
    } else if (side_val == 1) {
        tick.side = core::Side::SELL;
        tick.is_trade = false;
    } else {
        tick.is_trade = true;
        // For trades, side indicates aggressor
        tick.side = (binary_tick->flags & 0x01) ? core::Side::BUY : core::Side::SELL;
    }
    
    return true;
}

std::vector<Tick> TickParser::parse_buffer(const uint8_t* data, size_t length) {
    std::vector<Tick> ticks;
    
    size_t offset = 0;
    size_t tick_size = sizeof(BinaryTick);
    
    while (offset + tick_size <= length) {
        Tick tick;
        if (parse_tick(data + offset, length - offset, tick)) {
            ticks.push_back(tick);
        }
        offset += tick_size;
    }
    
    return ticks;
}

bool TickParser::serialize_tick(const Tick& tick, uint8_t* buffer, size_t buffer_size) {
    if (buffer_size < sizeof(BinaryTick)) {
        return false;
    }
    
    uint32_t symbol_id = symbol_map_.get_id(tick.symbol);
    if (symbol_id == 0) {
        return false; // Unknown symbol
    }
    
    BinaryTick* binary_tick = reinterpret_cast<BinaryTick*>(buffer);
    
    binary_tick->symbol_id = hton_uint32(symbol_id);
    binary_tick->timestamp = hton_uint64(tick.timestamp);
    binary_tick->price = hton_int64(tick.price);
    binary_tick->volume = hton_uint32(tick.volume);
    binary_tick->exchange_id = hton_uint16(tick.exchange_id);
    
    if (tick.is_trade) {
        binary_tick->side = 2;
        binary_tick->flags = (tick.side == core::Side::BUY) ? 0x01 : 0x00;
    } else {
        binary_tick->side = (tick.side == core::Side::BUY) ? 0 : 1;
        binary_tick->flags = 0;
    }
    
    return true;
}

// Network byte order conversion (host to network and network to host)
uint32_t TickParser::ntoh_uint32(uint32_t val) const {
    return ntohl(val);
}

uint64_t TickParser::ntoh_uint64(uint64_t val) const {
    return be64toh(val);
}

int64_t TickParser::ntoh_int64(int64_t val) const {
    return static_cast<int64_t>(be64toh(static_cast<uint64_t>(val)));
}

uint16_t TickParser::ntoh_uint16(uint16_t val) const {
    return ntohs(val);
}

uint32_t TickParser::hton_uint32(uint32_t val) const {
    return htonl(val);
}

uint64_t TickParser::hton_uint64(uint64_t val) const {
    return htobe64(val);
}

int64_t TickParser::hton_int64(int64_t val) const {
    return static_cast<int64_t>(htobe64(static_cast<uint64_t>(val)));
}

uint16_t TickParser::hton_uint16(uint16_t val) const {
    return htons(val);
}

} // namespace market_data
} // namespace hft
