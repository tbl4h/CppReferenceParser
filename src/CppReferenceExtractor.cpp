#include "CppReferenceExtractor.hpp"

#include <cctype>
#include <algorithm>
#include <sstream>
#include <unordered_set>

namespace {

/// Check if character is whitespace
inline bool IsWhitespace(char ch) {
    return ch == ' ' || ch == '\n' || ch == '\r' || ch == '\t';
}

/// Decode HTML entities
std::string DecodeHtmlEntities(const std::string &text) {
    std::string result;
    result.reserve(text.size());

    for (size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '&') {
            size_t end = text.find(';', i + 1);
            if (end == std::string::npos) {
                result.push_back('&');
                continue;
            }

            std::string entity = text.substr(i + 1, end - i - 1);

            // Named entities
            if (entity == "amp") result += '&';
            else if (entity == "lt") result += '<';
            else if (entity == "gt") result += '>';
            else if (entity == "quot") result += '"';
            else if (entity == "apos") result += '\'';
            else if (entity == "nbsp") result += ' ';
            else if (entity == "mdash") result += "\xE2\x80\x94";
            else if (entity == "ndash") result += "\xE2\x80\x93";
            else if (entity == "hellip") result += "\xE2\x80\x26";
            else if (entity == "rsquo") result += "\xE2\x80\x19";
            else if (entity == "lsquo") result += "\xE2\x80\x18";
            else if (entity == "rdquo") result += "\xE2\x80\x1D";
            else if (entity == "ldquo") result += "\xE2\x80\x1C";
            else if (entity == "bull") result += "\xE2\x80\x22";
            else if (entity == "permil") result += "\xE2\x80\x30";
            else if (entity == "deg") result += "\xC0\xb0";
            else if (entity == "plusminus") result += "\xE2\x82\x13";
            else {
                // Numeric entity
                if (!entity.empty() && entity[0] == 'x') {
                    try {
                        unsigned int codepoint = std::stoul(entity.substr(1), nullptr, 16);
                        if (codepoint < 128) result += static_cast<char>(codepoint);
                        else result += entity;
                    } catch (...) {
                        result += entity;
                    }
                } else {
                    try {
                        unsigned int codepoint = std::stoul(entity);
                        if (codepoint < 128) result += static_cast<char>(codepoint);
                        else result += entity;
                    } catch (...) {
                        result += entity;
                    }
                }
            }

            i = end;
        } else {
            result.push_back(text[i]);
        }
    }

    return result;
}

/// Extract class attribute value from attributes string
std::string ExtractClass(const std::string &attrs) {
    size_t pos = attrs.find("class=");
    if (pos == std::string::npos) return "";

    pos += 6;
    if (pos >= attrs.size()) return "";

    char quote = attrs[pos];
    if (quote != '"' && quote != '\'') return "";

    pos++;
    size_t end = attrs.find(quote, pos);
    if (end == std::string::npos) return attrs.substr(pos);

    return attrs.substr(pos, end - pos);
}

/// Extract id attribute value from attributes string
std::string ExtractId(const std::string &attrs) {
    size_t pos = attrs.find("id=");
    if (pos == std::string::npos) return "";

    pos += 3;
    if (pos >= attrs.size()) return "";

    char quote = attrs[pos];
    if (quote != '"' && quote != '\'') return "";

    pos++;
    size_t end = attrs.find(quote, pos);
    if (end == std::string::npos) return attrs.substr(pos);

    return attrs.substr(pos, end - pos);
}

/// Convert string to lowercase
std::string ToLower(const std::string &s) {
    std::string result = s;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](char c) { return std::tolower(static_cast<unsigned char>(c)); });
    return result;
}

/// Self-closing HTML tags
const std::unordered_set<std::string> SelfClosingTags = {
    "br", "hr", "img", "meta", "link", "colophon", "area", "sourcecode"
};

/// Skip tags (by tag name)
const std::unordered_set<std::string> SkipTagNames = {
    "script", "style", "meta", "noscript", "link"
};

/// Skip ID values
const std::unordered_set<std::string> SkipIdValues = {
    "catlinks", "footer", "mw-navigation"
};

/// Skip class substrings
const std::vector<std::string> SkipClassValues = {
    "t-navbar-menu", "t-navbar-sep", "printfooter",
    "mw-editsection", "noprint", "navbox", "referencelist"
};

} // anonymous namespace

// ExtractionConfig constructor
CppReferenceExtractor::ExtractionConfig::ExtractionConfig()
    : format(CppReferenceExtractor::Format::Markdown)
    , code_fence("```")
    , preserve_links(true)
    , link_format("[{text}]({url})")
    , max_heading_level(6)
    , include_metadata(true)
{
}

