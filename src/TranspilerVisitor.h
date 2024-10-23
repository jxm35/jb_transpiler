// TranspilerVisitor.h
#pragma once
#include "JBLangBaseVisitor.h"
#include <string>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <memory>

//#define output std::cout

struct Function {
    std::string name;
    std::vector<std::string> paramTypes;
    std::string paramString;
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
    virtual antlrcpp::Any visitPrimary(JBLangParser::PrimaryContext *ctx) override {
        if (ctx->expression()) {
            // If there's an expression in parentheses, evaluate it
            return visit(ctx->expression());
        } else if (ctx->IDENTIFIER()) {
            // Return the variable name
            return ctx->IDENTIFIER()->getText();
        } else {
            // This is a literal, pass it to the appropriate visitor method
            return visit(ctx->literal());
        }
    }

    virtual antlrcpp::Any visitLiteralExpr(JBLangParser::LiteralExprContext *ctx) override {
        return visit(ctx->literal());
    }

    virtual antlrcpp::Any visitLiteral(JBLangParser::LiteralContext *ctx) override {
        if (ctx->INTEGER()) {
            return ctx->INTEGER()->getText();
        } else if (ctx->STRING()) {
            return ctx->STRING()->getText();
        } else if (ctx->getText() == "true" || ctx->getText() == "false") {
            return ctx->getText();
        }

        return "";
    }


    virtual antlrcpp::Any visitFunctionCall(JBLangParser::FunctionCallContext *ctx) override {
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

    virtual antlrcpp::Any visitBlock(JBLangParser::BlockContext *ctx) override {
        output << "{\n";
        for (auto stmt : ctx->statement()) {
            visit(stmt);
            output << "\n";
        }
        output << "}\n";
        return nullptr;
    }


    virtual antlrcpp::Any visitVarDecl(JBLangParser::VarDeclContext *ctx) override {
        std::string varName = ctx->IDENTIFIER()->getText();
        std::string type = ctx->typeSpec() ? translateType(ctx->typeSpec()->getText()) : "int";
        std::string expr = ctx->expression() ? std::any_cast<std::string>(visit(ctx->expression())) : "";

        if (!expr.empty()) {
            output << type << " " << varName << " = " << expr << ";";
        } else {
            output << type << " " << varName << ";";
        }

        return nullptr;
    }

    virtual antlrcpp::Any visitMemberExpr(JBLangParser::MemberExprContext *ctx) override {
        std::string obj = std::any_cast<std::string>(visit(ctx->expression()));
        std::string member = ctx->IDENTIFIER()->getText();
        return obj + "." + member;
    }

    virtual antlrcpp::Any visitFuncCallExpr(JBLangParser::FuncCallExprContext *ctx) override {
        return visit(ctx->functionCall());
    }

    virtual antlrcpp::Any visitMulDivExpr(JBLangParser::MulDivExprContext *ctx) override {
        std::string left = std::any_cast<std::string>(visit(ctx->expression(0)));
        std::string right = std::any_cast<std::string>(visit(ctx->expression(1)));
        std::string op = ctx->op->getText();
        return left + " " + op + " " + right;
    }

    virtual antlrcpp::Any visitAddSubExpr(JBLangParser::AddSubExprContext *ctx) override {
        std::string left = std::any_cast<std::string>(visit(ctx->expression(0)));
        std::string right = std::any_cast<std::string>(visit(ctx->expression(1)));
        std::string op = ctx->op->getText();
        return left + " " + op + " " + right;
    }

    virtual antlrcpp::Any visitCompareExpr(JBLangParser::CompareExprContext *ctx) override {
        std::string left = std::any_cast<std::string>(visit(ctx->expression(0)));
        std::string right = std::any_cast<std::string>(visit(ctx->expression(1)));
        std::string op = ctx->op->getText();
        return left + " " + op + " " + right;
    }

    virtual antlrcpp::Any visitAssignExpr(JBLangParser::AssignExprContext *ctx) override {
        std::string left = ctx->expression(0)->getText();
        std::string right = std::any_cast<std::string>(visit(ctx->expression(1)));
        return left + " = " + right;
    }

    virtual antlrcpp::Any visitExprStmt(JBLangParser::ExprStmtContext *ctx) override {
        output << std::any_cast<std::string>(visit(ctx->expression())) << ";";
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

        output << "runtime_spawn(" << wrapperName << ", NULL);";

        return nullptr;
    }

    virtual antlrcpp::Any visitReturnStmt(JBLangParser::ReturnStmtContext *ctx) override {
        std::string returnValue = ctx->expression() ? std::any_cast<std::string>(visit(ctx->expression())) : "";

        if (!returnValue.empty()) {
            output << "return " << returnValue << ";";
        } else {
            output << "return;";
        }

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