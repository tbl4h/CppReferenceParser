#include "CppReferenceExtractor.hpp"

#include <cstddef>
#include <cstring>
#include <sstream>

#include <lexbor/dom/interfaces/character_data.h>
#include <lexbor/dom/interfaces/text.h>
#include <lexbor/html/interfaces/document.h>
#include <lexbor/tag/tag.h>

namespace
{
bool IsAsciiWhitespace(unsigned char ch)
{
    return ch == ' ' ||
           ch == '\n' ||
           ch == '\r' ||
           ch == '\t' ||
           ch == '\f' ||
           ch == '\v';
}

bool IsNbsp(const lxb_char_t *text, size_t length, size_t index)
{
    return index + 1 < length &&
           text[index] == 0xC2 &&
           text[index + 1] == 0xA0;
}

bool IsZeroWidthSpace(const lxb_char_t *text, size_t length, size_t index)
{
    return index + 2 < length &&
           text[index] == 0xE2 &&
           text[index + 1] == 0x80 &&
           text[index + 2] == 0x8B;
}

lxb_dom_element_t *FindFirstElementByTag(lxb_dom_node_t *node, lxb_tag_id_t tagId)
{
    if (node == nullptr)
    {
        return nullptr;
    }

    if (node->type == LXB_DOM_NODE_TYPE_ELEMENT)
    {
        lxb_dom_element_t *element =
            lxb_dom_interface_element(node);

        if (lxb_dom_element_tag_id(element) == tagId)
        {
            return element;
        }
    }

    for (lxb_dom_node_t *child = node->first_child;
         child != nullptr;
         child = child->next)
    {
        lxb_dom_element_t *match =
            FindFirstElementByTag(child, tagId);

        if (match != nullptr)
        {
            return match;
        }
    }

    return nullptr;
}
}

CppReferenceExtractor::CppReferenceExtractor()
    : m_document(nullptr)
{
}

CppReferenceExtractor::~CppReferenceExtractor()
{
    if (m_document != nullptr)
    {
        lxb_html_document_destroy(m_document);
        m_document = nullptr;
    }
}

bool CppReferenceExtractor::Convert(
    const std::filesystem::path &inputFile,
    const std::filesystem::path &outputFile)
{
    if (!LoadHtml(inputFile))
    {
        return false;
    }

    m_output.open(outputFile);

    if (!m_output)
    {
        return false;
    }

    if (m_document == nullptr)
    {
        return false;
    }

    lxb_dom_element_t *content = FindContentRoot();

    if (content == nullptr)
    {
        return false;
    }

    m_outputText.clear();
    m_tableStack.clear();
    m_tableType = TableType::None;
    m_tableColumn = 0;
    m_preDepth = 0;

    WritePageHeader();
    Walk(lxb_dom_interface_node(content));
    WriteFooter();
    TrimOutput();

    m_output << m_outputText;

    return static_cast<bool>(m_output);
}

bool CppReferenceExtractor::LoadHtml(
    const std::filesystem::path &file)
{
    std::ifstream input(file, std::ios::binary);

    if (!input.is_open())
    {
        return false;
    }

    std::stringstream buffer;
    buffer << input.rdbuf();

    std::string html = buffer.str();

    if (html.empty())
    {
        return false;
    }

    if (m_document != nullptr)
    {
        lxb_html_document_destroy(m_document);
        m_document = nullptr;
    }

    lxb_html_parser_t *parser =
        lxb_html_parser_create();

    if (parser == nullptr)
    {
        return false;
    }

    lxb_status_t status =
        lxb_html_parser_init(parser);

    if (status != LXB_STATUS_OK)
    {
        lxb_html_parser_destroy(parser);

        return false;
    }

    m_document =
        lxb_html_parse(
            parser,
            reinterpret_cast<const lxb_char_t *>(html.data()),
            html.size());

    lxb_html_parser_destroy(parser);

    return m_document != nullptr;
}

void CppReferenceExtractor::Walk(lxb_dom_node_t *node)
{
    if (node == nullptr)
    {
        return;
    }

    switch (node->type)
    {
    case LXB_DOM_NODE_TYPE_ELEMENT:
    {
        lxb_dom_element_t *element =
            lxb_dom_interface_element(node);

        if (!VisitElement(element))
        {
            return;
        }

        WalkChildren(node);
        LeaveElement(element);
        return;
    }

    case LXB_DOM_NODE_TYPE_TEXT:
    {
        VisitText(lxb_dom_interface_text(node));
        return;
    }

    default:
        break;
    }

    WalkChildren(node);
}

