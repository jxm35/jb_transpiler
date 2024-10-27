// TranspilerVisitor.cpp
#include "TranspilerVisitor.h"
//#include "../build/JBLangParser.h"
#include <stdexcept>

// Type and Function implementations remain the same...

std::string outputVariable(const Type& type, const std::string& name) {
    if (type.isArray()) {
        return type.toString();
    } else {
        return type.toString() + " " + name;
    }
}

antlrcpp::Any TranspilerVisitor::visitProgram(JBLangParser::ProgramContext *ctx) {
    m_output << "#include \"runtime.h\"\n";
//    m_output << "#include <stdio.h>\n";
//    m_output << "#include <stdbool.h>\n\n";
    for (auto stmt : ctx->preprocessorDirective()) {
        visit(stmt);
        m_output << "\n";
    }

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
            Type paramType;
            std::string paramName;
            if (param->arrayDecl()) {
                paramName = param->arrayDecl()->IDENTIFIER()->getText();
                paramType = translateType(param->arrayDecl()->typeSpec()->getText());
                int size = std::stoi(param->arrayDecl()->arraySize(0)->INTEGER()->getText());
                paramType.setArray(paramName, size);
            } else {
                paramType = translateType(param->typeSpec()->getText());
                paramName = param->IDENTIFIER()->getText();
            }
            func->params.push_back({paramName, paramType});
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
    std::string varName;
    Type varType;
    if (ctx->arrayDecl()) {
        varType = translateType(ctx->arrayDecl()->typeSpec()->getText());
        varName = ctx->arrayDecl()->IDENTIFIER()->getText();
        auto size = ctx->arrayDecl()->arraySize(0);
        if (size->IDENTIFIER())  {
//            m_output << size->IDENTIFIER()->getText();
            varType.setArray(varName, std::stoi(m_defines[size->IDENTIFIER()->getText()]));
        } else {
//            m_output << size->INTEGER()->getText();
            varType.setArray(varName, std::stoi(size->INTEGER()->getText()));
        }
    } else {
        varName = ctx->IDENTIFIER()->getText();
        varType = translateType(ctx->typeSpec()->getText());
    }

    if (isDeclaredInCurrentScope(varName)) {
        throw std::runtime_error("Variable " + varName + " already declared in this scope");
    }

//    if (ctx->arrayDecl()) {
//        for (auto size : ctx->arrayDecl()->arraySize()) {
//            m_output << "[";
//            if (size->IDENTIFIER())  {
//                m_output << size->IDENTIFIER()->getText();
//                varType.setArray(std::stoi(m_defines[size->IDENTIFIER()->getText()]));
//            } else {
//                m_output << size->INTEGER()->getText();
//                varType.setArray(std::stoi(size->INTEGER()->getText()));
//            }
//            m_output  << "]";
//        }
//    }
//    m_output << varType.toString() << " " << varName;
    m_output << outputVariable(varType, varName);
    if (ctx->expression()) {
        m_output << " = " << std::any_cast<std::string>(visit(ctx->expression()));
    }

    m_output << ";\n";
    m_currentScope->variables[varName] = varType;

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
    Type type;

    size_t arrayStart = sourceType.find('[');
    bool isArray = arrayStart != std::string::npos;
    std::string baseType = isArray ? sourceType.substr(0, arrayStart) : sourceType;

    bool isPointer = sourceType.find('*') != std::string::npos;
    baseType = isPointer ? sourceType.substr(0, sourceType.find('*')) : sourceType;

    if (baseType == "void") type = Type(Type::BaseType::Void);
    else if (baseType == "int") type = Type(Type::BaseType::Int);
    else if (baseType == "string") type = Type(Type::BaseType::String, true); // strings are char*
    else if (baseType == "bool") type = Type(Type::BaseType::Bool);
    else if (m_typedefs.find(baseType) != m_typedefs.end())type = m_typedefs[baseType];
    else throw std::runtime_error("Unknown type: " + sourceType);

    if (isPointer) {
        type.setPointer(true);
    }

    if (isArray) {
        size_t arrayEnd = sourceType.find(']');
        if (arrayEnd != std::string::npos) {
            std::string sizeStr = sourceType.substr(arrayStart + 1, arrayEnd - arrayStart - 1);
            int size = sizeStr.empty() ? 0 : std::stoi(sizeStr);
            type.setArray("james_fix", size);
        } else {
            throw std::runtime_error("Invalid array type: " + sourceType);
        }
    }

    return type;
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

antlrcpp::Any TranspilerVisitor::visitPreprocessorDirective(JBLangParser::PreprocessorDirectiveContext *ctx) {
//    if (ctx->getText().substr(0, 8) == "#include") {
//        std::string includeFile = ctx->STRING()->getText();
//        m_output << "#include " << includeFile << "\n";
//    } else
    if (ctx->getText().substr(0, 7) == "#define") {
        std::string name = ctx->IDENTIFIER()->getText();
        std::string value = ctx->INTEGER() ? ctx->INTEGER()->getText() :
                            ctx->STRING() ? ctx->STRING()->getText() : "";
        m_defines[name] = value;
        m_output << "#define " << name << " " << value << "\n";
    } else {
        m_output << ctx->getText();
    }
    return nullptr;
}

antlrcpp::Any TranspilerVisitor::visitStructDecl(JBLangParser::StructDeclContext *ctx) {
    m_output << "struct " << ctx->IDENTIFIER()->getText() << " {\n";

    for (auto member : ctx->structMember()) {
        if (member->arrayDecl()) {
            visit(member->arrayDecl());
        } else {
            Type memberType = translateType(member->typeSpec()->getText());
            m_output << "    " << memberType.toString() << " "
                     << member->IDENTIFIER()->getText() << ";\n";
        }
    }

    m_output << "}\n";
    return nullptr;
}

antlrcpp::Any TranspilerVisitor::visitIfStmt(JBLangParser::IfStmtContext *ctx) {
    m_output << "if (" << std::any_cast<std::string>(visit(ctx->expression())) << ") ";

    visit(ctx->statement(0));

    if (ctx->statement(1)) {  // Has else clause
        m_output << "else ";
        visit(ctx->statement(1));
    }

    return nullptr;
}

antlrcpp::Any TranspilerVisitor::visitWhileStmt(JBLangParser::WhileStmtContext *ctx) {
    m_output << "while (" << std::any_cast<std::string>(visit(ctx->expression())) << ") ";
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
    Type type;
    m_output << "typedef ";
    if (ctx->structDecl()) {
        visit(ctx->structDecl());
        type = Type(Type::BaseType::Struct);
        type.setStruct(ctx->structDecl()->IDENTIFIER()->getText());
    } else {
        type = translateType(ctx->typeSpec()->getText());
        m_output << type.toString();
    }
    std::string alias = ctx->IDENTIFIER()->getText();
    m_typedefs[alias] = type;

    m_output  << " " << alias << ";\n";
    return nullptr;
}

antlrcpp::Any TranspilerVisitor::visitArrayAccessExpr(JBLangParser::ArrayAccessExprContext *ctx) {
    auto array = std::any_cast<std::string>(visit(ctx->expression(0)));
    auto index = std::any_cast<std::string>(visit(ctx->expression(1)));
    return array + "[" + index + "]";
}

antlrcpp::Any TranspilerVisitor::visitArrayDecl(JBLangParser::ArrayDeclContext *ctx) {
    Type memberType = translateType(ctx->typeSpec()->getText());
    m_output << "    " << memberType.toString() << " "
             << ctx->IDENTIFIER()->getText();
    for (auto size : ctx->arraySize()) {
        m_output << "[";
        if (size->IDENTIFIER())  {
            m_output << size->IDENTIFIER()->getText();
        } else {
            m_output << size->INTEGER()->getText();
        }
        m_output  << "]";
    }
    m_output << ";\n";
    return nullptr;
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
    std::string str;
    if (m_isStruct) {
        str = this->getStructName();
    } else {
        str = baseTypeToString(this->m_baseType);
    }

    if (m_arrayInfo.isArray) {
        str += " " + m_arrayInfo.name;
        str += "[";
        if (m_arrayInfo.size > 0) {
            str += std::to_string(m_arrayInfo.size);
        }
        str += "]";
    }

    if (this->isPointer()) {
        str += "*";
    }
    return str;
}


std::string Function::getSignature() const {
    std::string signature =  this->returnType.toString() + " " + this->name + "(";
    bool first = true;
    for (const std::pair<std::string, Type>& paramPair : this->params) {
        if (!first) {
            signature += ", ";
        }
        signature += outputVariable(paramPair.second, paramPair.first);
//        signature +=  paramPair.second.toString() + " " + paramPair.first;
        first = false;
    }
    return signature + ")";
}
