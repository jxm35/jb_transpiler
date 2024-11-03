// main.cpp
#include "antlr4-runtime.h"
#include "JBLangLexer.h"
#include "JBLangParser.h"
#include "TranspilerVisitor.h"
#include "CCodeGenerator.h"
#include <iostream>
#include <string>
#include <cstdlib>
#include <filesystem>

// Helper function to check if a file exists
bool fileExists(const std::string& path) {
    std::ifstream f(path.c_str());
    return f.good();
}

// Helper function to get file name without extension
std::string getBaseFileName(const std::string& path) {
    std::filesystem::path p(path);
    return p.stem().string();
}

bool compileCode(const std::string& cFilePath, const std::string& outputPath,
                 const std::string& allocatorType = "simple") {
    std::string allocatorFlag;
    if (allocatorType == "mark_sweep") {
        allocatorFlag = "-DUSE_MARK_SWEEP";
    } else if (allocatorType == "reference_count") {
        allocatorFlag = "-DUSE_REF_COUNT";
    }

    std::string command = "gcc -o " + outputPath + " " +
                          cFilePath + " " +
                          "../src/runtime/runtime.c " +
                          "../src/runtime/allocator_impl.c " +
                          "../src/runtime/" + allocatorType + "_allocator.c " +
                          allocatorFlag + " " +
                          "-I ../src/runtime";

    return system(command.c_str()) == 0;
}

void printUsage(const char* programName) {
    std::cerr << "Usage: " << programName << " <input-file> -o <output-name>\n";
}

int main(int argc, char* argv[]) {
    if (argc != 4 || std::string(argv[2]) != "-o") {
        printUsage(argv[0]);
        return 1;
    }

    std::string inputFile = argv[1];
    std::string outputName = argv[3];
    std::string cFilePath = outputName + ".c";
    std::string executablePath = outputName;

    try {
        if (!fileExists(inputFile)) {
            throw std::runtime_error("Input file does not exist: " + inputFile);
        }

        if (!fileExists("../src/runtime/runtime.h") || !fileExists("../src/runtime/runtime.c")) {
            throw std::runtime_error("Runtime files not found in ./runtime directory");
        }

        // Setup ANTLR
        std::ifstream stream;
        stream.open(inputFile);
        antlr4::ANTLRInputStream input(stream);

        // Create lexer and parser
        JBLangLexer lexer(&input);
        antlr4::CommonTokenStream tokens(&lexer);
        JBLangParser parser(&tokens);

        // Parse and visit
        auto* tree = parser.program();
        std::unique_ptr<CodeGenerator> generator = std::make_unique<CCodeGenerator>();
        TranspilerVisitor visitor(std::move(generator));
        std::string cCode = std::any_cast<std::string>(visitor.visitProgram(tree));

        std::ofstream outFile(cFilePath);
        if (!outFile) {
            throw std::runtime_error("Failed to create output C file: " + cFilePath);
        }
        outFile << cCode;
        outFile.close();

        std::cout << "Successfully generated C code: " << cFilePath << std::endl;

        if (!compileCode(cFilePath, executablePath)) {
            throw std::runtime_error("Compilation failed");
        }

        std::cout << "Successfully compiled executable: " << executablePath << std::endl;
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}