void CppReferenceExtractor::WalkChildren(lxb_dom_node_t *node)
{
    if (node == nullptr)
    {
        return;
    }

    for (lxb_dom_node_t *child = node->first_child;
         child != nullptr;
         child = child->next)
    {
        Walk(child);
    }
}

bool CppReferenceExtractor::VisitElement(
    lxb_dom_element_t *element)
{
    if (ShouldSkipElement(element))
    {
        return false;
    }

    if (HasClass(element, "spacer"))
    {
        EnsureNewline();
        return false;
    }

    if (HasClass(element, "t-navbar-head") ||
        HasClass(element, "t-li1") ||
        HasClass(element, "t-example-live-link") ||
        HasClass(element, "coliru-btn"))
    {
        EnsureNewline();
    }

    if (m_tableType == TableType::Description &&
        m_tableColumn == 1 &&
        HasClass(element, "t-lines"))
    {
        EnsureNewline();
    }

    switch (lxb_dom_element_tag_id(element))
    {
    case LXB_TAG_TABLE:
        m_tableStack.push_back(m_tableType);
        m_tableType = DetectTableType(element);
        m_tableColumn = 0;
        EnsureNewline();
        break;

    case LXB_TAG_TR:
        m_tableColumn = 0;
        EnsureNewline();
        break;

    case LXB_TAG_TH:
    case LXB_TAG_TD:
        if (m_tableColumn > 0)
        {
            if ((m_tableType == TableType::Declaration ||
                 m_tableType == TableType::Description) &&
                m_tableColumn == 1)
            {
                EnsureNewline();
                Write("\t");
            }
            else
            {
                Write("\t");
            }
        }

        ++m_tableColumn;
        break;

    case LXB_TAG_H1:
    case LXB_TAG_H2:
    case LXB_TAG_H3:
    case LXB_TAG_H4:
    case LXB_TAG_H5:
    case LXB_TAG_H6:
        EnsureNewline();
        break;

    case LXB_TAG_P:
    case LXB_TAG_UL:
    case LXB_TAG_OL:
    case LXB_TAG_DL:
    case LXB_TAG_DT:
        EnsureNewline();
        break;

    case LXB_TAG_DD:
        EnsureNewline();
        Write("\t");
        break;

    case LXB_TAG_BR:
        EnsureNewline();
        break;

    case LXB_TAG_PRE:
        EnsureNewline();
        ++m_preDepth;
        break;

    case LXB_TAG_LI:
        EnsureNewline();
        Write("    ");
        break;

    default:
        break;
    }

    return true;
}

void CppReferenceExtractor::VisitText(
    lxb_dom_text_t *text)
{
    if (text == nullptr)
    {
        return;
    }

    const lexbor_str_t &str = text->char_data.data;

    WriteText(str.data, str.length);
}

void CppReferenceExtractor::LeaveElement(
    lxb_dom_element_t *element)
{
    switch (lxb_dom_element_tag_id(element))
    {
    case LXB_TAG_TABLE:
        if (!m_tableStack.empty())
        {
            m_tableType = m_tableStack.back();
            m_tableStack.pop_back();
        }
        else
        {
            m_tableType = TableType::None;
        }

        m_tableColumn = 0;
        AppendNewline();
        break;

    case LXB_TAG_TR:
    case LXB_TAG_UL:
    case LXB_TAG_OL:
    case LXB_TAG_DL:
    case LXB_TAG_DT:
    case LXB_TAG_DD:
    case LXB_TAG_LI:
        EnsureNewline();
        break;

    case LXB_TAG_H1:
        AppendNewline();
        AppendNewline();
        break;

    case LXB_TAG_H2:
    case LXB_TAG_H3:
    case LXB_TAG_H4:
    case LXB_TAG_H5:
    case LXB_TAG_H6:
        EnsureNewline();
        break;

    case LXB_TAG_P:
        AppendNewline();
        AppendNewline();
        break;

    case LXB_TAG_PRE:
        if (m_preDepth > 0)
        {
            --m_preDepth;
        }

        AppendNewline();
        break;

    default:
        break;
    }

    if (HasClass(element, "t-navbar-head") ||
        HasClass(element, "t-li1") ||
        HasClass(element, "t-example-live-link") ||
        HasClass(element, "coliru-btn"))
    {
        EnsureNewline();
    }

    if (m_tableType == TableType::Description &&
        m_tableColumn == 1 &&
        HasClass(element, "t-lines"))
    {
        EnsureNewline();
    }
}

