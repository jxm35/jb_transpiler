#include "jblang/ast/TranspilerVisitor.h"
#include "jblang/core/CompilerError.h"
//#include "../build/JBLangParser.h"
#include <stdexcept>

antlrcpp::Any TranspilerVisitor::visitProgram(JBLangParser::ProgramContext* ctx)
{
    m_output << "#include \"runtime.h\"\n";

    for (auto stmt : ctx->preprocessorDirective()) {
        visit(stmt);
        m_output << "\n";
    }

    for (auto stmt : ctx->statement()) {
        visit(stmt);
        m_output << "\n";
    }

    m_output << m_typeSystem->generateVTableStruct();

    for (const auto& className : m_classNames) {
        if (m_typeSystem->hasVirtualMethods(className)) {
            m_output << m_typeSystem->generateVTableInstance(className);
        }
    }

//    generateClassMethodBodies();

    m_output << "int main() {\n    runtime_init();\n";
    for (const auto& [name, type] : m_symbolTable->globalVars) {
        m_output << "    runtime_register_root(&" << name << ");\n";
    }
    m_output << "    main_();\n    runtime_shutdown();\n}\n";

    return m_output.str();
}

Type TranspilerVisitor::getArrayFromCode(JBLangParser::ArrayDeclContext* ctx)
{
    Type type = resolveTypeFromContext(ctx->typeSpec());
    std::vector<int> sizes;
    sizes.reserve(ctx->arraySize().size());
    for (auto size : ctx->arraySize()) {
        if (size->IDENTIFIER()) {
            std::string defineVal = size->IDENTIFIER()->getText();
            sizes.emplace_back(std::stoi(m_typeSystem->getDefineValue(defineVal)));

        }
        else {
            sizes.emplace_back(std::stoi(size->INTEGER()->getText()));
        }
    }
    type.setArray(ctx->IDENTIFIER()->getText(), sizes);
    return type;
}

antlrcpp::Any TranspilerVisitor::visitFunctionDecl(JBLangParser::FunctionDeclContext* ctx)
{
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
            }
            else {
                paramType = resolveTypeFromContext(param->typeSpec());
                paramName = param->IDENTIFIER()->getText();
            }
            func->params.emplace_back(paramName, paramType);
        }
    }

    func->returnType = ctx->typeSpec() ? resolveTypeFromContext(ctx->typeSpec()) : Type(
            Type::BaseType::Void);

    m_typeSystem->registerFunction(func);
    m_symbolTable->currentFunc = func;
    m_output << m_codeGen->generateFunctionDecl(func);
    visit(ctx->block());
    m_symbolTable->currentFunc = nullptr;

    return nullptr;
}

antlrcpp::Any TranspilerVisitor::visitVarDecl(JBLangParser::VarDeclContext* ctx)
{
    try {
        std::string indentLevel = m_symbolTable->getIndentLevel();
        std::string varName;
        Type varType;
        if (ctx->arrayDecl()) {
            varType = getArrayFromCode(ctx->arrayDecl());
            varName = varType.getArrayName();
        }
        else {
            varName = ctx->IDENTIFIER()->getText();
            varType = resolveTypeFromContext(ctx->typeSpec());
        }
        if (m_symbolTable->isGlobalScope() && varType.isPointer()) {
            m_symbolTable->globalVars.emplace_back(varName, varType);
        }
        if (ctx->expression()) {
            auto initExpr = std::any_cast<std::string>(visit(ctx->expression()));

            Variable initVar;
            if (m_symbolTable->lookupSymbol(initExpr, initVar)) {
                if (m_typeSystem->isCompatible(initVar.type, varType)) {
                    initExpr = m_codeGen->generateCast(initExpr, initVar.type, varType);
                }
            }
            Variable assignedFrom;
            bool found = m_symbolTable->lookupSymbol(initExpr, assignedFrom);
            if (found && assignedFrom.type.isPointer()) {
                m_output << indentLevel << m_codeGen->generateIncRef(assignedFrom);
            }
            m_output << indentLevel << m_codeGen->generateVarDecl(varName, varType, " = "+initExpr);
        }
        else {
            m_output << indentLevel << m_codeGen->generateVarDecl(varName, varType);
        }
        m_symbolTable->addSymbol(varName, varType);

        return nullptr;
    }
    catch (const CompilerError& e) {
        throw e;
    }
}

