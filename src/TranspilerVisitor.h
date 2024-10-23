// TranspilerVisitor.h
#pragma once
#include "JBLangBaseVisitor.h"
#include <string>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <memory>

// Represents a variable's type and attributes
class Type {
public:
    enum class BaseType {
        Void,
        Int,
        String,
        Bool,
        Struct
    };

    // Default constructor
    Type() : m_baseType(BaseType::Void), m_isPointer(false), m_isConst(false) {}

    // Main constructors
    explicit Type(BaseType base) : m_baseType(base), m_isPointer(false), m_isConst(false) {}
    Type(BaseType base, bool isPointer, bool isConst = false)
            : m_baseType(base), m_isPointer(isPointer), m_isConst(isConst) {}

    bool isPointer() const { return m_isPointer; }
    bool isConst() const { return m_isConst; }
    BaseType getBaseType() const { return m_baseType; }
    std::string toString() const;

private:
    BaseType m_baseType;
    bool m_isPointer;
    bool m_isConst;
};

// Represents a function declaration
class Function {
public:
    // Default constructor
    Function() : returnType(Type::BaseType::Void), isStatic(false) {}

    // Copy constructor
    Function(const Function& other) = default;

    std::string name;
    std::vector<std::pair<std::string, Type>> params;  // (name, type) pairs
    Type returnType;
    bool isStatic;

    std::string getSignature() const;
};

class TranspilerVisitor : public JBLangBaseVisitor {
public:
    TranspilerVisitor();

    // Main entry point
    virtual antlrcpp::Any visitProgram(JBLangParser::ProgramContext *ctx) override;

    // Declarations
    virtual antlrcpp::Any visitFunctionDecl(JBLangParser::FunctionDeclContext *ctx) override;
    virtual antlrcpp::Any visitVarDecl(JBLangParser::VarDeclContext *ctx) override;

    // Statements
    virtual antlrcpp::Any visitBlock(JBLangParser::BlockContext *ctx) override;
    virtual antlrcpp::Any visitSpawnStmt(JBLangParser::SpawnStmtContext *ctx) override;
    virtual antlrcpp::Any visitReturnStmt(JBLangParser::ReturnStmtContext *ctx) override;
    virtual antlrcpp::Any visitExprStmt(JBLangParser::ExprStmtContext *ctx) override;

    // Expressions
    virtual antlrcpp::Any visitPrimary(JBLangParser::PrimaryContext *ctx) override;
    virtual antlrcpp::Any visitLiteralExpr(JBLangParser::LiteralExprContext *ctx) override;
    virtual antlrcpp::Any visitMemberExpr(JBLangParser::MemberExprContext *ctx) override;
    virtual antlrcpp::Any visitFuncCallExpr(JBLangParser::FuncCallExprContext *ctx) override;
    virtual antlrcpp::Any visitMulDivExpr(JBLangParser::MulDivExprContext *ctx) override;
    virtual antlrcpp::Any visitAddSubExpr(JBLangParser::AddSubExprContext *ctx) override;
    virtual antlrcpp::Any visitCompareExpr(JBLangParser::CompareExprContext *ctx) override;
    virtual antlrcpp::Any visitAssignExpr(JBLangParser::AssignExprContext *ctx) override;

    // Helpers
    virtual antlrcpp::Any visitLiteral(JBLangParser::LiteralContext *ctx) override;
    virtual antlrcpp::Any visitFunctionCall(JBLangParser::FunctionCallContext *ctx) override;

    // Get the generated code
    std::string getOutput() const { return m_output.str(); }

private:
    void generateSpawnWrapper(const std::string& wrapperName,
                              JBLangParser::FunctionCallContext* funcCall,
                              std::shared_ptr<Function> func);
    Type translateType(const std::string& sourceType);
    void addBuiltinFunctions();

    // Scope management
    class Scope {
    public:
        std::unordered_map<std::string, Type> variables;
        std::shared_ptr<Scope> parent;

        Scope() : parent(nullptr) {}  // Added default constructor
        bool lookupVariable(const std::string& name, Type& type) const;
    };

    void pushScope();
    void popScope();
    bool isDeclaredInCurrentScope(const std::string& name) const;

private:
    std::stringstream m_output;
    std::unordered_map<std::string, std::shared_ptr<Function>> m_functions;
    std::shared_ptr<Scope> m_currentScope;
    int m_tempVarCounter;
    bool m_isInGlobalScope;
};