bool CppReferenceExtractor::ShouldSkipElement(
    lxb_dom_element_t *element)
{
    switch (lxb_dom_element_tag_id(element))
    {
    case LXB_TAG_SCRIPT:
    case LXB_TAG_STYLE:
    case LXB_TAG_META:
    case LXB_TAG_LINK:
    case LXB_TAG_NOSCRIPT:
        return true;

    default:
        break;
    }

    size_t idLength = 0;

    const lxb_char_t *id =
        lxb_dom_element_id(element, &idLength);

    if (id != nullptr)
    {
        std::string_view idView(
            reinterpret_cast<const char *>(id),
            idLength);

        if (idView == "catlinks" ||
            idView == "footer" ||
            idView == "mw-navigation")
        {
            return true;
        }
    }

    if (HasClass(element, "t-navbar-menu") ||
        HasClass(element, "t-navbar-sep") ||
        HasClass(element, "printfooter") ||
        HasClass(element, "mw-editsection") ||
        HasClass(element, "noprint"))
    {
        return true;
    }

    return false;
}

bool CppReferenceExtractor::HasClass(
    lxb_dom_element_t *element,
    std::string_view className) const
{
    if (element == nullptr)
    {
        return false;
    }

    size_t length = 0;

    const lxb_char_t *classData =
        lxb_dom_element_class(element, &length);

    if (classData == nullptr || length == 0)
    {
        return false;
    }

    std::string_view classes(
        reinterpret_cast<const char *>(classData),
        length);

    size_t position = 0;

    while (position < classes.size())
    {
        while (position < classes.size() &&
               IsAsciiWhitespace(static_cast<unsigned char>(classes[position])))
        {
            ++position;
        }

        const size_t start = position;

        while (position < classes.size() &&
               !IsAsciiWhitespace(static_cast<unsigned char>(classes[position])))
        {
            ++position;
        }

        if (classes.substr(start, position - start) == className)
        {
            return true;
        }
    }

    return false;
}

CppReferenceExtractor::TableType CppReferenceExtractor::DetectTableType(
    lxb_dom_element_t *element) const
{
    if (HasClass(element, "t-dcl-begin"))
    {
        return TableType::Declaration;
    }

    if (HasClass(element, "t-par-begin"))
    {
        return TableType::Parameters;
    }

    if (HasClass(element, "t-dsc-begin"))
    {
        return TableType::Description;
    }

    return TableType::Generic;
}

void CppReferenceExtractor::WritePageHeader()
{
    lxb_dom_element_t *heading = FindElementById("firstHeading");

    if (heading != nullptr)
    {
        Walk(lxb_dom_interface_node(heading));
    }
}

void CppReferenceExtractor::WriteFooter()
{
    lxb_dom_element_t *navigation = FindElementById("cpp-navigation");

    if (navigation != nullptr)
    {
        for (lxb_dom_node_t *child = lxb_dom_interface_node(navigation)->first_child;
             child != nullptr;
             child = child->next)
        {
            if (child->type != LXB_DOM_NODE_TYPE_ELEMENT)
            {
                continue;
            }

            lxb_dom_element_t *element =
                lxb_dom_interface_element(child);

            if (lxb_dom_element_tag_id(element) == LXB_TAG_UL)
            {
                Walk(child);
            }
        }
    }

    lxb_dom_element_t *lastModified =
        FindElementById("footer-info-lastmod");

    if (lastModified != nullptr)
    {
        Walk(lxb_dom_interface_node(lastModified));
    }
}

lxb_dom_element_t *CppReferenceExtractor::FindElementById(const char *id)
{
    if (m_document == nullptr || id == nullptr)
    {
        return nullptr;
    }

    lxb_dom_element_t *root =
        lxb_dom_interface_element(m_document);

    return lxb_dom_element_by_id(
        root,
        reinterpret_cast<const lxb_char_t *>(id),
        std::strlen(id));
}

