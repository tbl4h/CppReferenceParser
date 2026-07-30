# CppReferenceExtractor

A C++ tool that converts C++ Reference documentation (HTML format) to Markdown format.

## Features

- Extracts structured text from cppreference HTML pages
- Converts headings, paragraphs, code blocks, and tables
- Handles HTML entities (e.g., `&`, `<`, `>`)
- Supports batch processing of directories
- Built-in HTML parser (no external dependencies)

## Prerequisites

- C++20 compiler (g++ 10.0 or higher)
- CMake (optional, for CMake-based builds)

## Building

### Using g++ directly:

```bash
g++ -std=c++20 -Wall -o CppReferenceExtractor \
    src/main.cpp src/CppReferenceExtractor.cpp -I include
```

### Using CMake (if CMakeLists.txt is available):

```bash
cmake_configure source_dir=. build_dir=build
cmake_build build_dir=build
```

## Usage

### Single file conversion:

```bash
./CppReferenceExtractor <input.html> <output.txt>
```

Example:
```bash
./CppReferenceExtractor accumulate.html output.md
```

### Batch directory conversion:

```bash
./CppReferenceExtractor --batch <input_dir> <output_dir>
```

Example:
```bash
./CppReferenceExtractor --batch html_pages/ markdown_output/
```

### In-memory extraction:

```bash
./CppReferenceExtractor --memory '<html_content>'
```

### Version information:

```bash
./CppReferenceExtractor --version
```

## Program Flow

1. **HTML Parsing**: The program tokenizes the input HTML into tags and text nodes
2. **Tree Building**: Tokens are assembled into an HTML AST (Abstract Syntax Tree)
3. **Walking**: The AST is traversed using a visitor pattern
4. **Text Extraction**: Relevant content is extracted and formatted as Markdown
5. **Output**: The result is written to the output file

## Supported HTML Elements

| Element | Output |
|---------|--------|
| `<h1>` | Heading with newline |
| `<h2>`-`<h6>` | Headings with proper nesting |
| `<p>` | Paragraph with double newline |
| `<pre>` | Code block with markdown fence |
| `<ul>`, `<ol>` | List items |
| `<table>` | Tabular data |
| `<a>` | Text content (links processed) |
| HTML entities | Decoded (e.g., `&` → `&`) |

## Skipped Elements

The following elements are intentionally skipped:
- `<script>`, `<style>`, `<meta>`, `<noscript>`
- Elements with IDs: `catlinks`, `footer`, `mw-navigation`
- Elements with classes: `t-navbar-menu`, `noprint`, `navbox`, etc.

## Configuration

The `ExtractionConfig` struct allows customization:

```cpp
CppReferenceExtractor::ExtractionConfig config;
config.format = CppReferenceExtractor::Format::Markdown;  // or PlainText, ReStructuredText
config.preserve_links = true;
config.link_format = "[{text}]({url})";
config.max_heading_level = 6;
config.include_metadata = true;
config.skip_classes = {"t-navbar-menu", "noprint"};
```

## Project Structure

```
CppReferenceParser/
├── include/
│   ├── config.hpp          # Global configuration
│   └── CppReferenceExtractor.hpp  # Main extractor header
├── src/
│   ├── main.cpp            # Entry point & CLI
│   └── CppReferenceExtractor.cpp  # Implementation
├── test/
│   ├── test.html           # Sample HTML file
│   └── test_main.cpp       # Test runner
└── CMakeLists.txt          # CMake build configuration
```

## License

See the `LICENSE` file for details.