// HTML Parser Implementation

std::shared_ptr<CppReferenceExtractor::HtmlNode> CppReferenceExtractor::ParseHtml(const std::string &html) {
    auto tokens = Tokenize(html);
    size_t pos = 0;
    return BuildTree(tokens, pos);
}

std::shared_ptr<CppReferenceExtractor::HtmlNode> CppReferenceExtractor::ParseHtmlFragment(const std::string &html) {
    auto tokens = Tokenize(html);
    size_t pos = 0;
    return BuildTree(tokens, pos);
}

std::vector<CppReferenceExtractor::Token> CppReferenceExtractor::Tokenize(const std::string &html) {
    std::vector<Token> tokens;
    size_t i = 0;

    while (i < html.size()) {
        if (html[i] == '<') {
            size_t end = html.find('>', i);
            if (end == std::string::npos) {
                size_t next = html.find('<', i + 1);
                if (next == std::string::npos) next = html.size();
                Token textToken;
                textToken.type = CppReferenceExtractor::Token::Type::Text;
                textToken.text = html.substr(i, next - i);
                tokens.push_back(textToken);
                i = next;
                continue;
            }

            std::string tagContent = html.substr(i + 1, end - i - 1);

            if (tagContent.substr(0, 3) == "!--") {
                Token commentToken;
                commentToken.type = CppReferenceExtractor::Token::Type::Comment;
                tokens.push_back(commentToken);
                i = end + 1;
                continue;
            }

            if (!tagContent.empty() && tagContent[0] == '/') {
                Token closeToken;
                closeToken.type = CppReferenceExtractor::Token::Type::CloseTag;
                std::string tagName = tagContent.substr(1);
                size_t spacePos = tagName.find_first_of(" \t\n\r>");
                if (spacePos != std::string::npos) {
                    tagName = tagName.substr(0, spacePos);
                }
                closeToken.tag = ToLower(tagName);
                tokens.push_back(closeToken);
                i = end + 1;
                continue;
            }

            Token openToken;
            openToken.type = CppReferenceExtractor::Token::Type::OpenTag;

            size_t spacePos = tagContent.find_first_of(" \t\n\r/>");
            if (spacePos != std::string::npos) {
                openToken.tag = ToLower(tagContent.substr(0, spacePos));
                openToken.attributes = tagContent.substr(spacePos);
            } else {
                openToken.tag = ToLower(tagContent);
                openToken.attributes = "";
            }

            tokens.push_back(openToken);
            i = end + 1;
        } else {
            size_t next = html.find('<', i);
            if (next == std::string::npos) next = html.size();

            Token textToken;
            textToken.type = CppReferenceExtractor::Token::Type::Text;
            textToken.text = DecodeHtmlEntities(html.substr(i, next - i));
            tokens.push_back(textToken);
            i = next;
        }
    }

    Token eofToken;
    eofToken.type = CppReferenceExtractor::Token::Type::Eof;
    tokens.push_back(eofToken);

    return tokens;
}

std::shared_ptr<CppReferenceExtractor::HtmlNode> CppReferenceExtractor::BuildTree(
    const std::vector<Token> &tokens, size_t &pos) {

    auto root = std::make_shared<HtmlNode>(HtmlNode::Type::Element);

    while (pos < tokens.size() && tokens[pos].type != CppReferenceExtractor::Token::Type::Eof) {
        const Token &token = tokens[pos];

        if (token.type == CppReferenceExtractor::Token::Type::Text) {
            auto textNode = std::make_shared<HtmlNode>(HtmlNode::Type::Text);
            textNode->text = token.text;
            root->children.push_back(textNode);
            pos++;
        }
        else if (token.type == CppReferenceExtractor::Token::Type::Comment) {
            auto commentNode = std::make_shared<HtmlNode>(HtmlNode::Type::Comment);
            root->children.push_back(commentNode);
            pos++;
        }
        else if (token.type == CppReferenceExtractor::Token::Type::OpenTag) {
            auto element = std::make_shared<HtmlNode>(HtmlNode::Type::Element);
            element->tag = token.tag;
            element->attributes = token.attributes;
            element->class_value = ExtractClass(token.attributes);
            element->id_value = ExtractId(token.attributes);

            bool selfClosing = false;
            if (!token.attributes.empty()) {
                size_t slashPos = token.attributes.rfind('/');
                if (slashPos != std::string::npos) {
                    selfClosing = true;
                }
            }

            if (selfClosing || SelfClosingTags.count(token.tag)) {
                root->children.push_back(element);
                pos++;
                continue;
            }

            root->children.push_back(element);
            pos++;

            auto childrenRoot = BuildTree(tokens, pos);
            element->children = childrenRoot->children;

            if (pos < tokens.size() && tokens[pos].type == CppReferenceExtractor::Token::Type::CloseTag &&
                tokens[pos].tag == token.tag) {
                pos++;
            }
        }
        else if (token.type == CppReferenceExtractor::Token::Type::CloseTag) {
            break;
        }
        else {
            pos++;
        }
    }

    return root;
}

