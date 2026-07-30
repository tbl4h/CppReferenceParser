#ifndef CPPREF_CONFIG_HPP
#define CPPREF_CONFIG_HPP

#include <string_view>
#include <vector>

namespace cppref {

/// Configuration settings for the C++ reference parser
struct Config {
    /// Output format (e.g., "markdown", "text", "rst")
    std::string_view output_format = "markdown";
    
    /// CSS classes to skip during parsing
    std::vector<std::string_view> skip_classes = {
        "navbox",
        "referencelist",
        "mw-editsection"
    };
    
    /// Code fence character for code blocks
    std::string_view code_fence = "```";
    
    /// Whether to preserve links in output
    bool preserve_links = true;
    
    /// Link format template when preserve_links is true
    /// Supported placeholders: {text}, {url}
    std::string_view link_format = "[{text}]({url})";
    
    /// Maximum heading level to include (default: 6)
    int max_heading_level = 6;
    
    /// Whether to include page metadata
    bool include_metadata = true;
    
    /// Metadata fields to include
    std::vector<std::string_view> metadata_fields = {
        "title",
        "last_modified"
    };
};

/// Global configuration instance
inline Config config;

} // namespace cppref

#endif // CPPREF_CONFIG_HPP