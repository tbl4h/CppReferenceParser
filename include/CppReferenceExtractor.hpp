#pragma once

#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

#include <lexbor/html/parser.h>
#include <lexbor/html/html.h>
#include <lexbor/dom/interfaces/document.h>
#include <lexbor/dom/interfaces/node.h>

class CppReferenceExtractor
{
public:
    CppReferenceExtractor();
    ~CppReferenceExtractor();

    bool Convert(const std::filesystem::path &inputFile,
                 const std::filesystem::path &outputFile);

private:
    lxb_html_document_t *m_document;

    std::ofstream m_output;
    std::string m_outputText;

    enum class TableType
    {
        None,
        Declaration,
        Parameters,
        Description,
        Generic
    };

    bool LoadHtml(const std::filesystem::path &file);

    void Walk(lxb_dom_node_t *node);
    void WalkChildren(lxb_dom_node_t *node);

    bool VisitElement(lxb_dom_element_t *element);
    void VisitText(lxb_dom_text_t *text);
    void LeaveElement(lxb_dom_element_t *element);

    bool ShouldSkipElement(lxb_dom_element_t *element);
    bool HasClass(lxb_dom_element_t *element, std::string_view className) const;
    TableType DetectTableType(lxb_dom_element_t *element) const;
    std::string_view DetectCodeLanguage(lxb_dom_element_t *element) const;

    void WritePageHeader();
    void WriteFooter();
    lxb_dom_element_t *FindElementById(const char *id);
    lxb_dom_element_t *FindContentRoot();

    void Write(const char *text);
    void Write(const lxb_char_t *text, size_t length);
    void WriteText(const lxb_char_t *text, size_t length);

    void AppendRawByte(char ch);
    void AppendTextSpace();
    void EnsureNewline();
    void AppendNewline();
    void TrimTrailingInlineWhitespace();
    void TrimOutput();

    void WriteAttribute(lxb_dom_attr_t *attr);

    TableType m_tableType = TableType::None;
    std::vector<TableType> m_tableStack;
    int m_tableColumn = 0;
    int m_preDepth = 0;
};
