// TranspilerVisitor.h
#pragma once
#include "JBLangBaseVisitor.h"
#include <string>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <memory>

struct Function {
    std::string name;
    std::vector<std::string> paramTypes;
    std::string returnType;
};

class TranspilerVisitor : public JBLangBaseVisitor {
public:
    TranspilerVisitor() : tempVarCounter(0) {}

    virtual antlrcpp::Any visitProgram(JBLangParser::ProgramContext *ctx) override {
        output << "#include \"runtime.h\"\n";
        output << "#include <stdio.h>\n\n";

        for (auto stmt : ctx->statement()) {
            visit(stmt);
            output << "\n";
        }

        return output.str();
    }

    virtual antlrcpp::Any visitFunctionDecl(JBLangParser::FunctionDeclContext *ctx) override {
        auto func = std::make_shared<Function>();
        func->name = ctx->IDENTIFIER()->getText();

        // Build parameter list
        std::stringstream params;
        if (ctx->paramList()) {
            for (auto param : ctx->paramList()->param()) {
                if (!func->paramTypes.empty()) {
                    params << ", ";
                }
                std::string type = translateType(param->typeSpec()->getText());
                func->paramTypes.push_back(type);
                params << type << " " << param->IDENTIFIER()->getText();
            }
        }

        // Handle return type
        func->returnType = ctx->typeSpec() ?
                           translateType(ctx->typeSpec()->getText()) : "void";
        functions[func->name] = func;

        // Generate function
        output << func->returnType << " "
               << func->name << "(" << params.str() << ") ";
        visit(ctx->block());

        return nullptr;
    }

    virtual antlrcpp::Any visitSpawnStmt(JBLangParser::SpawnStmtContext *ctx) override {
        auto* exprCtx = ctx->expression();
        auto* funcCallCtx = dynamic_cast<JBLangParser::FuncCallExprContext*>(exprCtx);
        if (!funcCallCtx) {
            throw std::runtime_error("Can only spawn function calls");
        }

        auto* funcCall = funcCallCtx->functionCall();
        std::string funcName = funcCall->IDENTIFIER()->getText();
        auto func = functions[funcName];

        // Generate spawn wrapper
        std::string wrapperName = funcName + "_wrapper_" + std::to_string(tempVarCounter++);
        generateSpawnWrapper(wrapperName, funcCall, func);

        output << "runtime_spawn(" << wrapperName << ", NULL);\n";

        return nullptr;
    }

private:
    void generateSpawnWrapper(const std::string& wrapperName,
                              JBLangParser::FunctionCallContext* funcCall,
                              std::shared_ptr<Function> func) {
        output << "void " << wrapperName << "(void* arg) {\n";

        // Call original function
        output << "    " << func->name << "(";
        if (funcCall->argumentList()) {
            std::vector<std::string> args;
            for (auto expr : funcCall->argumentList()->expression()) {
                // Fix: Use std::any_cast instead of .as<T>()
                args.push_back(std::any_cast<std::string>(visit(expr)));
            }
            for (size_t i = 0; i < args.size(); ++i) {
                if (i > 0) output << ", ";
                output << args[i];
            }
        }
        output << ");\n";
        output << "}\n\n";
    }

    std::string translateType(const std::string& sourceType) {
        static const std::unordered_map<std::string, std::string> typeMap = {
                {"int", "int"},
                {"string", "char*"},
                {"bool", "bool"}
        };

        auto it = typeMap.find(sourceType);
        return it != typeMap.end() ? it->second : sourceType;
    }

    std::stringstream output;
    std::unordered_map<std::string, std::shared_ptr<Function>> functions;
    int tempVarCounter;
};