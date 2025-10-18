#!/bin/bash

# HFT Platform Build Script
# Supports multiple compilers, build types, and execution modes
# Uses Ninja by default (falls back to Make if not available)

set -e  # Exit on error
set -o pipefail  # Catch errors in pipes

# Default values
BUILD_TYPE="Release"
COMPILER="gcc"
RUN_TESTS=false
RUN_DEMOS=false
CLEAN_BUILD=false
VERBOSE=false
BUILD_DIR="build"
TEST_FILTER=""
USE_NINJA=true
GENERATOR=""

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Check for required dependencies
check_dependencies() {
    local missing_deps=()
    local optional_missing=()
    
    # Check for CMake
    if ! command -v cmake &> /dev/null; then
        missing_deps+=("cmake")
    fi
    
    # Check for compiler
    if [ "$COMPILER" = "gcc" ] || [ "$COMPILER" = "g++" ]; then
        if ! command -v g++ &> /dev/null; then
            missing_deps+=("g++")
        fi
    elif [ "$COMPILER" = "clang" ] || [ "$COMPILER" = "clang++" ]; then
        if ! command -v clang++ &> /dev/null; then
            missing_deps+=("clang++")
        fi
    fi
    
    # Check for Google Test
    if [ ! -d "/usr/src/googletest" ] && [ ! -f "/usr/lib/libgtest.a" ] && [ ! -f "/usr/local/lib/libgtest.a" ]; then
        missing_deps+=("libgtest-dev")
    fi
    
    # Check for Ninja (optional but recommended)
    if ! command -v ninja &> /dev/null; then
        optional_missing+=("ninja-build")
        USE_NINJA=false
    fi
    
    # Check for Make (fallback)
    if ! command -v make &> /dev/null && [ "$USE_NINJA" = false ]; then
        missing_deps+=("make")
    fi
    
    # If critical dependencies are missing, show installation instructions
    if [ ${#missing_deps[@]} -gt 0 ]; then
        echo -e "${RED}===============================================${NC}"
        echo -e "${RED}     Missing Required Dependencies${NC}"
        echo -e "${RED}===============================================${NC}"
        echo ""
        echo -e "${YELLOW}The following dependencies are missing:${NC}"
        for dep in "${missing_deps[@]}"; do
            echo -e "  [X] $dep"
        done
        echo ""
        echo -e "${YELLOW}To install all dependencies on Ubuntu/Debian:${NC}"
        echo ""
        echo -e "${GREEN}sudo apt update${NC}"
        echo -e "${GREEN}sudo apt install cmake build-essential libgtest-dev ninja-build${NC}"
        echo ""
        exit 1
    fi
    
    # Show optional missing packages
    if [ ${#optional_missing[@]} -gt 0 ]; then
        echo -e "${YELLOW}===============================================${NC}"
        echo -e "${YELLOW}     Optional Dependencies (Recommended)${NC}"
        echo -e "${YELLOW}===============================================${NC}"
        echo ""
        echo "The following optional packages would improve build speed:"
        for dep in "${optional_missing[@]}"; do
            echo "  [?] $dep"
        done
        echo ""
        echo "To install (recommended for faster builds):"
        echo "  sudo apt install ninja-build"
        echo ""
        echo "Continuing with Make as the build system..."
        echo ""
        sleep 2
    fi
}

# Determine build generator
set_generator() {
    if [ -n "$GENERATOR" ]; then
        # User explicitly specified a generator
        return
    fi
    
    if [ "$USE_NINJA" = true ] && command -v ninja &> /dev/null; then
        GENERATOR="Ninja"
    else
        GENERATOR="Unix Makefiles"
    fi
}

# Help message
show_help() {
    cat << EOF
HFT Platform Build Script

Usage: ./build.sh [OPTIONS]

OPTIONS:
    --help, -h              Show this help message
    --compiler COMPILER     Set compiler (gcc, clang, g++, clang++) [default: gcc]
    --debug                 Build in Debug mode with sanitizers
    --release               Build in Release mode with optimizations [default]
    --clean                 Clean build directory before building
    --test [FILTER]         Run tests after building (optional filter pattern)
    --demo                  Run demo programs after building
    --all                   Build and run everything (tests + demos)
    --verbose, -v           Enable verbose output
    --build-dir DIR         Specify build directory [default: build]
    --use-make              Force use of Make instead of Ninja
    --generator GEN         Manually specify CMake generator (Ninja, "Unix Makefiles")

BUILD SYSTEM:
    By default, this script uses Ninja for faster builds. If Ninja is not installed,
    it falls back to Make. Ninja is 2-3x faster than Make for incremental builds.
    
    Install Ninja:
        sudo apt install ninja-build

TEST FILTERING:
    Run specific tests using gtest filter patterns:
    
    --test "OrderBookTest.*"              # Run all OrderBook tests
    --test "MicrostructureTest.*"         # Run all Microstructure tests
    --test "*Imbalance*"                  # Run all tests with "Imbalance" in name
    --test "CMEScenariosTest.ESNormalTrading"  # Run specific test
    --test "*CME*"                        # Run all CME-related tests
    --test "*NASDAQ*"                     # Run all NASDAQ-related tests

EXAMPLES:
    ./build.sh                                      # Standard release build
    ./build.sh --clean --test                       # Clean build and run all tests
    ./build.sh --test "OrderBookTest.*"             # Run only OrderBook tests
    ./build.sh --compiler clang --debug --test      # Debug build with clang
    ./build.sh --all                                # Build and run everything
    ./build.sh --use-make --clean                   # Force Make instead of Ninja

AVAILABLE TEST SUITES:
    OrderBookTest           - Basic order book functionality
    MicrostructureTest      - Market microstructure signals
    RegulatoryLimitsTest    - Regulatory compliance checks
    CMEScenariosTest        - CME exchange scenarios
    NASDAQScenariosTest     - NASDAQ exchange scenarios

DEPENDENCIES:
    Required: cmake, g++ (or clang++), libgtest-dev
    Recommended: ninja-build (for faster builds)
    
    Install on Ubuntu/Debian:
        sudo apt install cmake build-essential libgtest-dev ninja-build

EOF
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --help|-h)
            show_help
            exit 0
            ;;
        --compiler)
            COMPILER="$2"
            shift 2
            ;;
        --debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        --release)
            BUILD_TYPE="Release"
            shift
            ;;
        --clean)
            CLEAN_BUILD=true
            shift
            ;;
        --test)
            RUN_TESTS=true
            if [[ $# -gt 1 && ! $2 =~ ^-- ]]; then
                TEST_FILTER="$2"
                shift 2
            else
                shift
            fi
            ;;
        --demo)
            RUN_DEMOS=true
            shift
            ;;
        --all)
            RUN_TESTS=true
            RUN_DEMOS=true
            shift
            ;;
        --verbose|-v)
            VERBOSE=true
            shift
            ;;
        --build-dir)
            BUILD_DIR="$2"
            shift 2
            ;;
        --use-make)
            USE_NINJA=false
            GENERATOR="Unix Makefiles"
            shift
            ;;
        --generator)
            GENERATOR="$2"
            shift 2
            ;;
        *)
            echo -e "${RED}Unknown option: $1${NC}"
            show_help
            exit 1
            ;;
    esac
