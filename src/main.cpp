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
        const std::string& allocatorType = "simple")
{
    std::string allocatorFlag;
    if (allocatorType=="mark_sweep") {
        allocatorFlag = "-DUSE_MARK_SWEEP";
    }
    else if (allocatorType=="reference_count") {
        allocatorFlag = "-DUSE_REF_COUNT";
    }

    std::string command = "cd ../runtime && make && cd ../build && gcc -o "+outputPath+" "+
            cFilePath+" "+
            "../runtime/lib/libjblang_runtime.a "+
            allocatorFlag+" "+
            "-I ../runtime/include";

    return system(command.c_str())==0;
}

void printUsage(const char* programName)
{
    std::cerr << "Usage: " << programName << " <input-file> -o <output-name>\n";
}

int main(int argc, char* argv[])
{
    if (argc!=4 || std::string(argv[2])!="-o") {
        printUsage(argv[0]);
        return 1;
    }

    std::string inputFile = argv[1];
    std::string outputName = argv[3];
    std::string cFilePath = outputName+".c";
    std::string executablePath = outputName;

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

        std::string allocator_type = "reference_count";
        bool useRefCount = true;

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

        if (!compileCode(cFilePath, executablePath, allocator_type)) {
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
