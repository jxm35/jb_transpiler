#ifndef CCODEGENERATOR_H
#define CCODEGENERATOR_H

#include "jblang/codegen/CodeGenerator.h"

class CCodeGenerator : public CodeGenerator {
public:
    explicit CCodeGenerator(bool useRefCounts) noexcept
            :m_useRefCounts(useRefCounts) { }

    ~CCodeGenerator() override = default;

    std::string generateFunctionDecl(const std::shared_ptr<Function>& func) override;
    std::string generateVarDecl(const std::string& name, const Type& type,
            const std::string& initializer = "") override;
    std::string generateStructDecl(const std::string& name, const Type& type,
            const std::string& initializer = "") override;
    std::string generateTypeDef(const std::string& name, const Type& type) override;
    std::string generateFunctionCall(const std::string& name,
            const std::vector<std::string>& args) override;
    std::string generateReturn(const std::string& value, const Type& type) override;
    std::string generateScopeEntry() override;
    std::string generateScopeExit(const std::map<std::string, Variable>& scopeVars) override;
    std::string generateIncRef(const Variable& var, const std::string& other = "NULL") override;
    std::string generateDecRef(const Variable& var) override;
    std::string generateAlloc(const Type& type) override;

private:
    const bool m_useRefCounts;
};

#endif //CCODEGENERATOR_H
