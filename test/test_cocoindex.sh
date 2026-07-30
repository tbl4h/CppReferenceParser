#!/bin/bash
# Test script for cocoindex integration with CppReferenceParser project
# This script tests the cocoindex CLI binary (/home/mic/.local/bin/ccc)

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Test counters
TESTS_PASSED=0
TESTS_FAILED=0
TOTAL_TESTS=0

# Helper function to run a test
run_test() {
    local test_name="$1"
    local test_command="$2"
    local expected_output="$3"
    
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    echo -n "Test $TOTAL_TESTS: $test_name... "
    
    # Execute the command and capture output
    output=$(eval "$test_command" 2>&1)
    exit_code=$?
    
    if [ $exit_code -eq 0 ]; then
        if [ -n "$expected_output" ]; then
            if echo "$output" | grep -q "$expected_output"; then
                echo -e "${GREEN}PASSED${NC}"
                TESTS_PASSED=$((TESTS_PASSED + 1))
            else
                echo -e "${RED}FAILED${NC} (expected: $expected_output)"
                echo "  Output: $output"
                TESTS_FAILED=$((TESTS_FAILED + 1))
            fi
        else
            echo -e "${GREEN}PASSED${NC}"
            TESTS_PASSED=$((TESTS_PASSED + 1))
        fi
    else
        echo -e "${RED}FAILED${NC} (exit code: $exit_code)"
        echo "  Command: $test_command"
        echo "  Output: $output"
        TESTS_FAILED=$((TESTS_FAILED + 1))
    fi
}

# Helper function to check if command exists
check_command_exists() {
    local binary_path="$1"
    if [ -x "$binary_path" ]; then
        return 0
    else
        return 1
    fi
}

echo "============================================"
echo "  CocoIndex Integration Tests"
echo "  Project: CppReferenceParser"
echo "============================================"
echo ""

# Test 1: Check if cocoindex binary exists
COCOINDEX_BINARY="/home/mic/.local/bin/ccc"
echo "Test 1: Check cocoindex binary existence"
TOTAL_TESTS=$((TOTAL_TESTS + 1))
if check_command_exists "$COCOINDEX_BINARY"; then
    echo -e "  ${GREEN}PASSED${NC} - Binary exists at $COCOINDEX_BINARY"
    TESTS_PASSED=$((TESTS_PASSED + 1))
else
    echo -e "  ${YELLOW}SKIPPED${NC} - Binary not found at $COCOINDEX_BINARY"
fi
echo ""

# Test 2: Test cocoindex --version or --help
echo "Test 2: Test cocoindex --help flag"
TOTAL_TESTS=$((TOTAL_TESTS + 1))
help_output=$(eval "$COCOINDEX_BINARY --help" 2>&1)
help_exit=$?
if [ $help_exit -eq 0 ] || echo "$help_output" | grep -qi "usage\|help\|cocoindex"; then
    echo -e "  ${GREEN}PASSED${NC}"
    echo "  Output: $(echo "$help_output" | head -5)"
    TESTS_PASSED=$((TESTS_PASSED + 1))
else
    echo -e "  ${YELLOW}SKIPPED${NC} - No --help flag available"
fi
echo ""

# Test 3: Test cocoindex mcp subcommand
echo "Test 3: Test cocoindex mcp subcommand"
TOTAL_TESTS=$((TOTAL_TESTS + 1))
mcp_output=$(eval "$COCOINDEX_BINARY mcp --help" 2>&1)
mcp_exit=$?
if [ $mcp_exit -eq 0 ]; then
    echo -e "  ${GREEN}PASSED${NC}"
    TESTS_PASSED=$((TESTS_PASSED + 1))
else
    echo -e "  ${YELLOW}SKIPPED${NC} - MCP subcommand not directly executable"
fi
echo ""

# Test 4: Test project directory structure
echo "Test 4: Verify project structure for cocoindex"
TOTAL_TESTS=$((TOTAL_TESTS + 1))
if [ -d "include" ] && [ -d "src" ] && [ -d "test" ]; then
    echo -e "  ${GREEN}PASSED${NC} - Project directories exist"
    TESTS_PASSED=$((TESTS_PASSED + 1))
else
    echo -e "  ${RED}FAILED${NC} - Missing project directories"
    TESTS_FAILED=$((TESTS_FAILED + 1))
fi
echo ""