lxb_dom_element_t *CppReferenceExtractor::FindContentRoot()
{
    lxb_dom_element_t *content = FindElementById("mw-content-text");

    if (content != nullptr)
    {
        return content;
    }

    if (m_document == nullptr)
    {
        return nullptr;
    }

    return FindFirstElementByTag(
        lxb_dom_interface_node(m_document),
        LXB_TAG_BODY);
}

void CppReferenceExtractor::Write(const char *text)
{
    if (text == nullptr)
    {
        return;
    }

    for (const char *current = text; *current != '\0'; ++current)
    {
        AppendRawByte(*current);
    }
}

void CppReferenceExtractor::Write(
    const lxb_char_t *text,
    size_t length)
{
    if (text == nullptr || length == 0)
    {
        return;
    }

    for (size_t index = 0; index < length; ++index)
    {
        AppendRawByte(static_cast<char>(text[index]));
    }
}

void CppReferenceExtractor::WriteText(
    const lxb_char_t *text,
    size_t length)
{
    if (text == nullptr || length == 0)
    {
        return;
    }

    for (size_t index = 0; index < length; ++index)
    {
        if (IsNbsp(text, length, index))
        {
            if (m_preDepth > 0)
            {
                AppendRawByte(' ');
            }
            else
            {
                AppendTextSpace();
            }

            ++index;
            continue;
        }

        if (IsZeroWidthSpace(text, length, index))
        {
            index += 2;
            continue;
        }

        const unsigned char ch = text[index];

        if (m_preDepth == 0 && IsAsciiWhitespace(ch))
        {
            AppendTextSpace();
            continue;
        }

        AppendRawByte(static_cast<char>(ch));
    }
}

void CppReferenceExtractor::AppendRawByte(char ch)
{
    if (ch == '\r')
    {
        return;
    }

    if (ch == '\n')
    {
        AppendNewline();
        return;
    }

    if (ch == '\t')
    {
        TrimTrailingInlineWhitespace();
        m_outputText.push_back('\t');
        return;
    }

    m_outputText.push_back(ch);
}

void CppReferenceExtractor::AppendTextSpace()
{
    if (m_outputText.empty())
    {
        return;
    }

    const char last = m_outputText.back();

    if (last == '\n' ||
        last == ' ' ||
        last == '\t')
    {
        return;
    }

    m_outputText.push_back(' ');
}

void CppReferenceExtractor::EnsureNewline()
{
    TrimTrailingInlineWhitespace();

    if (m_outputText.empty() ||
        m_outputText.back() == '\n')
    {
        return;
    }

    m_outputText.push_back('\n');
}

void CppReferenceExtractor::AppendNewline()
{
    TrimTrailingInlineWhitespace();

    if (m_outputText.empty())
    {
        return;
    }

    size_t newlineCount = 0;

    for (size_t index = m_outputText.size(); index > 0; --index)
    {
        if (m_outputText[index - 1] != '\n')
        {
            break;
        }

        ++newlineCount;
    }

    if (newlineCount < 2)
    {
        m_outputText.push_back('\n');
    }
}

void CppReferenceExtractor::TrimTrailingInlineWhitespace()
{
    while (!m_outputText.empty())
    {
        const char last = m_outputText.back();

        if (last != ' ' && last != '\t')
        {
            break;
        }

        m_outputText.pop_back();
    }
}

void CppReferenceExtractor::TrimOutput()
{
    TrimTrailingInlineWhitespace();

    while (!m_outputText.empty() &&
           m_outputText.back() == '\n')
    {
        m_outputText.pop_back();
    }

    while (!m_outputText.empty() &&
           (m_outputText.front() == '\n' ||
            m_outputText.front() == ' ' ||
            m_outputText.front() == '\t'))
    {
        m_outputText.erase(m_outputText.begin());
    }

    m_outputText.push_back('\n');
}

void CppReferenceExtractor::WriteAttribute(
    lxb_dom_attr_t *attr)
{
    if (attr == nullptr)
    {
        return;
    }

    size_t nameLength = 0;
    size_t valueLength = 0;

    const lxb_char_t *name =
        lxb_dom_attr_local_name(attr, &nameLength);

    const lxb_char_t *value =
        lxb_dom_attr_value(attr, &valueLength);

    Write(name, nameLength);
    Write("=\"");
    Write(value, valueLength);
    Write("\"");
}