done

# Validate and set compiler
set_compiler() {
    case $COMPILER in
        gcc|g++)
            export CC=gcc
            export CXX=g++
            ;;
        clang|clang++)
            export CC=clang
            export CXX=clang++
            ;;
        *)
            echo -e "${RED}Error: Unknown compiler '$COMPILER'${NC}"
            echo "Supported compilers: gcc, g++, clang, clang++"
            exit 1
            ;;
    esac
}

# Print build configuration
print_config() {
    echo -e "${BLUE}===============================================${NC}"
    echo -e "${BLUE}     HFT Platform Build Configuration${NC}"
    echo -e "${BLUE}===============================================${NC}"
    echo ""
    echo -e "  ${GREEN}Compiler:${NC}      $CXX ($($CXX --version | head -n1))"
    echo -e "  ${GREEN}Build Type:${NC}    $BUILD_TYPE"
    echo -e "  ${GREEN}Generator:${NC}     $GENERATOR"
    echo -e "  ${GREEN}Build Dir:${NC}     $BUILD_DIR"
    echo -e "  ${GREEN}Clean Build:${NC}   $CLEAN_BUILD"
    echo -e "  ${GREEN}Run Tests:${NC}     $RUN_TESTS"
    if [ -n "$TEST_FILTER" ]; then
        echo -e "  ${GREEN}Test Filter:${NC}   $TEST_FILTER"
    fi
    echo -e "  ${GREEN}Run Demos:${NC}     $RUN_DEMOS"
    echo ""
}

