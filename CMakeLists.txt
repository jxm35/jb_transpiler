cmake_minimum_required(VERSION 3.10)
project(JBLangTranspiler)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_OSX_ARCHITECTURES "arm64")

find_package(Java COMPONENTS Runtime REQUIRED)

set(ANTLR_JAR ${CMAKE_SOURCE_DIR}/antlr-4.13.1-complete.jar)

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

include_directories(
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_BINARY_DIR}
        ${ANTLR4_INCLUDE_DIR}
)

add_custom_command(
        OUTPUT
        ${CMAKE_BINARY_DIR}/JBLangLexer.cpp
        ${CMAKE_BINARY_DIR}/JBLangParser.cpp
        ${CMAKE_BINARY_DIR}/JBLangBaseVisitor.cpp
        ${CMAKE_BINARY_DIR}/JBLangVisitor.cpp
        COMMAND
        ${Java_JAVA_EXECUTABLE} -jar ${ANTLR_JAR} -Dlanguage=Cpp -visitor -no-listener
        -o ${CMAKE_BINARY_DIR}
        ${CMAKE_SOURCE_DIR}/grammar/JBLang.g4
        DEPENDS
        ${CMAKE_SOURCE_DIR}/grammar/JBLang.g4
        WORKING_DIRECTORY
        ${CMAKE_BINARY_DIR}
)

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

add_library(JBLangCore
        src/ast/TranspilerVisitor.cpp
        src/codegen/CCodeGenerator.cpp
        src/types/TypeSystem.cpp
        src/types/SymbolTable.cpp
        src/core/CompilerError.cpp
        src/runtime/runtime.c
        src/runtime/allocator_impl.c
        src/runtime/reference_count_allocator.c
        src/runtime/mark_sweep_allocator.c
        src/runtime/simple_allocator.c
        )

target_include_directories(JBLangCore
        PUBLIC
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_BINARY_DIR}
        )

add_executable(transpiler
        src/main.cpp
        )

target_link_libraries(transpiler
        PRIVATE
        JBLangCore
        JBLangParser
        )

include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
FetchContent_MakeAvailable(googletest)
enable_testing()

add_executable(tests tests/test_core.cpp)
target_link_libraries(tests JBLangCore JBLangParser GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(tests)
