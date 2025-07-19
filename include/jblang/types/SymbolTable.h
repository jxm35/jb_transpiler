#ifndef SYMBOLTABLE_H
#define SYMBOLTABLE_H

#include <string>
#include <utility>
#include <vector>
#include <map>
#include <memory>
#include "jblang/types/TypeSystem.h"
#include "jblang/core/CompilerError.h"

class Variable {
public:
    Variable() = default;
    Variable(std::string name, Type type, std::string struct_name = "", std::string field_name = "")
            :name(std::move(name)), type(std::move(type)),
             struct_name(std::move(struct_name)), field_name(std::move(field_name)) { }

    std::string name;
    Type type;
    std::string struct_name;
    std::string field_name; // used so we can calculate offset at runtime to find the header
};

class SymbolTable {
public:
    SymbolTable()
    {
        scopes.emplace_back();
    }
    ~SymbolTable() = default;

    SymbolTable(const SymbolTable&) = delete;
    SymbolTable& operator=(const SymbolTable&) = delete;
    SymbolTable(SymbolTable&&) = default;
    SymbolTable& operator=(SymbolTable&&) = default;

    void enterScope()
    {
        scopes.emplace_back();
    }

    void exitScope()
    {
        if (!scopes.empty()) {
            scopes.pop_back();
        }
    }

    std::string getIndentLevel() const
    {
        return std::string(this->scopes.size()*4, ' ');
    }

    void addSymbol(const std::string& name, const Type& type,
            std::string struct_name = "", std::string field_name = "")
    {
        if (scopes.empty()) {
            throw CompilerError("No active scope");
        }

        Variable var(name, type, std::move(struct_name), std::move(field_name));
        scopes.back().symbols[name] = std::move(var);
    }

    bool lookupSymbol(const std::string& name, Variable& var) const
    {
        for (auto it = scopes.rbegin(); it!=scopes.rend(); ++it) {
            const auto found = it->symbols.find(name);
            if (found!=it->symbols.end()) {
                var = found->second;
                return true;
            }
        }

        if (currentFunc) {
            for (const auto& param : currentFunc->params) {
                if (param.first==name) {
                    var = Variable(name, param.second);
                    return true;
                }
            }
        }

        return false;
    }

    const std::map<std::string, Variable>& getCurrentScopeSymbols() const
    {
        static const std::map<std::string, Variable> empty;
        return scopes.empty() ? empty : scopes.back().symbols;
    }
    bool isGlobalScope() const { return scopes.size()==1; }

    std::vector<std::pair<std::string, Type>> globalVars;

    std::shared_ptr<Function> currentFunc;

private:
    struct Scope {
      std::map<std::string, Variable> symbols;
    };

    std::vector<Scope> scopes;
};

#endif //SYMBOLTABLE_H
