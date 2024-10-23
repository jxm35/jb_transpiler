// TranspilerVisitor.cpp
#include "TranspilerVisitor.h"
//#include "../build/JBLangParser.h"
#include <stdexcept>

// Type and Function implementations remain the same...

antlrcpp::Any TranspilerVisitor::visitProgram(JBLangParser::ProgramContext *ctx) {
    m_output << "#include \"runtime.h\"\n";
    m_output << "#include <stdio.h>\n";
    m_output << "#include <stdbool.h>\n\n";

    for (auto stmt : ctx->statement()) {
        visit(stmt);
        m_output << "\n";
    }

    return m_output.str();
}

antlrcpp::Any TranspilerVisitor::visitFunctionDecl(JBLangParser::FunctionDeclContext *ctx) {
    auto func = std::make_shared<Function>();

    func->name = ctx->IDENTIFIER()->getText();

    // Handle parameters
    if (ctx->paramList()) {
        for (auto param : ctx->paramList()->param()) {
            Type paramType = translateType(param->typeSpec()->getText());
            func->params.push_back({param->IDENTIFIER()->getText(), paramType});
        }
    }

    // Handle return type
    func->returnType = ctx->typeSpec() ?
                       translateType(ctx->typeSpec()->getText()) : Type(Type::BaseType::Void);

    m_functions[func->name] = func;

    // Generate function declaration
    m_output << func->getSignature();
    visit(ctx->block());

    return nullptr;
}

antlrcpp::Any TranspilerVisitor::visitVarDecl(JBLangParser::VarDeclContext *ctx) {
    std::string varName = ctx->IDENTIFIER()->getText();
    Type varType = translateType(ctx->typeSpec()->getText());

    if (isDeclaredInCurrentScope(varName)) {
        throw std::runtime_error("Variable " + varName + " already declared in this scope");
    }

    m_currentScope->variables[varName] = varType;

    m_output << varType.toString() << " " << varName;
    if (ctx->expression()) {
        m_output << " = " << std::any_cast<std::string>(visit(ctx->expression()));
    }
    m_output << ";\n";

    return nullptr;
}

antlrcpp::Any TranspilerVisitor::visitBlock(JBLangParser::BlockContext *ctx) {
    pushScope();

    m_output << "{\n";
//    this->m_output << "runtime_block_start();\n";

    for (auto stmt : ctx->statement()) {
        visit(stmt);
    }

//    this->m_output << "runtime_block_end();\n";
    m_output << "}\n";

    popScope();
    return nullptr;
}

antlrcpp::Any TranspilerVisitor::visitSpawnStmt(JBLangParser::SpawnStmtContext *ctx) {
    auto* funcCallCtx = dynamic_cast<JBLangParser::FuncCallExprContext*>(ctx->expression());
    if (!funcCallCtx) {
        throw std::runtime_error("Can only spawn function calls");
    }

    auto* funcCall = funcCallCtx->functionCall();
    std::string funcName = funcCall->IDENTIFIER()->getText();
    auto func = m_functions[funcName];

    std::string wrapperName = funcName + "_wrapper_" + std::to_string(m_tempVarCounter++);
    generateSpawnWrapper(wrapperName, funcCall, func);

    m_output << "runtime_spawn(" << wrapperName << ", NULL);\n";
    return nullptr;
}

antlrcpp::Any TranspilerVisitor::visitReturnStmt(JBLangParser::ReturnStmtContext *ctx) {
    m_output << "return";
    if (ctx->expression()) {
        m_output << " " << std::any_cast<std::string>(visit(ctx->expression()));
    }
    m_output << ";\n";
    return nullptr;
}

antlrcpp::Any TranspilerVisitor::visitExprStmt(JBLangParser::ExprStmtContext *ctx) {
    m_output << std::any_cast<std::string>(visit(ctx->expression())) << ";\n";
    return nullptr;
}

antlrcpp::Any TranspilerVisitor::visitPrimary(JBLangParser::PrimaryContext *ctx) {
    if (ctx->expression()) {
        return "(" + std::any_cast<std::string>(visit(ctx->expression())) + ")";
    } else if (ctx->IDENTIFIER()) {
        return ctx->IDENTIFIER()->getText();
    } else {
        return visit(ctx->literal());
    }
}

antlrcpp::Any TranspilerVisitor::visitLiteralExpr(JBLangParser::LiteralExprContext *ctx) {
    return visit(ctx->literal());
}

antlrcpp::Any TranspilerVisitor::visitMemberExpr(JBLangParser::MemberExprContext *ctx) {
    std::string base = std::any_cast<std::string>(visit(ctx->expression()));
    return base + "." + ctx->IDENTIFIER()->getText();
}

antlrcpp::Any TranspilerVisitor::visitFuncCallExpr(JBLangParser::FuncCallExprContext *ctx) {
    return visit(ctx->functionCall());
}