# Clean build directory
clean_build() {
    if [ "$CLEAN_BUILD" = true ]; then
        echo -e "${YELLOW}Cleaning build directory...${NC}"
        rm -rf "$BUILD_DIR"
        echo -e "${GREEN}Clean complete${NC}"
        echo ""
    fi
}

# Configure with CMake
configure_cmake() {
    echo -e "${YELLOW}Configuring with CMake ($GENERATOR)...${NC}"
    mkdir -p "$BUILD_DIR"
    
    if [ "$VERBOSE" = true ]; then
        if ! cmake -B "$BUILD_DIR" -G "$GENERATOR" -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DCMAKE_EXPORT_COMPILE_COMMANDS=ON; then
            echo -e "${RED}===============================================${NC}"
            echo -e "${RED}CMake configuration FAILED${NC}"
            echo -e "${RED}===============================================${NC}"
            exit 1
        fi
    else
        if ! cmake -B "$BUILD_DIR" -G "$GENERATOR" -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DCMAKE_EXPORT_COMPILE_COMMANDS=ON > /dev/null 2>&1; then
            echo -e "${RED}===============================================${NC}"
            echo -e "${RED}CMake configuration FAILED${NC}"
            echo -e "${RED}===============================================${NC}"
            echo ""
            echo "Re-running with verbose output to show error:"
            echo ""
            cmake -B "$BUILD_DIR" -G "$GENERATOR" -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
            exit 1
        fi
    fi
    
    echo -e "${GREEN}CMake configuration complete${NC}"
    echo ""
}

# Build the project
build_project() {
    echo -e "${YELLOW}Building project with $GENERATOR...${NC}"
    echo ""
    
    # Determine number of cores for parallel build
    if [[ "$OSTYPE" == "darwin"* ]]; then
        CORES=$(sysctl -n hw.ncpu)
    else
        CORES=$(nproc)
    fi
    
    cd "$BUILD_DIR"
    
    local build_failed=false
    
    if [ "$GENERATOR" = "Ninja" ]; then
        if [ "$VERBOSE" = true ]; then
            if ! ninja -j$CORES; then
                build_failed=true
            fi
        else
            if ! ninja -j$CORES 2>&1; then
                build_failed=true
            fi
        fi
    else
        if [ "$VERBOSE" = true ]; then
            if ! make -j$CORES; then
                build_failed=true
            fi
        else
            if ! make -j$CORES 2>&1; then
                build_failed=true
            fi
        fi
    fi
    
    cd ..
    
    if [ "$build_failed" = true ]; then
        echo ""
        echo -e "${RED}===============================================${NC}"
        echo -e "${RED}BUILD FAILED${NC}"
        echo -e "${RED}===============================================${NC}"
        echo ""
        exit 1
    fi
    
    echo ""
    echo -e "${GREEN}Build complete [Success]${NC}"
    echo ""
}