antlrcpp::Any TranspilerVisitor::visitBlock(JBLangParser::BlockContext* ctx)
{
    m_symbolTable->enterScope();
    m_output << m_codeGen->generateScopeEntry();

    std::string indentLevel = m_symbolTable->getIndentLevel();

    for (auto stmt : ctx->statement()) {
        visit(stmt);
    }

    for (const auto& var : m_symbolTable->getCurrentScopeSymbols()) {
        if (var.second.type.isPointer()) {
            m_output << indentLevel << m_codeGen->generateDecRef(var.second);
        }
    }

    m_symbolTable->exitScope();
    m_output << indentLevel << m_codeGen->generateScopeExit(m_symbolTable->getCurrentScopeSymbols());
    return nullptr;
}

antlrcpp::Any TranspilerVisitor::visitSpawnStmt(JBLangParser::SpawnStmtContext* ctx)
{
    auto* funcCallCtx = dynamic_cast<JBLangParser::FuncCallExprContext*>(ctx->expression());
    if (!funcCallCtx) {
        throw std::runtime_error("Can only spawn function calls");
    }

    auto* funcCall = funcCallCtx->functionCall();
    std::string funcName = funcCall->IDENTIFIER()->getText();

    std::string wrapperName = funcName+"_wrapper_"+std::to_string(m_tempVarCounter++);
//    generateSpawnWrapper(wrapperName, funcCall, func);

    m_output << m_symbolTable->getIndentLevel() << "runtime_spawn(" << wrapperName << ", NULL);\n";
    return nullptr;
}

antlrcpp::Any TranspilerVisitor::visitReturnStmt(JBLangParser::ReturnStmtContext* ctx)
{
    Variable returnedVariable;
    this->addRefCounts = false;
    auto returnExpr = std::any_cast<std::string>(visit(ctx->expression()));
    this->addRefCounts = true;
    bool found = m_symbolTable->lookupSymbol(returnExpr, returnedVariable);
    if (found && returnedVariable.type.isPointer()) {
        m_output << m_symbolTable->getIndentLevel() << m_codeGen->generateIncRef(returnedVariable);
    }

    std::string indentLevel = m_symbolTable->getIndentLevel();
    for (const auto& var : m_symbolTable->getCurrentScopeSymbols()) {
        if (var.second.type.isPointer()) {
            m_output << indentLevel << m_codeGen->generateDecRef(var.second);
        }
    }

    m_output << indentLevel << m_codeGen->generateReturn(returnExpr, Type(Type::BaseType::NO_TYPE));
    return nullptr;
}

antlrcpp::Any TranspilerVisitor::visitExprStmt(JBLangParser::ExprStmtContext* ctx)
{
    m_output << m_symbolTable->getIndentLevel() << std::any_cast<std::string>(visit(ctx->expression())) << ";\n";
    return nullptr;
}

antlrcpp::Any TranspilerVisitor::visitForStmt(JBLangParser::ForStmtContext* ctx)
{
    std::string indentLevel = m_symbolTable->getIndentLevel();

    m_output << indentLevel << "for (";

    if (ctx->varDecl()) {
        std::string varName;
        Type varType;
        if (ctx->varDecl()->arrayDecl()) {
            varType = getArrayFromCode(ctx->varDecl()->arrayDecl());
            varName = varType.getArrayName();
        }
        else {
            varName = ctx->varDecl()->IDENTIFIER()->getText();
            varType = resolveTypeFromContext(ctx->varDecl()->typeSpec());
        }

        m_output << varType.toString() << " " << varName;
        if (ctx->varDecl()->expression()) {
            auto initExpr = std::any_cast<std::string>(visit(ctx->varDecl()->expression()));
            m_output << " = " << initExpr;
        }

        m_symbolTable->addSymbol(varName, varType);
    }
    else if (ctx->exprStmt()) {
        auto exprStr = std::any_cast<std::string>(visit(ctx->exprStmt()->expression()));
        m_output << exprStr;
    }

    m_output << "; ";

    if (ctx->expression().size()>=1 && ctx->expression(0)) {
        this->addRefCounts = false;
        m_output << std::any_cast<std::string>(visit(ctx->expression(0)));
        this->addRefCounts = true;
    }

    m_output << "; ";

    if (ctx->expression().size()>=2 && ctx->expression(1)) {
        m_output << std::any_cast<std::string>(visit(ctx->expression(1)));
    }

    m_output << ") ";

    if (auto blockCtx = dynamic_cast<JBLangParser::BlockContext*>(ctx->statement())) {
        visit(blockCtx);
    }
    else {
        m_output << "{\n";
        visit(ctx->statement());
        m_output << indentLevel << "}\n";
    }

    return nullptr;
}

