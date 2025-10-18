#ifndef HFT_MARKET_DATA_TICK_PARSER_H
#define HFT_MARKET_DATA_TICK_PARSER_H

#include "../core/types.h"
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace hft {
namespace market_data {

// Binary tick data structure (packed, network byte order)
#pragma pack(push, 1)
struct BinaryTick {
    uint32_t symbol_id;          // 4 bytes - numeric symbol ID
    uint64_t timestamp;          // 8 bytes - microseconds since epoch
    int64_t  price;              // 8 bytes - price in basis points
    uint32_t volume;             // 4 bytes - volume
    uint8_t  side;               // 1 byte  - 0=bid, 1=ask, 2=trade
    uint8_t  flags;              // 1 byte  - various flags
    uint16_t exchange_id;        // 2 bytes - exchange identifier
    // Total: 28 bytes per tick
};
#pragma pack(pop)

// Parsed tick data (native representation)
struct Tick {
    std::string symbol;
    core::Timestamp timestamp;
    core::Price price;
    core::Volume volume;
    core::Side side;
    bool is_trade;
    uint16_t exchange_id;
    
    Tick() 
        : timestamp(0)
        , price(0)
        , volume(0)
        , side(core::Side::BUY)
        , is_trade(false)
        , exchange_id(0)
    {}
};

// Symbol ID mapping
class SymbolMap {
public:
    SymbolMap();
    
    void add_symbol(uint32_t id, const std::string& symbol);
    std::string get_symbol(uint32_t id) const;
    uint32_t get_id(const std::string& symbol) const;
    
    bool has_symbol(uint32_t id) const;
    bool has_id(const std::string& symbol) const;
    
private:
    std::map<uint32_t, std::string> id_to_symbol_;
    std::map<std::string, uint32_t> symbol_to_id_;
};

// Binary tick parser
class TickParser {
public:
    TickParser(const SymbolMap& symbol_map);
    
    // Parse single tick from binary data
    bool parse_tick(const uint8_t* data, size_t length, Tick& tick);
    
    // Parse multiple ticks from buffer
    std::vector<Tick> parse_buffer(const uint8_t* data, size_t length);
    
    // Serialize tick to binary format
    bool serialize_tick(const Tick& tick, uint8_t* buffer, size_t buffer_size);
    
    // Get bytes per tick
    static constexpr size_t bytes_per_tick() { return sizeof(BinaryTick); }
    
private:
    const SymbolMap& symbol_map_;
    
    // Network byte order conversion helpers
    uint32_t ntoh_uint32(uint32_t val) const;
    uint64_t ntoh_uint64(uint64_t val) const;
    int64_t  ntoh_int64(int64_t val) const;
    uint16_t ntoh_uint16(uint16_t val) const;
    
    uint32_t hton_uint32(uint32_t val) const;
    uint64_t hton_uint64(uint64_t val) const;
    int64_t  hton_int64(int64_t val) const;
    uint16_t hton_uint16(uint16_t val) const;
};

} // namespace market_data
} // namespace hft

#endif // HFT_MARKET_DATA_TICK_PARSER_H
