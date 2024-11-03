#include "TranspilerVisitor.h"
#include "CompilerError.h"
//#include "../build/JBLangParser.h"
#include <stdexcept>

antlrcpp::Any TranspilerVisitor::visitProgram(JBLangParser::ProgramContext *ctx) {
    m_output << "#include \"runtime.h\"\n";

    for (auto stmt : ctx->preprocessorDirective()) {
        visit(stmt);
        m_output << "\n";
    }

    for (auto stmt : ctx->statement()) {
        visit(stmt);
        m_output << "\n";
    }

    m_output << "int main() {\n    runtime_init();\n    main_();\n    runtime_print_stats();\n}\n";

    return m_output.str();
}

Type TranspilerVisitor::getArrayFromCode(JBLangParser::ArrayDeclContext *ctx) {
    Type type = m_typeSystem->resolveType(ctx->typeSpec()->getText());
    std::vector<int> sizes;
    sizes.reserve(ctx->arraySize().size());
    for (auto size : ctx->arraySize()) {
        if (size->IDENTIFIER())  {
            std::string defineVal =  size->IDENTIFIER()->getText();
            sizes.emplace_back(std::stoi(m_typeSystem->getDefineValue(defineVal)));

        } else {
            sizes.emplace_back(std::stoi(size->INTEGER()->getText()));
        }
    }
    type.setArray(ctx->IDENTIFIER()->getText(), sizes);
    return type;
}

antlrcpp::Any TranspilerVisitor::visitFunctionDecl(JBLangParser::FunctionDeclContext *ctx) {
    auto func = std::make_shared<Function>();

    func->name = ctx->IDENTIFIER()->getText();

    // Handle parameters
    if (ctx->paramList()) {
        for (auto param : ctx->paramList()->param()) {
            Type paramType;
            std::string paramName;
            if (param->arrayDecl()) {
                paramType = this->getArrayFromCode(param->arrayDecl());
                paramName = paramType.getArrayName();
            } else {
                paramType = m_typeSystem->resolveType(param->typeSpec()->getText());
                paramName = param->IDENTIFIER()->getText();
            }
            func->params.emplace_back(paramName, paramType);
        }
    }

    // Handle return type
    func->returnType = ctx->typeSpec() ? m_typeSystem->resolveType(ctx->typeSpec()->getText()) : Type(Type::BaseType::Void);


    m_typeSystem->registerFunction(func);
    m_output << m_codeGen->generateFunctionDecl(func);
    visit(ctx->block());

    return nullptr;
}

antlrcpp::Any TranspilerVisitor::visitVarDecl(JBLangParser::VarDeclContext *ctx) {
    try {
        std::string varName;
        Type varType;
        if (ctx->arrayDecl()) {
            varType = getArrayFromCode(ctx->arrayDecl());
            varName = varType.getArrayName();
        } else {
            varName = ctx->IDENTIFIER()->getText();
            varType = m_typeSystem->resolveType(ctx->typeSpec()->getText());
        }
        if (ctx->expression()) {
            auto initExpr = std::any_cast<std::string>(visit(ctx->expression()));
            m_output << m_symbolTable->getIndentLevel() << m_codeGen->generateVarDecl(varName, varType, " = " + initExpr);
        } else {
            m_output << m_symbolTable->getIndentLevel() << m_codeGen->generateVarDecl(varName, varType);
        }
        m_symbolTable->addSymbol(varName, varType);

        return nullptr;
    }
    catch (const CompilerError& e) {
        throw e;
    }
}

antlrcpp::Any TranspilerVisitor::visitBlock(JBLangParser::BlockContext *ctx) {
    m_symbolTable->enterScope();
    m_output << m_codeGen->generateScopeEntry();

    for (auto stmt : ctx->statement()) {
        visit(stmt);
    }

    m_symbolTable->exitScope();
    m_output << m_symbolTable->getIndentLevel() << m_codeGen->generateScopeExit(m_symbolTable->getCurrentScopeSymbols());
    return nullptr;
}