antlrcpp::Any TranspilerVisitor::visitPrimary(JBLangParser::PrimaryContext* ctx)
{
    if (ctx->expression()) {
        return "("+std::any_cast<std::string>(visit(ctx->expression()))+")";
    }
    else if (ctx->IDENTIFIER()) {
        return ctx->IDENTIFIER()->getText();
    }
    else {
        return visit(ctx->literal());
    }
}

antlrcpp::Any TranspilerVisitor::visitLiteralExpr(JBLangParser::LiteralExprContext* ctx)
{
    return visit(ctx->literal());
}

antlrcpp::Any TranspilerVisitor::visitMemberExpr(JBLangParser::MemberExprContext* ctx)
{
    std::string base = std::any_cast<std::string>(visit(ctx->expression()));
    return base+"."+ctx->IDENTIFIER()->getText();
}

antlrcpp::Any TranspilerVisitor::visitFuncCallExpr(JBLangParser::FuncCallExprContext* ctx)
{
    return visit(ctx->functionCall());
}

antlrcpp::Any TranspilerVisitor::visitMulDivExpr(JBLangParser::MulDivExprContext* ctx)
{
    auto left = std::any_cast<std::string>(visit(ctx->expression(0)));
    auto right = std::any_cast<std::string>(visit(ctx->expression(1)));
    return left+" "+ctx->op->getText()+" "+right;
}

antlrcpp::Any TranspilerVisitor::visitAddSubExpr(JBLangParser::AddSubExprContext* ctx)
{
    auto left = std::any_cast<std::string>(visit(ctx->expression(0)));
    auto right = std::any_cast<std::string>(visit(ctx->expression(1)));
    return left+" "+ctx->op->getText()+" "+right;
}

antlrcpp::Any TranspilerVisitor::visitCompareExpr(JBLangParser::CompareExprContext* ctx)
{
    auto left = std::any_cast<std::string>(visit(ctx->expression(0)));
    auto right = std::any_cast<std::string>(visit(ctx->expression(1)));
    return left+" "+ctx->op->getText()+" "+right;
}

antlrcpp::Any TranspilerVisitor::visitAssignExpr(JBLangParser::AssignExprContext* ctx)
{
    auto left = std::any_cast<std::string>(visit(ctx->expression(0)));
    auto right = std::any_cast<std::string>(visit(ctx->expression(1)));

    Variable leftVar, rightVar;
    bool leftFound = m_symbolTable->lookupSymbol(left, leftVar);
    bool rightFound = m_symbolTable->lookupSymbol(right, rightVar);

    if (leftFound && rightFound) {
        if (m_typeSystem->isCompatible(rightVar.type, leftVar.type)) {
            right = m_codeGen->generateCast(right, rightVar.type, leftVar.type);
        }
    }

    Variable assignedFrom;
    bool found = m_symbolTable->lookupSymbol(right, assignedFrom);
    if (found && assignedFrom.type.isPointer()) {
        // check if we are assigning to a struct pointer
        Variable assignedTo;
        size_t pos = left.find("->");
        if (pos!=std::string::npos) {
            found = m_symbolTable->lookupSymbol(left.substr(0, pos), assignedTo);
            if (found && assignedTo.type.isPointer()) {
                m_output << m_symbolTable->getIndentLevel() << m_codeGen->generateIncRef(assignedFrom, assignedTo.name);
            }
        }
        else {
            m_output << m_symbolTable->getIndentLevel() << m_codeGen->generateIncRef(assignedFrom);
        }
    }
    found = m_symbolTable->lookupSymbol(left, assignedFrom);
    if (found && assignedFrom.type.isPointer()) {
        m_output << m_symbolTable->getIndentLevel() << m_codeGen->generateDecRef(assignedFrom);
    }

    return left+" = "+right;
}

antlrcpp::Any TranspilerVisitor::visitLiteral(JBLangParser::LiteralContext* ctx)
{
    if (ctx->INTEGER()) return ctx->INTEGER()->getText();
    if (ctx->STRING()) return ctx->STRING()->getText();
    return ctx->getText(); // for true/false
}

