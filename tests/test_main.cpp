#include <gtest/gtest.h>
#include <iostream>

int main(int argc, char **argv) {
    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════════════════╗\n";
    std::cout << "║        HFT PLATFORM TEST SUITE                     ║\n";
    std::cout << "╚════════════════════════════════════════════════════╝\n";
    std::cout << "\n";
    
    ::testing::InitGoogleTest(&argc, argv);
    
    std::cout << "Running all tests...\n";
    std::cout << "Use --gtest_filter to run specific tests\n";
    std::cout << "\nExamples:\n";
    std::cout << "  --gtest_filter=OrderBookTest.*\n";
    std::cout << "  --gtest_filter=*Microstructure*\n";
    std::cout << "  --gtest_filter=CMEScenariosTest.ESNormalTrading\n";
    std::cout << "\n";
    
    int result = RUN_ALL_TESTS();
    
    std::cout << "\n";
    if (result == 0) {
        std::cout << "╔════════════════════════════════════════════════════╗\n";
        std::cout << "║        ✓ ALL TESTS PASSED                         ║\n";
        std::cout << "╚════════════════════════════════════════════════════╝\n";
    } else {
        std::cout << "╔════════════════════════════════════════════════════╗\n";
        std::cout << "║        ✗ SOME TESTS FAILED                        ║\n";
        std::cout << "╚════════════════════════════════════════════════════╝\n";
    }
    std::cout << "\n";
    
    return result;
}