antlrcpp::Any TranspilerVisitor::visitSpawnStmt(JBLangParser::SpawnStmtContext *ctx) {
    auto* funcCallCtx = dynamic_cast<JBLangParser::FuncCallExprContext*>(ctx->expression());
    if (!funcCallCtx) {
        throw std::runtime_error("Can only spawn function calls");
    }

    auto* funcCall = funcCallCtx->functionCall();
    std::string funcName = funcCall->IDENTIFIER()->getText();

    std::string wrapperName = funcName + "_wrapper_" + std::to_string(m_tempVarCounter++);
//    generateSpawnWrapper(wrapperName, funcCall, func);

    m_output << m_symbolTable->getIndentLevel() << "runtime_spawn(" << wrapperName << ", NULL);\n";
    return nullptr;
}

antlrcpp::Any TranspilerVisitor::visitReturnStmt(JBLangParser::ReturnStmtContext *ctx) {
    std::string returnExpr = std::any_cast<std::string>(visit(ctx->expression()));
    m_output << m_symbolTable->getIndentLevel() << m_codeGen->generateReturn(returnExpr, Type(Type::BaseType::NO_TYPE));
    return nullptr;
}

antlrcpp::Any TranspilerVisitor::visitExprStmt(JBLangParser::ExprStmtContext *ctx) {
    m_output << m_symbolTable->getIndentLevel() << std::any_cast<std::string>(visit(ctx->expression())) << ";\n";
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
    auto left = std::any_cast<std::string>(visit(ctx->expression(0)));
    auto right = std::any_cast<std::string>(visit(ctx->expression(1)));
    return left + " " + ctx->op->getText() + " " + right;
}

antlrcpp::Any TranspilerVisitor::visitAddSubExpr(JBLangParser::AddSubExprContext *ctx) {
    auto left = std::any_cast<std::string>(visit(ctx->expression(0)));
    auto right = std::any_cast<std::string>(visit(ctx->expression(1)));
    return left + " " + ctx->op->getText() + " " + right;
}

antlrcpp::Any TranspilerVisitor::visitCompareExpr(JBLangParser::CompareExprContext *ctx) {
    auto left = std::any_cast<std::string>(visit(ctx->expression(0)));
    auto right = std::any_cast<std::string>(visit(ctx->expression(1)));
    return left + " " + ctx->op->getText() + " " + right;
}

antlrcpp::Any TranspilerVisitor::visitAssignExpr(JBLangParser::AssignExprContext *ctx) {
    auto left = std::any_cast<std::string>(visit(ctx->expression(0)));
    auto right = std::any_cast<std::string>(visit(ctx->expression(1)));
    return left + " = " + right;
}

antlrcpp::Any TranspilerVisitor::visitLiteral(JBLangParser::LiteralContext *ctx) {
    if (ctx->INTEGER()) return ctx->INTEGER()->getText();
    if (ctx->STRING()) return ctx->STRING()->getText();
    return ctx->getText(); // for true/false
}

antlrcpp::Any TranspilerVisitor::visitFunctionCall(JBLangParser::FunctionCallContext *ctx) {
    std::vector<std::string> args;

    if (ctx->argumentList()) {
        args.reserve(ctx->argumentList()->expression().size());
        for (auto arg : ctx->argumentList()->expression()) {
            args.emplace_back(std::any_cast<std::string>(visit(arg)));
        }
    }

    return m_codeGen->generateFunctionCall(ctx->IDENTIFIER()->getText(), args);
}

antlrcpp::Any TranspilerVisitor::visitPreprocessorDirective(JBLangParser::PreprocessorDirectiveContext *ctx) {
    if (ctx->getText().substr(0, 7) == "#define") {
        std::string name = ctx->IDENTIFIER()->getText();
        std::string value = ctx->INTEGER() ? ctx->INTEGER()->getText() :
                            ctx->STRING() ? ctx->STRING()->getText() : "";
        m_typeSystem->registerDefine(name, value);
        m_output << "#define " << name << " " << value << "\n";
    } else {
        m_output << ctx->getText();
    }
    return nullptr;
}