antlrcpp::Any TranspilerVisitor::visitFunctionCall(JBLangParser::FunctionCallContext* ctx)
{
    std::vector<std::string> args;
    std::string code = "";
    std::string indentLevel = m_symbolTable->getIndentLevel();
    bool pointerArgs = false;

    std::string funcName = ctx->IDENTIFIER()->getText();
    if (funcName.find("METHOD_CALL:")==0) {
        size_t firstColon = funcName.find(':', 12);
        size_t secondColon = funcName.find(':', firstColon+1);
        std::string methodName = funcName.substr(12, firstColon-12);
        std::string objName = funcName.substr(secondColon+1);

        args.push_back(objName);
    }

    else if (funcName.find("VIRTUAL_CALL:")==0) {
        size_t firstColon = funcName.find(':', 13);
        size_t secondColon = funcName.find(':', firstColon+1);
        std::string methodName = funcName.substr(13, firstColon-13);
        std::string objName = funcName.substr(secondColon+1);

        args.push_back(objName);

        code += indentLevel+objName+"->vtable->"+methodName+"("+objName;
        if (ctx->argumentList()) {
            for (auto arg : ctx->argumentList()->expression()) {
                code += ", "+std::any_cast<std::string>(visit(arg));
            }
        }
        code += ")";
        if (pointerArgs) {
            code += ";\n";
        }
        return code;
    }

    if (ctx->argumentList()) {
        args.reserve(ctx->argumentList()->expression().size());
        for (auto arg : ctx->argumentList()->expression()) {
            auto argString = std::any_cast<std::string>(visit(arg));

            Variable assignedFrom;
            bool found = m_symbolTable->lookupSymbol(argString, assignedFrom);
            if (this->addRefCounts && found && assignedFrom.type.isPointer()) {
                pointerArgs = true;
                m_output << indentLevel << m_codeGen->generateIncRef(assignedFrom);
            }

            args.emplace_back(argString);
        }
    }

    code += indentLevel+m_codeGen->generateFunctionCall(ctx->IDENTIFIER()->getText(), args)+(pointerArgs ? ";\n" : "");

    if (this->addRefCounts && ctx->argumentList()) {
        args.reserve(ctx->argumentList()->expression().size());
        for (auto arg : ctx->argumentList()->expression()) {
            auto argString = std::any_cast<std::string>(visit(arg));

            Variable assignedFrom;
            bool found = m_symbolTable->lookupSymbol(argString, assignedFrom);
            if (found && assignedFrom.type.isPointer()) {
                code += indentLevel+m_codeGen->generateDecRef(assignedFrom);
            }
        }
    }

    return code;
}

antlrcpp::Any TranspilerVisitor::visitPreprocessorDirective(JBLangParser::PreprocessorDirectiveContext* ctx)
{
    if (ctx->getText().substr(0, 7)=="#define") {
        std::string name = ctx->IDENTIFIER()->getText();
        std::string value = ctx->INTEGER() ? ctx->INTEGER()->getText() :
                            ctx->STRING() ? ctx->STRING()->getText() : "";
        m_typeSystem->registerDefine(name, value);
        m_output << "#define " << name << " " << value << "\n";
    }
    else {
        m_output << ctx->getText();
    }
    return nullptr;
}

Type TranspilerVisitor::getStructFromCode(JBLangParser::StructDeclContext* ctx)
{
    std::string structName = ctx->IDENTIFIER()->getText();
    m_typeSystem->registerStruct(structName);

    std::vector<std::pair<std::string, Type>> members;

    members.reserve(ctx->structMember().size());
    for (auto member : ctx->structMember()) {
        if (member->arrayDecl()) {
            Type type = this->getArrayFromCode(member->arrayDecl());
            members.emplace_back(type.getArrayName(), type);
        }
        else {
            members.emplace_back(member->IDENTIFIER()->getText(),
                    resolveTypeFromContext(member->typeSpec()));
        }
    }

    return m_typeSystem->setStructMembers(structName, members);
}

antlrcpp::Any TranspilerVisitor::visitStructDecl(JBLangParser::StructDeclContext* ctx)
{
    Type structType = getStructFromCode(ctx);

    m_output << m_codeGen->generateStructDecl(structType.getStructName(), structType);
    return nullptr;
}

Type TranspilerVisitor::getClassFromCode(JBLangParser::ClassDeclContext* ctx)
{
    std::string className = ctx->IDENTIFIER(0)->getText();
    m_typeSystem->registerClass(className);

    if (ctx->IDENTIFIER().size()>1) {
        std::string parentName = ctx->IDENTIFIER(1)->getText();
        m_typeSystem->setClassParent(className, parentName);
    }
    std::vector<std::pair<std::string, Type>> fields;

    for (auto member : ctx->classMember()) {
        if (auto fieldCtx = dynamic_cast<JBLangParser::ClassFieldContext*>(member)) {
            if (fieldCtx->arrayDecl()) {
                Type type = this->getArrayFromCode(fieldCtx->arrayDecl());
                fields.emplace_back(type.getArrayName(), type);
            }
            else {
                fields.emplace_back(fieldCtx->IDENTIFIER()->getText(),
                        resolveTypeFromContext(fieldCtx->typeSpec()));
            }
        }
    }

    return m_typeSystem->setClassMembers(className, fields);
}