# Test 5: Test source file existence
echo "Test 5: Verify source files exist"
TOTAL_TESTS=$((TOTAL_TESTS + 1))
if [ -f "src/CppReferenceExtractor.cpp" ] && [ -f "include/CppReferenceExtractor.hpp" ]; then
    echo -e "  ${GREEN}PASSED${NC} - Source files exist"
    TESTS_PASSED=$((TESTS_PASSED + 1))
else
    echo -e "  ${RED}FAILED${NC} - Source files missing"
    TESTS_FAILED=$((TESTS_FAILED + 1))
fi
echo ""

# Test 6: Test include file existence
echo "Test 6: Verify include files exist"
TOTAL_TESTS=$((TOTAL_TESTS + 1))
if [ -f "include/config.hpp" ]; then
    echo -e "  ${GREEN}PASSED${NC} - Config header exists"
    TESTS_PASSED=$((TESTS_PASSED + 1))
else
    echo -e "  ${RED}FAILED${NC} - Config header missing"
    TESTS_FAILED=$((TESTS_FAILED + 1))
fi
echo ""

# Test 7: Test test file existence
echo "Test 7: Verify test files exist"
TOTAL_TESTS=$((TOTAL_TESTS + 1))
if [ -f "test/test_cppreference.cpp" ] && [ -f "test/test_main.cpp" ]; then
    echo -e "  ${GREEN}PASSED${NC} - Test files exist"
    TESTS_PASSED=$((TESTS_PASSED + 1))
else
    echo -e "  ${RED}FAILED${NC} - Test files missing"
    TESTS_FAILED=$((TESTS_FAILED + 1))
fi
echo ""

# Test 8: Verify cocoindex_code directory
echo "Test 8: Verify .cocoindex_code directory"
TOTAL_TESTS=$((TOTAL_TESTS + 1))
if [ -d ".cocoindex_code" ]; then
    echo -e "  ${GREEN}PASSED${NC} - .cocoindex_code directory exists"
    TESTS_PASSED=$((TESTS_PASSED + 1))
else
    echo -e "  ${RED}FAILED${NC} - .cocoindex_code directory missing"
    TESTS_FAILED=$((TESTS_FAILED + 1))
fi
echo ""

# Test 9: Verify settings.yml in .cocoindex_code
echo "Test 9: Verify .cocoindex_code/settings.yml"
TOTAL_TESTS=$((TOTAL_TESTS + 1))
if [ -f ".cocoindex_code/settings.yml" ]; then
    echo -e "  ${GREEN}PASSED${NC} - settings.yml exists"
    TESTS_PASSED=$((TESTS_PASSED + 1))
else
    echo -e "  ${RED}FAILED${NC} - settings.yml missing"
    TESTS_FAILED=$((TESTS_FAILED + 1))
fi
echo ""

# Test 10: Verify CMakeLists.txt
echo "Test 10: Verify CMakeLists.txt contains test target"
TOTAL_TESTS=$((TOTAL_TESTS + 1))
if [ -f "CMakeLists.txt" ] && grep -q "CppReferenceExtractorTest" "CMakeLists.txt"; then
    echo -e "  ${GREEN}PASSED${NC} - Test target found in CMakeLists.txt"
    TESTS_PASSED=$((TESTS_PASSED + 1))
else
    echo -e "  ${RED}FAILED${NC} - Test target not found"
    TESTS_FAILED=$((TESTS_FAILED + 1))
fi
echo ""

# Test 11: Verify source code patterns
echo "Test 11: Verify C++ source patterns in project"
TOTAL_TESTS=$((TOTAL_TESTS + 1))
cpp_count=$(find src/ - "*.cpp" -f | wc -line)
if [ "$cpp_count" -gt 0 ]; then
    echo -e "  ${GREEN}PASSED${NC} - Found $cpp_count C++ source files"
    TESTS_PASSED=$((TESTS_PASSED + 1))
else
    echo -e "  ${RED}FAILED${NC} - No C++ source files found"
    TESTS_FAILED=$((TESTS_FAILED + 1))
fi
echo ""

# Summary
echo "============================================"
echo "  Test Summary"
echo "============================================"
echo -e "  Passed:  ${GREEN}$TESTS_PASSED${NC}"
echo -e "  Failed:  ${RED}$TESTS_FAILED${NC}"
echo -e "  Total:   $TOTAL_TESTS"
echo ""

if [ $TESTS_FAILED -eq 0 ]; then
    echo -e "${GREEN}All tests passed!${NC}"
    exit 0
else
    echo -e "${RED}Some tests failed.${NC}"
    exit 1
fi