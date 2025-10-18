cmake_minimum_required(VERSION 3.15)
project(hft-platform)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -fsanitize=address -fsanitize=undefined")
    set(CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} -fsanitize=address -fsanitize=undefined")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=native")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror")

include_directories(${CMAKE_SOURCE_DIR}/src)

# Core library
add_library(hft_core
    src/core/types.cpp
    src/core/timestamp.cpp
)

# Order book library
add_library(hft_orderbook
    src/orderbook/order.cpp
    src/orderbook/order_book.cpp
)
target_link_libraries(hft_orderbook hft_core)

# Signals library
add_library(hft_signals
    src/signals/microstructure_signals.cpp
)
target_link_libraries(hft_signals hft_orderbook)

# Risk library
add_library(hft_risk
    src/risk/regulatory_limits.cpp
    src/risk/risk_manager.cpp
)
target_link_libraries(hft_risk hft_orderbook)

# Market data library
add_library(hft_market_data
    src/market_data/market_data.cpp
)
target_link_libraries(hft_market_data hft_core)

# Server library
add_library(hft_server
    src/server/trading_server.cpp
)
target_link_libraries(hft_server hft_orderbook hft_risk hft_market_data hft_signals)

# Find Google Test
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

enable_testing()

# Single unified test executable with all tests
add_executable(run_tests
    tests/test_main.cpp
    tests/test_orderbook.cpp
    tests/test_microstructure.cpp
    tests/test_regulatory_limits.cpp
    tests/test_cme_scenarios.cpp
    tests/test_nasdaq_scenarios.cpp
)
target_link_libraries(run_tests 
    hft_server
    hft_risk 
    hft_signals 
    hft_orderbook 
    hft_market_data
    hft_core
    ${GTEST_BOTH_LIBRARIES} 
    pthread
)

# Add the unified test to CTest
add_test(NAME AllTests COMMAND run_tests)

# Demo executables
add_executable(demo_microstructure examples/demo_microstructure.cpp)
target_link_libraries(demo_microstructure hft_signals hft_orderbook)

add_executable(demo_regulatory examples/demo_regulatory.cpp)
target_link_libraries(demo_regulatory hft_risk hft_orderbook)

add_executable(demo_full_system examples/demo_full_system.cpp)
target_link_libraries(demo_full_system hft_server)
