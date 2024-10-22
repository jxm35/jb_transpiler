// main.cpp
#include "antlr4-runtime.h"
#include "JBLangLexer.h"
#include "JBLangParser.h"
#include "TranspilerVisitor.h"
#include <iostream>
#include <fstream>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <input-file>\n";
        return 1;
    }

    try {
        // Setup ANTLR input
        std::ifstream stream;
        stream.open(argv[1]);
        antlr4::ANTLRInputStream input(stream);

        // Create lexer and parser
        JBLangLexer lexer(&input);
        antlr4::CommonTokenStream tokens(&lexer);
        JBLangParser parser(&tokens);

        // Parse and visit
        auto* tree = parser.program();
        TranspilerVisitor visitor;
        std::string cCode = std::any_cast<std::string>(visitor.visitProgram(tree));

        // Output generated code
        std::cout << cCode;

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}