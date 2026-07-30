#include "CppReferenceExtractor.hpp"

#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <vector>
#include <chrono>
#include <iomanip>

namespace fs = std::filesystem;

/// Display usage information
void ShowUsage(const char* programName)
{
    std::cout
        << "\n"
        << "CppReferenceExtractor - C++ Reference Text Extractor\n"
        << "Version: " << CppReferenceExtractor::Version() << "\n"
        << "\n"
        << "Usage:\n"
        << "  " << programName << " <input.html> <output.md>\n"
        << "  " << programName << " --batch <input_dir> <output_dir>\n"
        << "  " << programName << " --memory '<html_content>'\n"
        << "  " << programName << " --version\n"
        << "\n"
        << "Arguments:\n"
        << "  <input.html>    Path to input HTML file\n"
        << "  <output.md>     Path to output text file\n"
        << "  <input_dir>     Directory containing HTML files\n"
        << "  <output_dir>    Directory for output files\n"
        << "\n"
        << "Examples:\n"
        << "  " << programName << " page.html output.txt\n"
        << "  " << programName << " --batch pages/ output/\n"
        << "  " << programName << " --version\n"
        << "\n";
}

/// Process a single file conversion
bool ProcessSingleFile(
    const fs::path &inputFile,
    const fs::path &outputFile,
    const CppReferenceExtractor::ExtractionConfig &config)
{
    CppReferenceExtractor extractor;

    if (!extractor.Convert(inputFile, outputFile, config))
    {
        std::cerr << "Error: Failed to convert '" << inputFile << "'\n";
        return false;
    }

    return true;
}

/// Process batch file conversions
int ProcessBatch(
    const fs::path &inputDir,
    const fs::path &outputDir,
    const CppReferenceExtractor::ExtractionConfig &config)
{
    std::error_code ec;
    auto fileCount = std::distance(fs::directory_iterator(inputDir, ec), fs::directory_iterator{});
    if (ec || fileCount == 0)
    {
        std::cerr << "Error: Directory '" << inputDir << "' is empty or inaccessible\n";
        return 1;
    }

    int successCount = 0;
    int failCount = 0;

    for (const auto &entry : fs::directory_iterator(inputDir, ec))
    {
        if (ec) continue;
        if (!fs::is_regular_file(entry.path(), ec)) continue;

        const auto &file = entry.path();
        if (!fs::is_regular_file(file, ec)) continue;
        if (file.extension() != ".html" && file.extension() != ".htm") continue;

        fs::path outPath = outputDir / file.filename();
        outPath.replace_extension(".txt");

        auto startTime = std::chrono::steady_clock::now();
        bool success = ProcessSingleFile(file, outPath, config);
        auto endTime = std::chrono::steady_clock::now();

        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            endTime - startTime).count();

        if (success)
        {
            std::cout << "OK: " << file.filename()
                      << " -> " << outPath.filename()
                      << " (" << duration << "ms)\n";
            ++successCount;
        }
        else
        {
            std::cerr << "FAIL: " << file.filename() << " (" << duration << "ms)\n";
            ++failCount;
        }
    }

    std::cout
        << "\nBatch complete: " << successCount << " succeeded, "
        << failCount << " failed out of " << fileCount << " files\n";

    return (failCount > 0) ? 1 : 0;
}

/// Process in-memory HTML extraction
int ProcessMemory(const std::string &htmlContent)
{
    CppReferenceExtractor extractor;

    auto result = extractor.ExtractFromMemory(htmlContent);

    if (result.empty())
    {
        std::cerr << "Error: Extraction failed\n";
        return 1;
    }

    std::cout << result;
    return 0;
}

/// Parse command-line arguments
int ParseArgs(int argc, char* argv[])
{
    if (argc < 3)
    {
        ShowUsage(argv[0]);
        return 1;
    }

    std::vector<std::string> args(argv + 1, argv + argc);

    // Check for version flag
    if (args[0] == "--version")
    {
        std::cout << CppReferenceExtractor::Version() << "\n";
        return 0;
    }

    // Check for batch mode
    if (args[0] == "--batch")
    {
        if (args.size() < 3)
        {
            ShowUsage(argv[0]);
            return 1;
        }

        const fs::path inputDir = args[1];
        const fs::path outputDir = args[2];

        // Default config
        CppReferenceExtractor::ExtractionConfig config;

        return ProcessBatch(inputDir, outputDir, config);
    }

    // Check for memory mode
    if (args[0] == "--memory")
    {
        if (args.size() < 2)
        {
            ShowUsage(argv[0]);
            return 1;
        }

        return ProcessMemory(args[1]);
    }

    // Standard single file mode
    const fs::path inputFile = args[0];
    const fs::path outputFile = args[1];

    // Default config
    CppReferenceExtractor::ExtractionConfig config;

    if (!ProcessSingleFile(inputFile, outputFile, config))
    {
        return 1;
    }

    std::cout << "Conversion complete.\n";
    return 0;
}

int main(int argc, char* argv[])
{
    auto result = ParseArgs(argc, argv);
    return result;
}