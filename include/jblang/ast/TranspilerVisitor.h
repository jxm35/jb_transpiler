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
public:
    TranspilerVisitor(std::unique_ptr<CodeGenerator> codeGenerator)
            :m_codeGen(std::move(codeGenerator)), m_symbolTable(std::make_unique<SymbolTable>()),
             m_typeSystem(std::make_unique<TypeSystem>()) { }

    // Main entry point
    virtual antlrcpp::Any visitProgram(JBLangParser::ProgramContext* ctx) override;

    // Preprocessor
    virtual antlrcpp::Any visitPreprocessorDirective(JBLangParser::PreprocessorDirectiveContext* ctx) override;

    // Declarations
    virtual antlrcpp::Any visitFunctionDecl(JBLangParser::FunctionDeclContext* ctx) override;
    virtual antlrcpp::Any visitVarDecl(JBLangParser::VarDeclContext* ctx) override;
    virtual antlrcpp::Any visitStructDecl(JBLangParser::StructDeclContext* ctx) override;
    virtual antlrcpp::Any visitTypedefDecl(JBLangParser::TypedefDeclContext* ctx) override;
    virtual antlrcpp::Any visitArrayDecl(JBLangParser::ArrayDeclContext* ctx) override;

    // Statements
    virtual antlrcpp::Any visitBlock(JBLangParser::BlockContext* ctx) override;
    virtual antlrcpp::Any visitSpawnStmt(JBLangParser::SpawnStmtContext* ctx) override;
    virtual antlrcpp::Any visitReturnStmt(JBLangParser::ReturnStmtContext* ctx) override;
    virtual antlrcpp::Any visitExprStmt(JBLangParser::ExprStmtContext* ctx) override;
    virtual antlrcpp::Any visitIfStmt(JBLangParser::IfStmtContext* ctx) override;
    virtual antlrcpp::Any visitWhileStmt(JBLangParser::WhileStmtContext* ctx) override;

    // Expressions
    virtual antlrcpp::Any visitPrimary(JBLangParser::PrimaryContext* ctx) override;
    virtual antlrcpp::Any visitLiteralExpr(JBLangParser::LiteralExprContext* ctx) override;
    virtual antlrcpp::Any visitMemberExpr(JBLangParser::MemberExprContext* ctx) override;
    virtual antlrcpp::Any visitFuncCallExpr(JBLangParser::FuncCallExprContext* ctx) override;
    virtual antlrcpp::Any visitMulDivExpr(JBLangParser::MulDivExprContext* ctx) override;
    virtual antlrcpp::Any visitAddSubExpr(JBLangParser::AddSubExprContext* ctx) override;
    virtual antlrcpp::Any visitCompareExpr(JBLangParser::CompareExprContext* ctx) override;
    virtual antlrcpp::Any visitAssignExpr(JBLangParser::AssignExprContext* ctx) override;
    virtual antlrcpp::Any visitPointerMemberExpr(JBLangParser::PointerMemberExprContext* ctx) override;
    virtual antlrcpp::Any visitAddressOfExpr(JBLangParser::AddressOfExprContext* ctx) override;
    virtual antlrcpp::Any visitDereferenceExpr(JBLangParser::DereferenceExprContext* ctx) override;
    virtual antlrcpp::Any visitArrayAccessExpr(JBLangParser::ArrayAccessExprContext* ctx) override;
    virtual antlrcpp::Any visitNewExpr(JBLangParser::NewExprContext* ctx) override;
    virtual antlrcpp::Any visitStructInitExpr(JBLangParser::StructInitExprContext* ctx) override;

    virtual antlrcpp::Any visitInitializerList(JBLangParser::InitializerListContext* ctx) override;

    virtual antlrcpp::Any visitFunctionCall(JBLangParser::FunctionCallContext* ctx) override;
    Type getStructFromCode(JBLangParser::StructDeclContext* ctx);
    Type getArrayFromCode(JBLangParser::ArrayDeclContext* ctx);

    // Helpers
    virtual antlrcpp::Any visitLiteral(JBLangParser::LiteralContext* ctx) override;

private:
    std::unique_ptr<CodeGenerator> m_codeGen;
    std::unique_ptr<SymbolTable> m_symbolTable;
    std::unique_ptr<TypeSystem> m_typeSystem;

    std::stringstream m_output;
    int m_tempVarCounter;

    bool addRefCounts = true;
};