// Extractor Implementation

CppReferenceExtractor::CppReferenceExtractor()
    : m_output(nullptr)
    , m_tableType(TableType::None)
    , m_tableColumn(0)
    , m_preDepth(0)
    , m_ddDepth(0)
{
    m_outputText.reserve(4096);
}

CppReferenceExtractor::~CppReferenceExtractor() = default;

bool CppReferenceExtractor::Convert(
    const std::filesystem::path &inputFile,
    const std::filesystem::path &outputFile,
    const ExtractionConfig &config)
{
    (void)config;

    std::ifstream inputFileStream(inputFile, std::ios::binary);
    if (!inputFileStream.is_open()) {
        return false;
    }

    std::stringstream buffer;
    buffer << inputFileStream.rdbuf();
    std::string htmlContent = buffer.str();

    auto document = ParseHtml(htmlContent);
    if (!document || document->children.empty()) {
        return false;
    }

    std::ofstream outputFileStream(outputFile);
    if (!outputFileStream.is_open()) {
        return false;
    }

    m_output = &outputFileStream;
    m_outputText.clear();
    m_tableStack.clear();
    m_tableType = TableType::None;
    m_tableColumn = 0;
    m_preDepth = 0;
    m_ddDepth = 0;

    WritePageHeader();

    const HtmlNode *contentRoot = FindContentRoot(document->children);
    if (contentRoot) {
        for (const auto &child : contentRoot->children) {
            Walk(child.get());
        }
    }

    WriteFooter();
    TrimOutput();

    outputFileStream << m_outputText;
    m_output = nullptr;

    return true;
}

bool CppReferenceExtractor::Convert(
    const std::filesystem::path &inputFile,
    std::ostream &output,
    const ExtractionConfig &config)
{
    (void)config;

    std::ifstream inputFileStream(inputFile, std::ios::binary);
    if (!inputFileStream.is_open()) {
        return false;
    }

    std::stringstream buffer;
    buffer << inputFileStream.rdbuf();
    std::string htmlContent = buffer.str();

    auto document = ParseHtml(htmlContent);
    if (!document || document->children.empty()) {
        return false;
    }

    m_output = &output;
    m_outputText.clear();
    m_tableStack.clear();
    m_tableType = TableType::None;
    m_tableColumn = 0;
    m_preDepth = 0;
    m_ddDepth = 0;

    WritePageHeader();

    const HtmlNode *contentRoot = FindContentRoot(document->children);
    if (contentRoot) {
        for (const auto &child : contentRoot->children) {
            Walk(child.get());
        }
    }

    WriteFooter();
    TrimOutput();

    if (m_output) {
        *m_output << m_outputText;
    }
    m_output = nullptr;

    return true;
}

std::string CppReferenceExtractor::ExtractFromMemory(
    const std::string &html,
    const ExtractionConfig &config)
{
    (void)config;

    auto document = ParseHtml(html);
    if (!document || document->children.empty()) {
        return "";
    }

    m_outputText.clear();
    m_tableStack.clear();
    m_tableType = TableType::None;
    m_tableColumn = 0;
    m_preDepth = 0;
    m_ddDepth = 0;

    const HtmlNode *contentRoot = FindContentRoot(document->children);
    if (contentRoot) {
        for (const auto &child : contentRoot->children) {
            Walk(child.get());
        }
    }

    TrimOutput();

    std::string result = m_outputText;
    return result;
}

std::string CppReferenceExtractor::Version() {
    return "CppReferenceParser v2.0";
}

std::string CppReferenceExtractor::Name() {
    return "C++ Reference Text Extractor";
}

// Walker and Visitor

void CppReferenceExtractor::Walk(const HtmlNode *node) {
    if (!node) return;

    switch (node->type) {
        case HtmlNode::Type::Element:
            VisitElement(node);
            for (const auto &child : node->children) {
                Walk(child.get());
            }
            LeaveElement(node);
            break;
        case HtmlNode::Type::Text:
            VisitText(node);
            break;
        case HtmlNode::Type::Comment:
            break;
    }
}

