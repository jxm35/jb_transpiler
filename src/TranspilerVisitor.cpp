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
    m_symbolTable->currentFunc = func;
    m_output << m_codeGen->generateFunctionDecl(func);
    visit(ctx->block());
    m_symbolTable->currentFunc = nullptr;

    return nullptr;
}

antlrcpp::Any TranspilerVisitor::visitVarDecl(JBLangParser::VarDeclContext *ctx) {
    try {
        std::string indentLevel = m_symbolTable->getIndentLevel();
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
            Variable assignedFrom;
            bool found = m_symbolTable->lookupSymbol(initExpr, assignedFrom);
            if (found && assignedFrom.type.isPointer()) {
                m_output << indentLevel << m_codeGen->generateIncRef(assignedFrom);
            }
            m_output << indentLevel << m_codeGen->generateVarDecl(varName, varType, " = " + initExpr);
        } else {
            m_output << indentLevel << m_codeGen->generateVarDecl(varName, varType);
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

    std::string indentLevel = m_symbolTable->getIndentLevel();

    for (auto stmt : ctx->statement()) {
        visit(stmt);
    }

    for (const auto& var: m_symbolTable->getCurrentScopeSymbols()) {
        if (var.second.type.isPointer()) {
            m_output << indentLevel << m_codeGen->generateDecRef(var.second);
        }
    }

    m_symbolTable->exitScope();
    m_output << indentLevel << m_codeGen->generateScopeExit(m_symbolTable->getCurrentScopeSymbols());
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
    Variable returnedVariable;
    this->addRefCounts = false;
    auto returnExpr = std::any_cast<std::string>(visit(ctx->expression()));
    this->addRefCounts = true;
    bool found = m_symbolTable->lookupSymbol(returnExpr, returnedVariable);
    if (found && returnedVariable.type.isPointer()) {
        m_output << m_symbolTable->getIndentLevel() << m_codeGen->generateIncRef(returnedVariable);
    }

    std::string indentLevel = m_symbolTable->getIndentLevel();
    for (const auto& var: m_symbolTable->getCurrentScopeSymbols()) {
        if (var.second.type.isPointer()) {
            m_output << indentLevel << m_codeGen->generateDecRef(var.second);
        }
    }

    m_output << indentLevel << m_codeGen->generateReturn(returnExpr, Type(Type::BaseType::NO_TYPE));
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

    Variable assignedFrom;
    bool found = m_symbolTable->lookupSymbol(right, assignedFrom);
    if (found && assignedFrom.type.isPointer()) {
        // check if we are assigning to a struct pointer
        Variable assignedTo;
        size_t pos = left.find("->");
        if (pos != std::string::npos) {
            found = m_symbolTable->lookupSymbol(left.substr(0, pos), assignedTo);
            if (found && assignedTo.type.isPointer()) {
                m_output << m_symbolTable->getIndentLevel() << m_codeGen->generateIncRef(assignedFrom, assignedTo.name);
            }
        } else {
            m_output << m_symbolTable->getIndentLevel() << m_codeGen->generateIncRef(assignedFrom);
        }
    }
    found = m_symbolTable->lookupSymbol(left, assignedFrom);
    if (found && assignedFrom.type.isPointer()) {
        m_output << m_symbolTable->getIndentLevel() << m_codeGen->generateDecRef(assignedFrom);
    }

    return left + " = " + right;
}

antlrcpp::Any TranspilerVisitor::visitLiteral(JBLangParser::LiteralContext *ctx) {
    if (ctx->INTEGER()) return ctx->INTEGER()->getText();
    if (ctx->STRING()) return ctx->STRING()->getText();
    return ctx->getText(); // for true/false
}

antlrcpp::Any TranspilerVisitor::visitFunctionCall(JBLangParser::FunctionCallContext *ctx) {
    std::vector<std::string> args;
    std::string code = "";
    std::string indentLevel = m_symbolTable->getIndentLevel();
    bool pointerArgs = false;

    if (ctx->argumentList()) {
        args.reserve(ctx->argumentList()->expression().size());
        for (auto arg : ctx->argumentList()->expression()) {
            auto argString = std::any_cast<std::string>(visit(arg));

            Variable assignedFrom;
            bool found = m_symbolTable->lookupSymbol(argString, assignedFrom);
            if (this->addRefCounts && found && assignedFrom.type.isPointer()) {
                pointerArgs = true;
                m_output << indentLevel <<  m_codeGen->generateIncRef(assignedFrom);
            }

            args.emplace_back(argString);
        }
    }

    code += indentLevel + m_codeGen->generateFunctionCall(ctx->IDENTIFIER()->getText(), args) + (pointerArgs ? ";\n" : "");

    if (this->addRefCounts && ctx->argumentList()) {
        args.reserve(ctx->argumentList()->expression().size());
        for (auto arg : ctx->argumentList()->expression()) {
            auto argString = std::any_cast<std::string>(visit(arg));

            Variable assignedFrom;
            bool found = m_symbolTable->lookupSymbol(argString, assignedFrom);
            if (found && assignedFrom.type.isPointer()) {
                code +=  indentLevel + m_codeGen->generateDecRef(assignedFrom);
            }
        }
    }

    return code;
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
    m_typeSystem->registerStruct(structName);

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

    return m_typeSystem->setStructMembers(structName, members);

}

antlrcpp::Any TranspilerVisitor::visitStructDecl(JBLangParser::StructDeclContext *ctx) {
    Type structType = getStructFromCode(ctx);

    m_output << m_codeGen->generateStructDecl(structType.getStructName(), structType);
    return nullptr;
}

antlrcpp::Any TranspilerVisitor::visitIfStmt(JBLangParser::IfStmtContext *ctx) {
    this->addRefCounts = false;
    m_output << m_symbolTable->getIndentLevel() << "if (" << std::any_cast<std::string>(visit(ctx->expression())) << ") {\n";
    this->addRefCounts = true;

    visit(ctx->statement(0));
    m_output << "}\n";

    if (ctx->statement(1)) {
        m_output << m_symbolTable->getIndentLevel() << "else {\n";
        visit(ctx->statement(1));
        m_output << "}\n";
    }

    return nullptr;
}

antlrcpp::Any TranspilerVisitor::visitWhileStmt(JBLangParser::WhileStmtContext *ctx) {
    this->addRefCounts = false;
    m_output << m_symbolTable->getIndentLevel() << "while (" << std::any_cast<std::string>(visit(ctx->expression())) << ") {\n";
    this->addRefCounts = true;
    visit(ctx->statement());
    m_output << "}\n";
    return nullptr;
}

antlrcpp::Any TranspilerVisitor::visitPointerMemberExpr(JBLangParser::PointerMemberExprContext *ctx) {
    auto base = std::any_cast<std::string>(visit(ctx->expression()));
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
    return nullptr;
}

antlrcpp::Any TranspilerVisitor::visitStructInitExpr(JBLangParser::StructInitExprContext *ctx) {
    std::string code = "{\n";
    code += std::any_cast<std::string>(visit(ctx->initializerList()));

    return code + m_symbolTable->getIndentLevel() +  "}";
}

antlrcpp::Any TranspilerVisitor::visitInitializerList(JBLangParser::InitializerListContext *ctx) {
    std::string code;
    Variable assignedFrom;
    std::string indentLevel = m_symbolTable->getIndentLevel();

    for (auto initializer : ctx->initializer()) {
        std::string assignExpr = initializer->expression()->getText();
        bool found = m_symbolTable->lookupSymbol(assignExpr, assignedFrom);
        if (found && assignedFrom.type.isPointer()) {
            m_output <<  indentLevel + m_codeGen->generateIncRef(assignedFrom);
        }
        code += indentLevel +  "." + initializer->IDENTIFIER()->getText() + " = " + assignExpr + ",\n";
    }
    return code;
}
