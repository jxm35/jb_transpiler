#pragma once

#include "JBLangBaseVisitor.h"
#include "jblang/types/TypeSystem.h"
#include "jblang/codegen/CodeGenerator.h"
#include "jblang/types/SymbolTable.h"
#include <string>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <memory>

class TranspilerVisitor : public JBLangBaseVisitor {
public:
    explicit TranspilerVisitor(std::unique_ptr<CodeGenerator> codeGenerator)
            :m_codeGen(std::move(codeGenerator)), m_symbolTable(std::make_unique<SymbolTable>()),
             m_typeSystem(std::make_unique<TypeSystem>()), m_tempVarCounter(0), addRefCounts(true) { }

    ~TranspilerVisitor() override = default;

    // Non-copyable but movable
    TranspilerVisitor(const TranspilerVisitor&) = delete;
    TranspilerVisitor& operator=(const TranspilerVisitor&) = delete;
    TranspilerVisitor(TranspilerVisitor&&) = default;
    TranspilerVisitor& operator=(TranspilerVisitor&&) = default;

    // Main entry point
    antlrcpp::Any visitProgram(JBLangParser::ProgramContext* ctx) override;

    // Preprocessor
    antlrcpp::Any visitPreprocessorDirective(JBLangParser::PreprocessorDirectiveContext* ctx) override;

    // Declarations
    antlrcpp::Any visitFunctionDecl(JBLangParser::FunctionDeclContext* ctx) override;
    antlrcpp::Any visitVarDecl(JBLangParser::VarDeclContext* ctx) override;
    antlrcpp::Any visitStructDecl(JBLangParser::StructDeclContext* ctx) override;
    antlrcpp::Any visitClassDecl(JBLangParser::ClassDeclContext* ctx) override;
    antlrcpp::Any visitClassField(JBLangParser::ClassFieldContext* ctx) override;
    antlrcpp::Any visitClassMethod(JBLangParser::ClassMethodContext* ctx) override;
    antlrcpp::Any visitClassConstructor(JBLangParser::ClassConstructorContext* ctx) override;
    antlrcpp::Any visitNewWithConstructorExpr(JBLangParser::NewWithConstructorExprContext* ctx) override;
    antlrcpp::Any visitTypedefDecl(JBLangParser::TypedefDeclContext* ctx) override;
    antlrcpp::Any visitArrayDecl(JBLangParser::ArrayDeclContext* ctx) override;

    // Statements
    antlrcpp::Any visitBlock(JBLangParser::BlockContext* ctx) override;
    antlrcpp::Any visitSpawnStmt(JBLangParser::SpawnStmtContext* ctx) override;
    antlrcpp::Any visitReturnStmt(JBLangParser::ReturnStmtContext* ctx) override;
    antlrcpp::Any visitExprStmt(JBLangParser::ExprStmtContext* ctx) override;
    antlrcpp::Any visitIfStmt(JBLangParser::IfStmtContext* ctx) override;
    antlrcpp::Any visitWhileStmt(JBLangParser::WhileStmtContext* ctx) override;
    antlrcpp::Any visitForStmt(JBLangParser::ForStmtContext* ctx) override;

    // Expressions
    antlrcpp::Any visitPrimary(JBLangParser::PrimaryContext* ctx) override;
    antlrcpp::Any visitLiteralExpr(JBLangParser::LiteralExprContext* ctx) override;
    antlrcpp::Any visitMemberExpr(JBLangParser::MemberExprContext* ctx) override;
    antlrcpp::Any visitFuncCallExpr(JBLangParser::FuncCallExprContext* ctx) override;
    antlrcpp::Any visitMulDivExpr(JBLangParser::MulDivExprContext* ctx) override;
    antlrcpp::Any visitAddSubExpr(JBLangParser::AddSubExprContext* ctx) override;
    antlrcpp::Any visitCompareExpr(JBLangParser::CompareExprContext* ctx) override;
    antlrcpp::Any visitAssignExpr(JBLangParser::AssignExprContext* ctx) override;
    antlrcpp::Any visitPointerMemberExpr(JBLangParser::PointerMemberExprContext* ctx) override;
    antlrcpp::Any visitMethodCallExpr(JBLangParser::MethodCallExprContext* ctx) override;
    antlrcpp::Any visitAddressOfExpr(JBLangParser::AddressOfExprContext* ctx) override;
    antlrcpp::Any visitDereferenceExpr(JBLangParser::DereferenceExprContext* ctx) override;
    antlrcpp::Any visitArrayAccessExpr(JBLangParser::ArrayAccessExprContext* ctx) override;
    antlrcpp::Any visitNewExpr(JBLangParser::NewExprContext* ctx) override;
    antlrcpp::Any visitStructInitExpr(JBLangParser::StructInitExprContext* ctx) override;
    antlrcpp::Any visitPostIncrementExpr(JBLangParser::PostIncrementExprContext* ctx) override;
    antlrcpp::Any visitPostDecrementExpr(JBLangParser::PostDecrementExprContext* ctx) override;
    antlrcpp::Any visitPreIncrementExpr(JBLangParser::PreIncrementExprContext* ctx) override;
    antlrcpp::Any visitPreDecrementExpr(JBLangParser::PreDecrementExprContext* ctx) override;
    antlrcpp::Any visitInitializerList(JBLangParser::InitializerListContext* ctx) override;
    antlrcpp::Any visitFunctionCall(JBLangParser::FunctionCallContext* ctx) override;

    // Helpers
    antlrcpp::Any visitLiteral(JBLangParser::LiteralContext* ctx) override;

    Type getStructFromCode(JBLangParser::StructDeclContext* ctx);
    Type getClassFromCode(JBLangParser::ClassDeclContext* ctx);
    std::string generateParentConstructorCall(JBLangParser::ClassConstructorContext* ctx, const std::string& className);
    std::string resolveFieldAccess(const std::string& fieldName, const std::string& className);
    Type getArrayFromCode(JBLangParser::ArrayDeclContext* ctx);
    std::vector<std::string> getClassNames() const;
    void generateClassMethodBodies();

private:
    std::unique_ptr<CodeGenerator> m_codeGen;
    std::unique_ptr<SymbolTable> m_symbolTable;
    std::unique_ptr<TypeSystem> m_typeSystem;
    bool m_first_pass;

    std::stringstream m_output;
    int m_tempVarCounter;
    bool addRefCounts;
    std::vector<std::string> m_classNames;
    Type resolveTypeFromContext(JBLangParser::TypeSpecContext* ctx);
};