void CppReferenceExtractor::WalkChildren(const HtmlNode *node) {
    if (!node) return;
    for (const auto &child : node->children) {
        Walk(child.get());
    }
}

void CppReferenceExtractor::VisitElement(const HtmlNode *element) {
    if (!element) return;

    if (ShouldSkipElement(element)) {
        return;
    }

    const std::string &tag = element->tag;

    if (element->class_value.find("spacer") != std::string::npos) {
        EnsureNewline();
        return;
    }

    if (tag == "table") {
        m_tableStack.push_back(m_tableType);
        m_tableType = DetectTableType(element);
        m_tableColumn = 0;
        EnsureNewline();
    }
    else if (tag == "tr") {
        m_tableColumn = 0;
        EnsureNewline();
    }
    else if (tag == "th" || tag == "td") {
        if (m_tableColumn > 0) {
            Write("\t");
        }
        ++m_tableColumn;
    }
    else if (tag == "h1") {
        EnsureNewline();
    }
    else if (tag == "h2" || tag == "h3" || tag == "h4" ||
             tag == "h5" || tag == "h6") {
        EnsureNewline();
    }
    else if (tag == "p" || tag == "ul" || tag == "ol" ||
             tag == "dl" || tag == "dt") {
        EnsureNewline();
    }
    else if (tag == "dd") {
        EnsureNewline();
        Write("\t");
    }
    else if (tag == "br") {
        EnsureNewline();
    }
    else if (tag == "pre") {
        EnsureNewline();
        Write("```");
        Write(DetectCodeLanguage(element));
        AppendNewline();
        ++m_preDepth;
    }
    else if (tag == "li") {
        EnsureNewline();
        Write("    ");
    }
    else if (tag == "a") {
        // Links - write text content as-is
    }
}

void CppReferenceExtractor::VisitText(const HtmlNode *textNode) {
    if (!textNode) return;
    VisitTextRaw(textNode->text);
}

void CppReferenceExtractor::LeaveElement(const HtmlNode *element) {
    if (!element) return;

    const std::string &tag = element->tag;

    if (tag == "table") {
        if (!m_tableStack.empty()) {
            m_tableType = m_tableStack.back();
            m_tableStack.pop_back();
        } else {
            m_tableType = TableType::None;
        }
        m_tableColumn = 0;
        AppendNewline();
    }
    else if (tag == "tr" || tag == "ul" || tag == "ol" ||
             tag == "dl" || tag == "dt" || tag == "dd" || tag == "li") {
        EnsureNewline();
    }
    else if (tag == "h1") {
        AppendNewline();
        AppendNewline();
    }
    else if (tag == "h2" || tag == "h3" || tag == "h4" ||
             tag == "h5" || tag == "h6") {
        EnsureNewline();
    }
    else if (tag == "p") {
        AppendNewline();
        AppendNewline();
    }
    else if (tag == "pre") {
        if (m_preDepth > 0) --m_preDepth;
        EnsureNewline();
        Write("```");
        AppendNewline();
    }
}

bool CppReferenceExtractor::ShouldSkipElement(const HtmlNode *element) const {
    if (!element) return true;

    const std::string &tag = element->tag;

    if (SkipTagNames.count(tag)) {
        return true;
    }

    if (SkipIdValues.count(element->id_value)) {
        return true;
    }

    for (const auto &skipClass : SkipClassValues) {
        if (element->class_value.find(skipClass) != std::string::npos) {
            return true;
        }
    }

    return false;
}

bool CppReferenceExtractor::HasClass(const HtmlNode *node, std::string_view className) const {
    if (!node || node->type != HtmlNode::Type::Element) return false;

    const std::string &classes = node->class_value;
    size_t pos = 0;

    while (pos < classes.size()) {
        while (pos < classes.size() && IsWhitespace(classes[pos])) {
            ++pos;
        }

        const size_t start = pos;
        while (pos < classes.size() && !IsWhitespace(classes[pos])) {
            ++pos;
        }

        if (pos - start == className.size() &&
            classes.substr(start, pos - start) == className) {
            return true;
        }
    }

    return false;
}

CppReferenceExtractor::TableType CppReferenceExtractor::DetectTableType(const HtmlNode *node) const {
    if (!node) return TableType::None;

    if (node->class_value.find("t-dcl-begin") != std::string::npos)
        return TableType::Declaration;
    if (node->class_value.find("t-par-begin") != std::string::npos)
        return TableType::Parameters;
    if (node->class_value.find("t-dsc-begin") != std::string::npos)
        return TableType::Description;

    return TableType::Generic;
}