void TranspilerVisitor::processClassInheritance(JBLangParser::ClassDeclContext* ctx, const std::string& className)
{
    if (ctx->IDENTIFIER().size()>1) {
        std::string parentName = ctx->IDENTIFIER(1)->getText();
        m_typeSystem->setClassParent(className, parentName);
    }
}

std::string TranspilerVisitor::generateParentConstructorCall(JBLangParser::ClassConstructorContext* ctx,
        const std::string& className)
{
    if (ctx->IDENTIFIER().size()>1) {
        std::string parentName = ctx->IDENTIFIER(1)->getText();
        std::string parentConstructor = parentName+"_"+parentName;

        std::vector<std::string> args;
        args.push_back("&(this->parent)");

        for (auto arg : ctx->argumentList()->expression()) {
            args.push_back(std::any_cast<std::string>(visit(arg)));
        }

        return m_symbolTable->getIndentLevel()+m_codeGen->generateFunctionCall(parentConstructor, args)+";\n";
    }
    return "";
}

std::string TranspilerVisitor::resolveFieldAccess(const std::string& fieldName, const std::string& className)
{
    auto classType = m_typeSystem->resolveType(className);

    for (const auto& member : classType.getStructMembers()) {
        if (member.first==fieldName) {
            return "this->"+fieldName;
        }
    }

    if (classType.hasParent()) {
        std::string parentClass = classType.getParentClass();
        return "this->parent."+fieldName;  // TODO: Validate field exists in parent
    }

    throw CompilerError(CompilerError::ErrorType::NameError, "Field not found: "+fieldName);
}

antlrcpp::Any TranspilerVisitor::visitClassDecl(JBLangParser::ClassDeclContext* ctx)
{
    Type classType = getClassFromCode(ctx);
    std::string className = ctx->IDENTIFIER(0)->getText();
    m_classNames.push_back(className);
    m_output << "struct " << className << " {\n";

    if (classType.hasParent()) {
        m_output << "    struct " << classType.getParentClass() << " parent;\n";
    }
    else if (false) { // hasVirtuals
        m_output << "    struct vtable* vtable;\n";
    }
    for (const auto& member : classType.getStructMembers()) {
        m_output << "    " << member.second.toString() << " " << member.first << ";\n";
    }
    m_output << "};\n\n";

    for (auto member : ctx->classMember()) {
        if (auto constructorCtx = dynamic_cast<JBLangParser::ClassConstructorContext*>(member)) {
            auto constructor = std::make_shared<Function>();
            constructor->name = className+"_"+className;
            constructor->returnType = Type(Type::BaseType::Void);

            Type thisType = classType;
            thisType.setPointer(true);
            constructor->params.emplace_back("this", thisType);

            if (constructorCtx->paramList()) {
                for (auto param : constructorCtx->paramList()->param()) {
                    Type paramType;
                    std::string paramName;
                    if (param->arrayDecl()) {
                        paramType = this->getArrayFromCode(param->arrayDecl());
                        paramName = paramType.getArrayName();
                    }
                    else {
                        paramType = resolveTypeFromContext(param->typeSpec());
                        paramName = param->IDENTIFIER()->getText();
                    }
                    constructor->params.emplace_back(paramName, paramType);
                }
            }

            m_typeSystem->registerClassConstructor(className, constructor);
            m_symbolTable->currentFunc = constructor;
            m_output << m_codeGen->generateFunctionDecl(constructor) << " {\n";

            if (m_typeSystem->hasVirtualMethods(className)) {
                m_output << "    this->vtable = &" << className << "_vtable;\n";
            }

            m_output << generateParentConstructorCall(constructorCtx, className);
            visit(constructorCtx->block());
            m_output << "}\n\n";
            m_symbolTable->currentFunc = nullptr;
        }
        else if (auto methodCtx = dynamic_cast<JBLangParser::ClassMethodContext*>(member)) {
            auto method = std::make_shared<Function>();
            method->name = className+"_"+methodCtx->IDENTIFIER()->getText();
            method->returnType = resolveTypeFromContext(methodCtx->typeSpec());
            std::string methodText = methodCtx->getText();
            method->isVirtual = methodText.find("virtual")!=std::string::npos;

            Type thisType = classType;
            thisType.setPointer(true);
            method->params.emplace_back("this", thisType);

            if (methodCtx->paramList()) {
                for (auto param : methodCtx->paramList()->param()) {
                    Type paramType;
                    std::string paramName;
                    if (param->arrayDecl()) {
                        paramType = this->getArrayFromCode(param->arrayDecl());
                        paramName = paramType.getArrayName();
                    }
                    else {
                        paramType = resolveTypeFromContext(param->typeSpec());
                        paramName = param->IDENTIFIER()->getText();
                    }
                    method->params.emplace_back(paramName, paramType);
                }
            }

            m_typeSystem->registerClassMethod(className, method);
            m_symbolTable->currentFunc = method;
            m_output << m_codeGen->generateFunctionDecl(method);
            if (method->isVirtual) {
                m_output << " {\n    struct " << className << "* this = (struct " << className << "*)this_param;\n";
                visit(methodCtx->block());
                m_output << "}\n\n";
            }
            else {
                visit(methodCtx->block());
            }

            m_symbolTable->currentFunc = nullptr;
        }
    }

//    bool hasVirtuals = m_typeSystem->hasVirtualMethods(className);
//
//    m_output << "struct " << className << " {\n";
//
//    if (classType.hasParent()) {
//        m_output << "    struct " << classType.getParentClass() << " parent;\n";
//    }
//    else if (hasVirtuals) {
//        m_output << "    struct vtable* vtable;\n";
//    }
//
//    for (const auto& member : classType.getStructMembers()) {
//        m_output << "    " << member.second.toString() << " " << member.first << ";\n";
//    }
//    m_output << "};\n\n";

    return nullptr;
}

