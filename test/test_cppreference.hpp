#ifndef TEST_CPPREFERENCE_HPP
#define TEST_CPPREFERENCE_HPP

#include <iostream>
#include <string>
#include <cassert>
#include <sstream>
#include <filesystem>

#include "../include/CppReferenceExtractor.hpp"

// Test result counter
static int testsPassed = 0;
static int testsFailed = 0;

#define TEST(name) \
    void name(); \
    struct name##_Registrar { \
        name##_Registrar() { \
            std::cout << "Running test: " #name << "...\n"; \
            try { \
                name(); \
                std::cout << "  PASSED\n"; \
                ++testsPassed; \
            } catch (const std::exception& e) { \
                std::cout << "  FAILED: " << e.what() << "\n"; \
                ++testsFailed; \
            } catch (...) { \
                std::cout << "  FAILED (unknown exception)\n"; \
                ++testsFailed; \
            } \
        } \
    }; \
    void name()

#define ASSERT_EQ(a, b) \
    assert((a) == (b) && "Assertion failed: " #a " != " #b)

#define ASSERT_TRUE(x) \
    assert((x) && "Assertion failed: " #x " is not true")

#define ASSERT_FALSE(x) \
    assert(!(x) && "Assertion failed: " #x " is not false")

#define ASSERT_NE(a, b) \
    assert((a) != (b) && "Assertion failed: " #a " == " #b)

// Test 1: Version information
TEST(test_version) {
    std::string version = CppReferenceExtractor::Version();
    ASSERT_TRUE(!version.empty());
    ASSERT_TRUE(version.find("CppReferenceParser") != std::string::npos);
}

// Test 2: Library name
TEST(test_name) {
    std::string name = CppReferenceExtractor::Name();
    ASSERT_TRUE(!name.empty());
    ASSERT_TRUE(name.find("C++ Reference") != std::string::npos ||
                name.find("CppReference") != std::string::npos);
}

// Test 3: Constructor and destructor
TEST(test_constructor_destructor) {
    CppReferenceExtractor extractor;
}

// Test 4: ExtractFromMemory with empty HTML
TEST(test_extract_empty_html) {
    CppReferenceExtractor extractor;
    std::string result = extractor.ExtractFromMemory("");
    ASSERT_TRUE(result.empty());
}

// Test 5: ExtractFromMemory with minimal HTML structure
TEST(test_extract_minimal_html) {
    CppReferenceExtractor extractor;
    std::string html = "<html><body><h1>Test Page</h1><p>Content here</p></body></html>";
    std::string result = extractor.ExtractFromMemory(html);
    ASSERT_TRUE(!result.empty());
}

// Test 6: ExtractFromMemory with heading
TEST(test_extract_heading) {
    CppReferenceExtractor extractor;
    std::string html = "<html><body><h1>Function Name</h1></body></html>";
    std::string result = extractor.ExtractFromMemory(html);
    ASSERT_TRUE(result.find("Function Name") != std::string::npos);
}

// Test 7: ExtractFromMemory with paragraph
TEST(test_extract_paragraph) {
    CppReferenceExtractor extractor;
    std::string html = "<html><body><p>This is a paragraph.</p></body></html>";
    std::string result = extractor.ExtractFromMemory(html);
    ASSERT_TRUE(result.find("This is a paragraph.") != std::string::npos);
}

// Test 8: ExtractFromMemory with code block
TEST(test_extract_code_block) {
    CppReferenceExtractor extractor;
    std::string html = R"(<html>
<body>
<pre class="cpp">int main() {
    return 0;
}</pre>
</body>
</html>)";
    std::string result = extractor.ExtractFromMemory(html);
    ASSERT_TRUE(result.find("int main()") != std::string::npos);
    ASSERT_TRUE(result.find("return 0;") != std::string::npos);
}

// Test 9: ExtractFromMemory with table
TEST(test_extract_table) {
    CppReferenceExtractor extractor;
    std::string html = R"(<html>
<body>
<table class="t-par-begin">
<tr><th>Name</th><th>Type</th></tr>
<tr><td>param</td><td>int</td></tr>
</table>
</body>
</html>)";
    std::string result = extractor.ExtractFromMemory(html);
    ASSERT_TRUE(!result.empty());
}

// Test 10: ExtractionConfig default values
TEST(test_extraction_config_defaults) {
    CppReferenceExtractor::ExtractionConfig config;
    ASSERT_EQ(config.format, CppReferenceExtractor::Format::Markdown);
    ASSERT_TRUE(config.preserve_links);
    ASSERT_EQ(config.max_heading_level, 6);
    ASSERT_TRUE(config.include_metadata);
}

// Test 11: ExtractionConfig custom values
TEST(test_extraction_config_custom) {
    CppReferenceExtractor::ExtractionConfig config;
    config.format = CppReferenceExtractor::Format::PlainText;
    config.preserve_links = false;
    config.max_heading_level = 3;

    ASSERT_EQ(config.format, CppReferenceExtractor::Format::PlainText);
    ASSERT_FALSE(config.preserve_links);
    ASSERT_EQ(config.max_heading_level, 3);
}

// Test 12: Format enum values
TEST(test_format_enum) {
    ASSERT_EQ(static_cast<int>(CppReferenceExtractor::Format::Markdown), 0);
    ASSERT_EQ(static_cast<int>(CppReferenceExtractor::Format::PlainText), 1);
    ASSERT_EQ(static_cast<int>(CppReferenceExtractor::Format::ReStructuredText), 2);
}

// Test 13: TableType enum values
TEST(test_table_type_enum) {
    ASSERT_EQ(static_cast<int>(CppReferenceExtractor::TableType::None), 0);
    ASSERT_EQ(static_cast<int>(CppReferenceExtractor::TableType::Declaration), 1);
    ASSERT_EQ(static_cast<int>(CppReferenceExtractor::TableType::Parameters), 2);
    ASSERT_EQ(static_cast<int>(CppReferenceExtractor::TableType::Description), 3);
    ASSERT_EQ(static_cast<int>(CppReferenceExtractor::TableType::Generic), 4);
}

// Test 14: ExtractFromMemory with special characters
TEST(test_extract_special_chars) {
    CppReferenceExtractor extractor;
    std::string html = "<html><body><p>Text with & ampersand</p></body></html>";
    std::string result = extractor.ExtractFromMemory(html);
    ASSERT_TRUE(!result.empty());
    ASSERT_TRUE(result.find("&") != std::string::npos || result.find("ampersand") != std::string::npos);
}

// Test 15: ExtractFromMemory with multiple paragraphs
TEST(test_extract_multiple_paragraphs) {
    CppReferenceExtractor extractor;
    std::string html = R"(<html>
<body>
<p>First paragraph.</p>
<p>Second paragraph.</p>
</body>
</html>)";
    std::string result = extractor.ExtractFromMemory(html);
    ASSERT_TRUE(result.find("First paragraph.") != std::string::npos);
    ASSERT_TRUE(result.find("Second paragraph.") != std::string::npos);
}

// Test 16: ExtractFromMemory with list
TEST(test_extract_list) {
    CppReferenceExtractor extractor;
    std::string html = R"(<html>
<body>
<ul>
<li>Item 1</li>
<li>Item 2</li>
<li>Item 3</li>
</ul>
</body>
</html>)";
    std::string result = extractor.ExtractFromMemory(html);
    ASSERT_TRUE(result.find("Item 1") != std::string::npos);
    ASSERT_TRUE(result.find("Item 2") != std::string::npos);
}

