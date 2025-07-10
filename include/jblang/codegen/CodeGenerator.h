#ifndef CODEGENERATOR_H
#define CODEGENERATOR_H

#include <string>
#include <map>
#include <vector>
#include <memory>
#include "jblang/types/TypeSystem.h"
#include "jblang/types/SymbolTable.h"

class CodeGenerator {
public:
    CodeGenerator() = default;
    virtual ~CodeGenerator() = default;

    CodeGenerator(const CodeGenerator&) = delete;
    CodeGenerator& operator=(const CodeGenerator&) = delete;
    CodeGenerator(CodeGenerator&&) = default;
    CodeGenerator& operator=(CodeGenerator&&) = default;

    virtual std::string generateFunctionDecl(const std::shared_ptr<Function>& func) = 0;
    virtual std::string generateVarDecl(const std::string& name, const Type& type,
            const std::string& initializer = "") = 0;
    virtual std::string generateStructDecl(const std::string& name, const Type& type,
            const std::string& initializer = "") = 0;
    virtual std::string generateTypeDef(const std::string& name, const Type& type) = 0;
    virtual std::string generateFunctionCall(const std::string& name,
            const std::vector<std::string>& args) = 0;
    virtual std::string generateReturn(const std::string& value, const Type& type) = 0;
    virtual std::string generateScopeEntry() = 0;
    virtual std::string generateScopeExit(const std::map<std::string, Variable>& scopeVars) = 0;
    virtual std::string generateIncRef(const Variable& var, const std::string& other = "NULL") = 0;
    virtual std::string generateDecRef(const Variable& var) = 0;
    virtual std::string generateAlloc(const Type& type) = 0;
};

#endif //CODEGENERATOR_H