antlrcpp::Any TranspilerVisitor::visitClassConstructor(JBLangParser::ClassConstructorContext* ctx)
{
    return nullptr;
}

antlrcpp::Any TranspilerVisitor::visitNewWithConstructorExpr(JBLangParser::NewWithConstructorExprContext* ctx)
{
    std::string className = ctx->IDENTIFIER()->getText();

    auto constructor = m_typeSystem->getClassConstructor(className);
    if (!constructor) {
        throw CompilerError(CompilerError::ErrorType::NameError, "No constructor for class: "+className);
    }

    std::string tempVar = "temp_"+std::to_string(m_tempVarCounter++);
    std::string indentLevel = m_symbolTable->getIndentLevel();

    Type classType = m_typeSystem->resolveType(className);
    classType.setPointer(true);

    m_output << indentLevel << m_codeGen->generateVarDecl(tempVar, classType,
            " = "+m_codeGen->generateAlloc(m_typeSystem->resolveType(className)));

    std::vector<std::string> args;
    args.push_back(tempVar);

    if (ctx->argumentList()) {
        for (auto arg : ctx->argumentList()->expression()) {
            args.push_back(std::any_cast<std::string>(visit(arg)));
        }
    }

    m_output << indentLevel << m_codeGen->generateFunctionCall(constructor->name, args) << ";\n";

    return tempVar;
}

antlrcpp::Any TranspilerVisitor::visitMethodCallExpr(JBLangParser::MethodCallExprContext* ctx)
{
    auto objExpr = std::any_cast<std::string>(visit(ctx->expression()));
    std::string methodName = ctx->IDENTIFIER()->getText();

    Variable objVar;
    if (!m_symbolTable->lookupSymbol(objExpr, objVar)) {
        throw CompilerError(CompilerError::ErrorType::NameError, "Unknown object: "+objExpr);
    }

    if (!objVar.type.isClass()) {
        throw CompilerError(CompilerError::ErrorType::TypeError, "Method call on non-class type");
    }

    std::string className = objVar.type.getClassName();

    std::string fullMethodName;
    std::string currentClass = className;
    bool isVirtual = false;

    while (true) {
        auto methods = m_typeSystem->getClassMethods(currentClass);
        for (const auto& method : methods) {
            if (method->name==currentClass+"_"+methodName) {
                fullMethodName = method->name;
                isVirtual = method->isVirtual;
                break;
            }
        }

        if (!fullMethodName.empty()) break;

        auto classType = m_typeSystem->resolveType(currentClass);
        if (!classType.hasParent()) {
            throw CompilerError(CompilerError::ErrorType::NameError,
                    "Method not found: "+methodName+" in class "+className);
        }
        currentClass = classType.getParentClass();
    }

    if (isVirtual) {
        std::vector<std::string> args;
        args.push_back(objExpr);

        if (ctx->argumentList()) {
            for (auto arg : ctx->argumentList()->expression()) {
                args.push_back(std::any_cast<std::string>(visit(arg)));
            }
        }

        std::string call = objExpr+"->vtable->"+methodName+"("+objExpr;
        for (size_t i = 1; i<args.size(); ++i) {
            call += ", "+args[i];
        }
        return call+")";
    }

    std::vector<std::string> args;

    if (currentClass!=className) {
        args.push_back("&(("+objExpr+")->parent)");
    }
    else {
        args.push_back(objExpr);
    }

    if (ctx->argumentList()) {
        for (auto arg : ctx->argumentList()->expression()) {
            args.push_back(std::any_cast<std::string>(visit(arg)));
        }
    }

    return m_codeGen->generateFunctionCall(fullMethodName, args);
}