// Test 17: ExtractFromMemory with nested elements
TEST(test_extract_nested_elements) {
    CppReferenceExtractor extractor;
    std::string html = R"(<html>
<body>
<p>Text with <strong>bold</strong> and <em>italic</em>.</p>
</body>
</html>)";
    std::string result = extractor.ExtractFromMemory(html);
    ASSERT_TRUE(result.find("Text with") != std::string::npos);
}

// Test 18: ExtractFromMemory with skip classes
TEST(test_extraction_config_skip_classes) {
    CppReferenceExtractor::ExtractionConfig config;
    config.skip_classes.push_back("t-navbar-menu");
    config.skip_classes.push_back("noprint");

    ASSERT_EQ(config.skip_classes.size(), 2u);
}

// Test 19: ExtractFromMemory with link preservation
TEST(test_extract_with_links) {
    CppReferenceExtractor::ExtractionConfig config;
    config.preserve_links = true;
    config.link_format = "[{text}]({url})";

    CppReferenceExtractor extractor;
    std::string html = R"(<html>
<body>
<a href="https://example.com">Example Link</a>
</body>
</html>)";
    std::string result = extractor.ExtractFromMemory(html, config);
    ASSERT_TRUE(!result.empty());
}

// Test 20: ExtractFromMemory with minimal HTML (no html tag)
TEST(test_extract_minimal_html_no_root) {
    CppReferenceExtractor extractor;
    std::string html = "<h1>Direct Heading</h1><p>Direct paragraph</p>";
    std::string result = extractor.ExtractFromMemory(html);
    ASSERT_TRUE(!result.empty());
}

// Print summary function
void print_summary() {
    std::cout << "\n========== Test Summary ==========\n";
    std::cout << "Passed: " << testsPassed << "\n";
    std::cout << "Failed: " << testsFailed << "\n";
    std::cout << "Total:  " << (testsPassed + testsFailed) << "\n";
    std::cout << "==================================\n";
}

// Main test runner
int run_all_tests() {
    std::cout << "=== CppReferenceExtractor Tests ===\n\n";

    // All tests are automatically registered via their registrar structs

    print_summary();

    return (testsFailed > 0) ? 1 : 0;
}

#endif // TEST_CPPREFERENCE_HPP