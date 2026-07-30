#ifndef CPPREFERENCE_EXTRACTOR_HPP
#define CPPREFERENCE_EXTRACTOR_HPP

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>
#include <memory>

#include "config.hpp"

class CppReferenceExtractor
{
public:
    /// Simple HTML element node for built-in parsing
    struct HtmlNode {
        enum class Type {
            Element,    // <tag ...>...</tag>
            Text,       // plain text content
            Comment,    // <!-- ... -->
        };

        Type type = Type::Element;
        std::string tag;              // element tag name (e.g., "h1", "p", "pre")
        std::string attributes;       // raw attributes string
        std::string class_value;      // class attribute value
        std::string id_value;         // id attribute value
        std::string text;             // text content
        std::vector<std::shared_ptr<HtmlNode>> children;

        HtmlNode() = default;
        explicit HtmlNode(Type t) : type(t) {}
    };

    /// Supported output formats
    enum class Format {
        Markdown,      /// Markdown format with headings, code blocks
        PlainText,     /// Plain text with minimal formatting
        ReStructuredText /// ReStructuredText format
    };

    /// Table types for parameter vs description sections
    enum class TableType {
        None,          /// Not in a table
        Declaration,   /// Function declaration table
        Parameters,    /// Parameters table
        Description,   /// Description table
        Generic        /// Other table
    };

    /// HTML tokenizer
    struct Token {
        enum class Type { OpenTag, CloseTag, Text, Comment, Eof };
        Type type = Type::Eof;
        std::string tag;
        std::string attributes;
        std::string text;
    };

    /// Configuration for the extractor
    struct ExtractionConfig {
        /// Output format (default: Markdown)
        Format format;

        /// CSS classes to skip during parsing
        std::vector<std::string_view> skip_classes;

        /// Code fence character for code blocks
        std::string_view code_fence;

        /// Whether to preserve links in output
        bool preserve_links;

        /// Link format template when preserve_links is true
        /// Supported placeholders: {text}, {url}
        std::string_view link_format;

        /// Maximum heading level to include (default: 6)
        int max_heading_level;

        /// Whether to include page metadata
        bool include_metadata;

        /// Metadata fields to include
        std::vector<std::string_view> metadata_fields;

        ExtractionConfig();
    };

    CppReferenceExtractor();
    ~CppReferenceExtractor();

    /// Extract text from HTML file and write to output file
    [[nodiscard]] bool Convert(
        const std::filesystem::path &inputFile,
        const std::filesystem::path &outputFile,
        const ExtractionConfig &config = ExtractionConfig{});

    /// Extract text from HTML file and write to output stream
    [[nodiscard]] bool Convert(
        const std::filesystem::path &inputFile,
        std::ostream &output,
        const ExtractionConfig &config = ExtractionConfig{});

    /// Extract text from HTML string
    [[nodiscard]] std::string ExtractFromMemory(
        const std::string &html,
        const ExtractionConfig &config = ExtractionConfig{});

    /// Get version information
    [[nodiscard]] static std::string Version();

    /// Get library name
    [[nodiscard]] static std::string Name();

private:
    /// HTML parser utilities
    static std::shared_ptr<HtmlNode> ParseHtml(const std::string &html);
    static std::shared_ptr<HtmlNode> ParseHtmlFragment(const std::string &html);

    static std::vector<Token> Tokenize(const std::string &html);
    static std::shared_ptr<HtmlNode> BuildTree(const std::vector<Token> &tokens, size_t &pos);

    /// Walker and visitor
    void Walk(const HtmlNode *node);
    void WalkChildren(const HtmlNode *node);
    void VisitElement(const HtmlNode *element);
    void VisitText(const HtmlNode *textNode);
    void LeaveElement(const HtmlNode *element);
    bool ShouldSkipElement(const HtmlNode *element) const;
    [[nodiscard]] bool HasClass(const HtmlNode *node, std::string_view className) const;
    [[nodiscard]] TableType DetectTableType(const HtmlNode *node) const;
    [[nodiscard]] std::string_view DetectCodeLanguage(const HtmlNode *node) const;
    void WritePageHeader();
    void WriteFooter();
    const HtmlNode *FindElementById(const std::string &id) const;
    const HtmlNode *FindContentRoot(const std::vector<std::shared_ptr<HtmlNode>> &children) const;

    /// Output writers
    void Write(const char *text);
    void Write(std::string_view text);
    void VisitTextRaw(const std::string &text);

    /// Buffer utilities
    void AppendRawByte(char ch);
    void AppendTextSpace();
    void EnsureNewline();
    void AppendNewline();
    void TrimTrailingInlineWhitespace();
    void TrimOutput();

    /// HTML helpers
    static std::string ExtractClass(const std::string &attributes);
    static std::string ExtractId(const std::string &attributes);
    static std::string DecodeHtmlEntities(const std::string &text);
    static std::string ToLower(const std::string &s);

    std::ostream *m_output;
    std::string m_outputText;
    std::vector<TableType> m_tableStack;
    TableType m_tableType;
    size_t m_tableColumn;
    size_t m_preDepth;
    size_t m_ddDepth;
    std::string m_pageTitle;
};

#endif // CPPREFERENCE_EXTRACTOR_HPP