antlrcpp::Any TranspilerVisitor::visitMulDivExpr(JBLangParser::MulDivExprContext *ctx) {
    std::string left = std::any_cast<std::string>(visit(ctx->expression(0)));
    std::string right = std::any_cast<std::string>(visit(ctx->expression(1)));
    return left + " " + ctx->op->getText() + " " + right;
}

antlrcpp::Any TranspilerVisitor::visitAddSubExpr(JBLangParser::AddSubExprContext *ctx) {
    std::string left = std::any_cast<std::string>(visit(ctx->expression(0)));
    std::string right = std::any_cast<std::string>(visit(ctx->expression(1)));
    return left + " " + ctx->op->getText() + " " + right;
}

antlrcpp::Any TranspilerVisitor::visitCompareExpr(JBLangParser::CompareExprContext *ctx) {
    std::string left = std::any_cast<std::string>(visit(ctx->expression(0)));
    std::string right = std::any_cast<std::string>(visit(ctx->expression(1)));
    return left + " " + ctx->op->getText() + " " + right;
}

antlrcpp::Any TranspilerVisitor::visitAssignExpr(JBLangParser::AssignExprContext *ctx) {
    std::string left = std::any_cast<std::string>(visit(ctx->expression(0)));
    std::string right = std::any_cast<std::string>(visit(ctx->expression(1)));
    return left + " = " + right;
}

antlrcpp::Any TranspilerVisitor::visitLiteral(JBLangParser::LiteralContext *ctx) {
    if (ctx->INTEGER()) return ctx->INTEGER()->getText();
    if (ctx->STRING()) return ctx->STRING()->getText();
    return ctx->getText(); // for true/false
}

antlrcpp::Any TranspilerVisitor::visitFunctionCall(JBLangParser::FunctionCallContext *ctx) {
    std::stringstream result;
    result << ctx->IDENTIFIER()->getText() << "(";

    if (ctx->argumentList()) {
        bool first = true;
        for (auto arg : ctx->argumentList()->expression()) {
            if (!first) result << ", ";
            result << std::any_cast<std::string>(visit(arg));
            first = false;
        }
    }

    result << ")";
    return result.str();
}

void TranspilerVisitor::generateSpawnWrapper(
        const std::string& wrapperName,
        JBLangParser::FunctionCallContext* funcCall,
        std::shared_ptr<Function> func) {

    m_output << "void " << wrapperName << "(void* arg) {\n";
    m_output << "    " << func->name << "(";

    if (funcCall->argumentList()) {
        bool first = true;
        for (auto expr : funcCall->argumentList()->expression()) {
            if (!first) m_output << ", ";
            m_output << std::any_cast<std::string>(visit(expr));
            first = false;
        }
    }

    m_output << ");\n}\n\n";
}

Type TranspilerVisitor::translateType(const std::string& sourceType) {
    if (sourceType == "void") return Type(Type::BaseType::Void);
    if (sourceType == "int") return Type(Type::BaseType::Int);
    if (sourceType == "string") return Type(Type::BaseType::String, true); // strings are char*
    if (sourceType == "bool") return Type(Type::BaseType::Bool);

    throw std::runtime_error("Unknown type: " + sourceType);
}

bool TranspilerVisitor::isDeclaredInCurrentScope(const std::string& name) const {
    return m_currentScope->variables.find(name) != m_currentScope->variables.end();
}

void TranspilerVisitor::pushScope() {
    auto newScope = std::make_shared<Scope>();
    newScope->parent = m_currentScope;
    m_currentScope = newScope;
    m_isInGlobalScope = false;
}

void TranspilerVisitor::popScope() {
    if (m_currentScope->parent) {
        m_currentScope = m_currentScope->parent;
        m_isInGlobalScope = (m_currentScope->parent == nullptr);
    }
}

TranspilerVisitor::TranspilerVisitor() {}

void TranspilerVisitor::addBuiltinFunctions() {

}

bool TranspilerVisitor::Scope::lookupVariable(const std::string& name, Type& type) const {
    auto it = variables.find(name);
    if (it != variables.end()) {
        type = it->second;
        return true;
    }
    return parent && parent->lookupVariable(name, type);
}

std::string baseTypeToString(const Type::BaseType& type) {
    switch (type) {
        case Type::BaseType::Void:
            return "void";
        case Type::BaseType::Int:
            return "int";
        case Type::BaseType::String:
            return "char*";
        case Type::BaseType::Bool:
            return "bool";
        case Type::BaseType::Struct:
            return "struct";
    }
}
std::string Type::toString() const {
    return baseTypeToString(this->m_baseType);
}


std::string Function::getSignature() const {
    std::string signature =  this->returnType.toString() + " " + this->name + "(";
    bool first = true;
    for (const std::pair<std::string, Type>& paramPair : this->params) {
        if (!first) {
            signature += ", ";
        }
        signature +=  paramPair.second.toString() + " " + paramPair.first;
        first = false;
    }
    return signature + ")";
}