Type TranspilerVisitor::getStructFromCode(JBLangParser::StructDeclContext *ctx) {
    std::string structName = ctx->IDENTIFIER()->getText();
    std::vector<std::pair<std::string, Type>> members;
    members.reserve(ctx->structMember().size());
    for (auto member : ctx->structMember()) {
        if (member->arrayDecl()) {
            Type type = this->getArrayFromCode(member->arrayDecl());
            members.emplace_back(type.getArrayName(), type);
        } else {
            members.emplace_back(member->IDENTIFIER()->getText(), m_typeSystem->resolveType(member->typeSpec()->getText()));
        }
    }

    return m_typeSystem->registerStruct(structName, members);

}

antlrcpp::Any TranspilerVisitor::visitStructDecl(JBLangParser::StructDeclContext *ctx) {
    Type structType = getStructFromCode(ctx);

    m_output << m_codeGen->generateStructDecl(structType.getStructName(), structType);
    return nullptr;
}

antlrcpp::Any TranspilerVisitor::visitIfStmt(JBLangParser::IfStmtContext *ctx) {
    m_output << m_symbolTable->getIndentLevel() << "if (" << std::any_cast<std::string>(visit(ctx->expression())) << ") ";

    visit(ctx->statement(0));

    if (ctx->statement(1)) {
        m_output << m_symbolTable->getIndentLevel() << "else ";
        visit(ctx->statement(1));
    }

    return nullptr;
}

antlrcpp::Any TranspilerVisitor::visitWhileStmt(JBLangParser::WhileStmtContext *ctx) {
    m_output << m_symbolTable->getIndentLevel() << "while (" << std::any_cast<std::string>(visit(ctx->expression())) << ") ";
    visit(ctx->statement());
    return nullptr;
}

antlrcpp::Any TranspilerVisitor::visitPointerMemberExpr(JBLangParser::PointerMemberExprContext *ctx) {
    std::string base = std::any_cast<std::string>(visit(ctx->expression()));
    return base + "->" + ctx->IDENTIFIER()->getText();
}

antlrcpp::Any TranspilerVisitor::visitAddressOfExpr(JBLangParser::AddressOfExprContext *ctx) {
    return "&" + std::any_cast<std::string>(visit(ctx->expression()));
}

antlrcpp::Any TranspilerVisitor::visitDereferenceExpr(JBLangParser::DereferenceExprContext *ctx) {
    return "*" + std::any_cast<std::string>(visit(ctx->expression()));
}



antlrcpp::Any TranspilerVisitor::visitTypedefDecl(JBLangParser::TypedefDeclContext *ctx) {
    Type type = ctx->structDecl() ? getStructFromCode(ctx->structDecl()) : m_typeSystem->resolveType(ctx->typeSpec()->getText());
    m_typeSystem->registerTypeDef(ctx->IDENTIFIER()->getText(), type);
    m_output << m_codeGen->generateTypeDef(ctx->IDENTIFIER()->getText(), type);

    return nullptr;
}

antlrcpp::Any TranspilerVisitor::visitArrayAccessExpr(JBLangParser::ArrayAccessExprContext *ctx) {
    auto array = std::any_cast<std::string>(visit(ctx->expression(0)));
    auto index = std::any_cast<std::string>(visit(ctx->expression(1)));
    return array + "[" + index + "]";
}

antlrcpp::Any TranspilerVisitor::visitNewExpr(JBLangParser::NewExprContext *ctx) {
    return m_codeGen->generateAlloc(m_typeSystem->resolveType(ctx->typeSpec()->getText()));
}

antlrcpp::Any TranspilerVisitor::visitArrayDecl(JBLangParser::ArrayDeclContext *ctx) {
    Type type = getArrayFromCode(ctx);
    m_output << type.toString();

//    m_output << "    " << memberType.toString() << " "
//             << ctx->IDENTIFIER()->getText();
//    for (auto size : ctx->arraySize()) {
//        m_output << "[";
//        if (size->IDENTIFIER())  {
//            m_output << size->IDENTIFIER()->getText();
//        } else {
//            m_output << size->INTEGER()->getText();
//        }
//        m_output  << "]";
//    }
//    m_output << ";\n";
    return nullptr;
}