antlrcpp::Any TranspilerVisitor::visitClassField(JBLangParser::ClassFieldContext* ctx)
{
    return nullptr;
}

antlrcpp::Any TranspilerVisitor::visitClassMethod(JBLangParser::ClassMethodContext* ctx)
{
    return nullptr;
}

antlrcpp::Any TranspilerVisitor::visitIfStmt(JBLangParser::IfStmtContext* ctx)
{
    this->addRefCounts = false;
    m_output << m_symbolTable->getIndentLevel() << "if (" << std::any_cast<std::string>(visit(ctx->expression()))
             << ") {\n";
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

antlrcpp::Any TranspilerVisitor::visitWhileStmt(JBLangParser::WhileStmtContext* ctx)
{
    this->addRefCounts = false;
    m_output << m_symbolTable->getIndentLevel() << "while (" << std::any_cast<std::string>(visit(ctx->expression()))
             << ") {\n";
    this->addRefCounts = true;
    visit(ctx->statement());
    m_output << "}\n";
    return nullptr;
}

antlrcpp::Any TranspilerVisitor::visitPointerMemberExpr(JBLangParser::PointerMemberExprContext* ctx)
{
    auto base = std::any_cast<std::string>(visit(ctx->expression()));
    if (base=="this" && m_symbolTable->currentFunc) {
        std::string funcName = m_symbolTable->currentFunc->name;
        size_t underscore = funcName.find('_');
        if (underscore!=std::string::npos) {
            std::string className = funcName.substr(0, underscore);
            try {
                auto classType = m_typeSystem->resolveType(className);
                if (classType.isClass()) {
                    return resolveFieldAccess(ctx->IDENTIFIER()->getText(), className);
                }
            }
            catch (...) {
                // Fall through
            }
        }
    }

    Variable baseVar;
    if (m_symbolTable->lookupSymbol(base, baseVar) && baseVar.type.isClass()) {
        std::string className = baseVar.type.getClassName();
        std::string memberName = ctx->IDENTIFIER()->getText();

        auto methods = m_typeSystem->getClassMethods(className);
        for (const auto& method : methods) {
            if (method->isVirtual) {
                return "VIRTUAL_CALL:"+memberName+":"+base;
            }
            if (method->name==className+"_"+memberName) {
                return "METHOD_CALL:"+className+"_"+memberName+":"+base;
            }
        }

        try {
            std::string fieldAccess = resolveFieldAccess(memberName, className);
            return fieldAccess.replace(0, 4, base);  // Replace "this" with actual object
        }
        catch (...) {
            // Fall through
        }
    }

    return base+"->"+ctx->IDENTIFIER()->getText();
}

antlrcpp::Any TranspilerVisitor::visitAddressOfExpr(JBLangParser::AddressOfExprContext* ctx)
{
    return "&"+std::any_cast<std::string>(visit(ctx->expression()));
}

antlrcpp::Any TranspilerVisitor::visitDereferenceExpr(JBLangParser::DereferenceExprContext* ctx)
{
    return "*"+std::any_cast<std::string>(visit(ctx->expression()));
}

antlrcpp::Any TranspilerVisitor::visitTypedefDecl(JBLangParser::TypedefDeclContext* ctx)
{
    Type type = ctx->structDecl() ? getStructFromCode(ctx->structDecl()) : resolveTypeFromContext(
            ctx->typeSpec());
    m_typeSystem->registerTypeDef(ctx->IDENTIFIER()->getText(), type);
    m_output << m_codeGen->generateTypeDef(ctx->IDENTIFIER()->getText(), type);

    return nullptr;
}

antlrcpp::Any TranspilerVisitor::visitArrayAccessExpr(JBLangParser::ArrayAccessExprContext* ctx)
{
    auto array = std::any_cast<std::string>(visit(ctx->expression(0)));
    auto index = std::any_cast<std::string>(visit(ctx->expression(1)));
    return array+"["+index+"]";
}

antlrcpp::Any TranspilerVisitor::visitNewExpr(JBLangParser::NewExprContext* ctx)
{
    return m_codeGen->generateAlloc(resolveTypeFromContext(ctx->typeSpec()));
}

antlrcpp::Any TranspilerVisitor::visitArrayDecl(JBLangParser::ArrayDeclContext* ctx)
{
    Type type = getArrayFromCode(ctx);
    m_output << type.toString();
    return nullptr;
}

antlrcpp::Any TranspilerVisitor::visitStructInitExpr(JBLangParser::StructInitExprContext* ctx)
{
    std::string code = "{\n";
    code += std::any_cast<std::string>(visit(ctx->initializerList()));

    return code+m_symbolTable->getIndentLevel()+"}";
}

antlrcpp::Any TranspilerVisitor::visitPostIncrementExpr(JBLangParser::PostIncrementExprContext* ctx)
{
    std::string expr = std::any_cast<std::string>(visit(ctx->expression()));
    return expr+"++";
}

antlrcpp::Any TranspilerVisitor::visitPostDecrementExpr(JBLangParser::PostDecrementExprContext* ctx)
{
    std::string expr = std::any_cast<std::string>(visit(ctx->expression()));
    return expr+"--";
}

antlrcpp::Any TranspilerVisitor::visitPreIncrementExpr(JBLangParser::PreIncrementExprContext* ctx)
{
    std::string expr = std::any_cast<std::string>(visit(ctx->expression()));
    return "++"+expr;
}

antlrcpp::Any TranspilerVisitor::visitPreDecrementExpr(JBLangParser::PreDecrementExprContext* ctx)
{
    std::string expr = std::any_cast<std::string>(visit(ctx->expression()));
    return "--"+expr;
}

antlrcpp::Any TranspilerVisitor::visitInitializerList(JBLangParser::InitializerListContext* ctx)
{
    std::string code;
    Variable assignedFrom;
    std::string indentLevel = m_symbolTable->getIndentLevel();

    for (auto initializer : ctx->initializer()) {
        std::string assignExpr = initializer->expression()->getText();
        bool found = m_symbolTable->lookupSymbol(assignExpr, assignedFrom);
        if (found && assignedFrom.type.isPointer()) {
            m_output << indentLevel+m_codeGen->generateIncRef(assignedFrom);
        }
        code += indentLevel+"."+initializer->IDENTIFIER()->getText()+" = "+assignExpr+",\n";
    }
    return code;
}

Type TranspilerVisitor::resolveTypeFromContext(JBLangParser::TypeSpecContext* ctx)
{
    if (!ctx) return Type(Type::BaseType::Void);

    std::string typeText = ctx->getText();

    if (typeText.substr(0, 6)=="struct") {
        std::string structName = typeText.substr(6);

        bool isPointer = false;
        if (!structName.empty() && structName.back()=='*') {
            isPointer = true;
            structName = structName.substr(0, structName.length()-1);
        }

        Type structType = m_typeSystem->resolveType(structName);
        if (isPointer) {
            structType.setPointer(true);
        }
        return structType;
    }

    return m_typeSystem->resolveType(typeText);
}

std::vector<std::string> TranspilerVisitor::getClassNames() const
{
    std::vector<std::string> classNames;
    return classNames;
}

void TranspilerVisitor::generateClassMethodBodies()
{
    for (const auto& className : m_classNames) {
        auto constructor = m_typeSystem->getClassConstructor(className);
        if (constructor) {
            m_symbolTable->currentFunc = constructor;
            m_output << m_codeGen->generateFunctionDecl(constructor) << " {\n";

            if (m_typeSystem->hasVirtualMethods(className)) {
                m_output << "    this->vtable = &" << className << "_vtable;\n";
            }

            m_output << "}\n\n";
            m_symbolTable->currentFunc = nullptr;
        }

        auto methods = m_typeSystem->getClassMethods(className);
        for (const auto& method : methods) {
            m_symbolTable->currentFunc = method;
            m_output << m_codeGen->generateFunctionDecl(method);

            if (method->isVirtual) {
                m_output << " {\n    struct " << className << "* this = (struct " << className << "*)this_param;\n";
                m_output << "}\n\n";
            }
            else {
                m_output << " {\n}\n\n";
            }
            m_symbolTable->currentFunc = nullptr;
        }
    }
}
