#ifndef SYMBOLTABLE_H
#define SYMBOLTABLE_H


#include <string>
#include <utility>
#include <vector>
#include "TypeSystem.h"
#include "CompilerError.h"

class Variable { ;
public:
    std::string name;
    Type type;
    std::string struct_name;
    std::string field_name; // used so we can calculate offset at runtime to find the header
};

class SymbolTable {
public:
    void enterScope() {
        scopes.push_back(Scope{});
    }

    void exitScope() {
        if (!scopes.empty()) {
            scopes.pop_back();
        }
    }

    std::string getIndentLevel() {
        return std::string(this->scopes.size()*4, ' ');
    }

    void    addSymbol(const std::string& name, const Type& type, std::string struct_name = "", std::string field_name = "") {
        if (scopes.empty()) {
            throw CompilerError("No active scope");
        }
        Variable v = {
                .name = name,
                .type = type,
                .struct_name = std::move(struct_name),
                .field_name = std::move(field_name),
        };
        scopes.back().symbols[name] = v;
    }

    bool lookupSymbol(const std::string& name, Variable& var) const {
        for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
            auto found = it->symbols.find(name);
            if (found != it->symbols.end()) {
                var = found->second;
                return true;
            }
        }
        for (const auto& param : currentFunc->params) {
            if (param.first == name) {
                var = Variable{
                    .name = name,
                    .type = param.second,
                };
                return true;
            }
        }
        return false;
    }

    std::map<std::string, Variable> getCurrentScopeSymbols() const {
        return scopes.empty() ? std::map<std::string, Variable>{} : scopes.back().symbols;
    }
    std::shared_ptr<Function> currentFunc;
private:
    struct Scope {
        std::map<std::string, Variable> symbols;
    };
    std::vector<Scope> scopes;
};


#endif //SYMBOLTABLE_H