# Run tests
run_tests() {
    if [ "$RUN_TESTS" = true ]; then
        echo -e "${BLUE}===============================================${NC}"
        echo -e "${BLUE}          Running Test Suite${NC}"
        echo -e "${BLUE}===============================================${NC}"
        echo ""
        
        cd "$BUILD_DIR"
        
        local test_failed=false
        
        if [ -n "$TEST_FILTER" ]; then
            echo -e "${YELLOW}Running filtered tests: $TEST_FILTER${NC}"
            echo ""
            if ! ./run_tests --gtest_filter="$TEST_FILTER"; then
                test_failed=true
            fi
        else
            echo -e "${YELLOW}Running all tests...${NC}"
            echo ""
            if ! ./run_tests; then
                test_failed=true
            fi
        fi
        
        cd ..
        
        if [ "$test_failed" = true ]; then
            echo ""
            echo -e "${RED}===============================================${NC}"
            echo -e "${RED}TESTS FAILED${NC}"
            echo -e "${RED}===============================================${NC}"
            echo ""
            exit 1
        fi
        
        echo ""
        echo -e "${GREEN}Tests completed [Success]${NC}"
        echo ""
    fi
}

# Run demos
run_demos() {
    if [ "$RUN_DEMOS" = true ]; then
        echo -e "${BLUE}===============================================${NC}"
        echo -e "${BLUE}           Running Demos${NC}"
        echo -e "${BLUE}===============================================${NC}"
        echo ""
        
        cd "$BUILD_DIR"
        
        echo -e "${YELLOW}Running Microstructure Demo...${NC}"
        if ! ./demo_microstructure; then
            echo -e "${RED}Microstructure demo FAILED${NC}"
            cd ..
            exit 1
        fi
        echo ""
        
        echo -e "${YELLOW}Running Regulatory Demo...${NC}"
        if ! ./demo_regulatory; then
            echo -e "${RED}Regulatory demo FAILED${NC}"
            cd ..
            exit 1
        fi
        echo ""
        
        echo -e "${YELLOW}Running Full System Demo...${NC}"
        if ! ./demo_full_system; then
            echo -e "${RED}Full system demo FAILED${NC}"
            cd ..
            exit 1
        fi
        echo ""
        
        cd ..
        
        echo -e "${GREEN}All demos completed [Success]${NC}"
        echo ""
    fi
}

# Show build summary
show_summary() {
    echo -e "${BLUE}===============================================${NC}"
    echo -e "${BLUE}          Build Summary${NC}"
    echo -e "${BLUE}===============================================${NC}"
    echo ""
    echo -e "  ${GREEN}Build completed successfully${NC}"
    echo ""
    echo "  Build system: $GENERATOR"
    echo "  Binaries available in: $BUILD_DIR/"
    echo ""
    echo "  Unified test executable:"
    echo "    - run_tests                (all tests in one binary)"
    echo ""
    echo "  Demo executables:"
    echo "    - demo_microstructure"
    echo "    - demo_regulatory"
    echo "    - demo_full_system"
    echo ""
    
    if [ "$RUN_TESTS" = false ]; then
        echo -e "${YELLOW}  Run Tests:${NC}"
        echo "    cd $BUILD_DIR && ./run_tests"
        echo ""
        echo -e "${YELLOW}  Run Specific Tests:${NC}"
        echo "    cd $BUILD_DIR && ./run_tests --gtest_filter='OrderBookTest.*'"
        echo "    cd $BUILD_DIR && ./run_tests --gtest_filter='*Microstructure*'"
        echo ""
        echo -e "${YELLOW}  List All Tests:${NC}"
        echo "    cd $BUILD_DIR && ./run_tests --gtest_list_tests"
        echo ""
        echo -e "${YELLOW}  Using build.sh:${NC}"
        echo "    ./build.sh --test                              # Run all tests"
        echo "    ./build.sh --test 'OrderBookTest.*'            # Run OrderBook tests"
        echo "    ./build.sh --test '*CME*'                      # Run all CME tests"
        echo ""
    fi
    
    if [ "$RUN_DEMOS" = false ]; then
        echo -e "${YELLOW}  Run Demos:${NC}"
        echo "    cd $BUILD_DIR && ./demo_microstructure"
        echo "    cd $BUILD_DIR && ./demo_regulatory"
        echo "    cd $BUILD_DIR && ./demo_full_system"
        echo ""
    fi
}

# Main execution flow
main() {
    check_dependencies
    set_compiler
    set_generator
    print_config
    clean_build
    configure_cmake
    build_project
    run_tests
    run_demos
    show_summary
}

# Run main function
main
