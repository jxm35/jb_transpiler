#ifndef SYMBOLTABLE_H
#define SYMBOLTABLE_H


#include <string>
#include <vector>
#include "TypeSystem.h"
#include "CompilerError.h"

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

    void addSymbol(const std::string& name, const Type& type) {
        if (scopes.empty()) {
            throw CompilerError("No active scope");
        }
        scopes.back().symbols[name] = type;
    }

    bool lookupSymbol(const std::string& name, Type& type) const {
        for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
            auto found = it->symbols.find(name);
            if (found != it->symbols.end()) {
                type = found->second;
                return true;
            }
        }
        return false;
    }

    std::map<std::string, Type> getCurrentScopeSymbols() const {
        return scopes.empty() ? std::map<std::string, Type>{} : scopes.back().symbols;
    }

private:
    struct Scope {
        std::map<std::string, Type> symbols;
    };
    std::vector<Scope> scopes;
};


#endif //SYMBOLTABLE_H