std::string_view CppReferenceExtractor::DetectCodeLanguage(const HtmlNode *node) const {
    (void)node;
    return "cpp";
}

void CppReferenceExtractor::WritePageHeader() {
    // Placeholder for future content extraction
}

void CppReferenceExtractor::WriteFooter() {
    // Placeholder for footer content
}

const CppReferenceExtractor::HtmlNode *CppReferenceExtractor::FindElementById(const std::string &id) const {
    (void)id;
    return nullptr;
}

const CppReferenceExtractor::HtmlNode *CppReferenceExtractor::FindContentRoot(
    const std::vector<std::shared_ptr<CppReferenceExtractor::HtmlNode>> &children) const {
    // Look for mw-content-text or body element
    for (const auto &child : children) {
        if (!child) continue;
        if (child->tag == "body") return child.get();
        if (child->id_value == "mw-content-text") return child.get();
    }

    // If no content root found, use first child
    if (!children.empty()) {
        return children[0].get();
    }

    return nullptr;
}

// Output Writers

void CppReferenceExtractor::Write(const char *text) {
    if (!text) return;
    for (const char *current = text; *current; ++current) {
        AppendRawByte(*current);
    }
}

void CppReferenceExtractor::Write(std::string_view text) {
    for (char ch : text) {
        AppendRawByte(ch);
    }
}

void CppReferenceExtractor::VisitTextRaw(const std::string &text) {
    for (size_t i = 0; i < text.size(); ++i) {
        char ch = text[i];

        if (ch == '\xA0') {
            AppendTextSpace();
            continue;
        }

        // Handle zero-width space (UTF-8: \xE2\x80\x8B)
        if (ch == '\xE2' && i + 2 < text.size() &&
            static_cast<unsigned char>(text[i + 1]) == 0x80 &&
            static_cast<unsigned char>(text[i + 2]) == 0x8B) {
            i += 2;
            continue;
        }

        if (IsWhitespace(static_cast<unsigned char>(ch))) {
            AppendTextSpace();
            continue;
        }

        AppendRawByte(ch);
    }
}

// Buffer Utilities

void CppReferenceExtractor::AppendRawByte(char ch) {
    if (ch == '\r') return;
    if (ch == '\n') { AppendNewline(); return; }
    if (ch == '\t') { TrimTrailingInlineWhitespace(); m_outputText.push_back('\t'); return; }

    m_outputText.push_back(ch);
}

void CppReferenceExtractor::AppendTextSpace() {
    if (m_outputText.empty()) return;

    const char last = m_outputText.back();
    if (last == '\n' || last == ' ' || last == '\t') return;

    m_outputText.push_back(' ');
}

void CppReferenceExtractor::EnsureNewline() {
    TrimTrailingInlineWhitespace();
    if (m_outputText.empty() || m_outputText.back() == '\n') return;
    m_outputText.push_back('\n');
}

void CppReferenceExtractor::AppendNewline() {
    TrimTrailingInlineWhitespace();
    if (m_outputText.empty()) return;

    size_t newlineCount = 0;
    for (size_t i = m_outputText.size(); i > 0; --i) {
        if (m_outputText[i - 1] != '\n') break;
        ++newlineCount;
    }

    if (newlineCount < 2) {
        m_outputText.push_back('\n');
    }
}

void CppReferenceExtractor::TrimTrailingInlineWhitespace() {
    while (!m_outputText.empty()) {
        const char last = m_outputText.back();
        if (last != ' ' && last != '\t') break;
        m_outputText.pop_back();
    }
}

void CppReferenceExtractor::TrimOutput() {
    TrimTrailingInlineWhitespace();

    while (!m_outputText.empty() && m_outputText.back() == '\n') {
        m_outputText.pop_back();
    }

    while (!m_outputText.empty() &&
           (m_outputText.front() == '\n' || m_outputText.front() == ' ' || m_outputText.front() == '\t')) {
        m_outputText.erase(m_outputText.begin());
    }

    m_outputText.push_back('\n');
}

// HTML Helpers

std::string CppReferenceExtractor::ExtractClass(const std::string &attributes) {
    return ::ExtractClass(attributes);
}

std::string CppReferenceExtractor::ExtractId(const std::string &attributes) {
    return ::ExtractId(attributes);
}

std::string CppReferenceExtractor::DecodeHtmlEntities(const std::string &text) {
    return ::DecodeHtmlEntities(text);
}

std::string CppReferenceExtractor::ToLower(const std::string &s) {
    return ::ToLower(s);
}