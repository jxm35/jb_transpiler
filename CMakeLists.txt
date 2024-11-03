cmake_minimum_required(VERSION 3.10)
project(JBLangTranspiler)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add to CMakeLists.txt before project()
set(CMAKE_OSX_ARCHITECTURES "arm64")

# Find Java (needed for ANTLR)
find_package(Java COMPONENTS Runtime REQUIRED)

# Set ANTLR jar path
set(ANTLR_JAR ${CMAKE_SOURCE_DIR}/antlr-4.13.1-complete.jar)

# Find ANTLR runtime
find_library(ANTLR4_RUNTIME
        NAMES antlr4-runtime
        PATHS /usr/local/lib
        /opt/homebrew/lib
        REQUIRED
        )

find_path(ANTLR4_INCLUDE_DIR
        NAMES antlr4-runtime.h
        PATHS /usr/local/include/antlr4-runtime
        /opt/homebrew/include/antlr4-runtime
        REQUIRED
        )

# Custom command to generate ANTLR parser
add_custom_command(
        OUTPUT
        ${CMAKE_BINARY_DIR}/JBLangLexer.cpp
        ${CMAKE_BINARY_DIR}/JBLangParser.cpp
        ${CMAKE_BINARY_DIR}/JBLangBaseVisitor.cpp
        ${CMAKE_BINARY_DIR}/JBLangVisitor.cpp
        COMMAND
        ${Java_JAVA_EXECUTABLE} -jar ${ANTLR_JAR} -Dlanguage=Cpp -visitor -no-listener
        -o ${CMAKE_BINARY_DIR}
        ${CMAKE_SOURCE_DIR}/src/JBLang.g4
        DEPENDS
        ${CMAKE_SOURCE_DIR}/src/JBLang.g4
        WORKING_DIRECTORY
        ${CMAKE_BINARY_DIR}
)

# Add the generated files to a library
add_library(JBLangParser
        ${CMAKE_BINARY_DIR}/JBLangLexer.cpp
        ${CMAKE_BINARY_DIR}/JBLangParser.cpp
        ${CMAKE_BINARY_DIR}/JBLangBaseVisitor.cpp
        ${CMAKE_BINARY_DIR}/JBLangVisitor.cpp
        )

target_include_directories(JBLangParser
        PUBLIC
        ${CMAKE_BINARY_DIR}
        build
        ${ANTLR4_INCLUDE_DIR}
        )

target_link_libraries(JBLangParser
        PUBLIC
        ${ANTLR4_RUNTIME}
        )

# Add the main executable
add_executable(transpiler
        src/main.cpp
        src/TranspilerVisitor.cpp src/CCodeGenerator.cpp src/CodeGenerator.h src/SymbolTable.h src/TypeSystem.cpp src/TypeSystem.h src/CompilerError.cpp src/CompilerError.h src/SymbolTable.cpp src/SymbolTable.h src/CCodeGenerator.h)

target_link_libraries(transpiler
        PRIVATE
        JBLangParser
        )