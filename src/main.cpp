#include "antlr4-runtime.h"
#include "JBLangLexer.h"
#include "JBLangParser.h"
#include "jblang/ast/TranspilerVisitor.h"
#include "jblang/codegen/CCodeGenerator.h"
#include <iostream>
#include <string>
#include <cstdlib>
#include <filesystem>

bool fileExists(const std::string& path)
{
    std::ifstream f(path.c_str());
    return f.good();
}

std::string getBaseFileName(const std::string& path)
{
    std::filesystem::path p(path);
    return p.stem().string();
}

bool compileCode(const std::string& cFilePath, const std::string& outputPath,
        const std::string& allocatorType = "simple", bool debug = false)
{
    std::string cleanRuntime = "cd ../runtime && make clean";
    std::string buildRuntime = "cd ../runtime && make "+allocatorType;
    if (debug) {
        buildRuntime += " DEBUG=1";
    }
    std::string compileCommand = "cd ../build && gcc -o "+outputPath+" "+
            cFilePath+" "+
            "../runtime/lib/libjblang_runtime.a "+
            "-I ../runtime/include";

    std::cout << "Cleaning runtime..." << std::endl;
    if (system(cleanRuntime.c_str())!=0) {
        std::cerr << "Failed to clean runtime" << std::endl;
        return false;
    }

    std::cout << "Building runtime with " << allocatorType << " allocator";
    if (debug) std::cout << " (debug mode)";
    std::cout << "..." << std::endl;

    if (system(buildRuntime.c_str())!=0) {
        std::cerr << "Failed to build runtime" << std::endl;
        return false;
    }

    std::cout << "Compiling with command: " << compileCommand << std::endl;
    return system(compileCommand.c_str())==0;
}

void printUsage(const char* programName)
{
    std::cerr << "Usage: " << programName << " <input-file> -o <output-name> [-a <allocator>] [--debug]\n";
    std::cerr << "Allocators: simple, reference_count, mark_sweep\n";
}

int main(int argc, char* argv[])
{
    if (argc<4) {
        printUsage(argv[0]);
        return 1;
    }

    std::string inputFile = argv[1];
    std::string outputName;
    std::string allocatorType = "reference_count";
    bool debug = false;

    for (int i = 2; i<argc; i++) {
        if (std::string(argv[i])=="-o" && i+1<argc) {
            outputName = argv[i+1];
            i++;
        }
        else if (std::string(argv[i])=="-a" && i+1<argc) {
            allocatorType = argv[i+1];
            i++;
        }
        else if (std::string(argv[i])=="--debug") {
            debug = true;
        }
    }

    if (outputName.empty()) {
        std::cerr << "Error: Output name not specified\n";
        printUsage(argv[0]);
        return 1;
    }

    if (allocatorType!="simple" && allocatorType!="reference_count" && allocatorType!="mark_sweep") {
        std::cerr << "Error: Invalid allocator type: " << allocatorType << std::endl;
        printUsage(argv[0]);
        return 1;
    }

    std::string cFilePath = outputName+".c";
    std::string executablePath = outputName;

    std::cout << "Using allocator: " << allocatorType << std::endl;

    try {
        if (!fileExists(inputFile)) {
            throw std::runtime_error("Input file does not exist: "+inputFile);
        }

        if (!fileExists("../runtime/include/runtime.h")) {
            throw std::runtime_error("Runtime files not found in runtime/ directory");
        }

        std::ifstream stream;
        stream.open(inputFile);
        antlr4::ANTLRInputStream input(stream);

        JBLangLexer lexer(&input);
        antlr4::CommonTokenStream tokens(&lexer);
        JBLangParser parser(&tokens);

        bool useRefCount = (allocatorType=="reference_count");

        auto* tree = parser.program();
        std::unique_ptr<CodeGenerator> generator = std::make_unique<CCodeGenerator>(useRefCount);
        TranspilerVisitor visitor(std::move(generator));
        auto cCode = std::any_cast<std::string>(visitor.visitProgram(tree));

        std::ofstream outFile(cFilePath);
        if (!outFile) {
            throw std::runtime_error("Failed to create output C file: "+cFilePath);
        }
        outFile << cCode;
        outFile.close();

        std::cout << "Successfully generated C code: " << cFilePath << std::endl;

        if (!compileCode(cFilePath, executablePath, allocatorType, debug)) {
            throw std::runtime_error("Compilation failed");
        }

        std::cout << "Successfully compiled executable: " << executablePath << std::endl;
        return 0;

    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
