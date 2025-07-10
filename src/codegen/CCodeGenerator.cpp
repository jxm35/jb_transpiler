#include "jblang/codegen/CCodeGenerator.h"

std::string outputVariable_(const Type& type, const std::string& name)
{
    if (type.isArray()) {
        return type.toString();
    }
    else {
        return type.toString()+" "+name;
    }
}

std::string CCodeGenerator::generateFunctionDecl(const std::shared_ptr<Function>& func)
{
    return func->getSignature();
}

std::string CCodeGenerator::generateVarDecl(const std::string& name, const Type& type, const std::string& initializer)
{
    return outputVariable_(type, name)+initializer+";\n";
}

std::string
CCodeGenerator::generateStructDecl(const std::string& name, const Type& type, const std::string& initializer)
{
    std::string structDecl = "struct "+name+"{\n";
    for (auto member : type.getStructMembers()) {
        structDecl += "\t"+member.second.toString()+(member.second.isArray() ? "" : ("\t"+member.first))+";\n";
    }
    return structDecl+"}\n";
}

std::string CCodeGenerator::generateTypeDef(const std::string& name, const Type& type)
{
    std::string typedef_ = "typedef ";
    typedef_ += type.isStruct() ? generateStructDecl(type.getStructName(), type) : type.toString();
    return typedef_+" "+name+";\n";
}

std::string CCodeGenerator::generateFunctionCall(const std::string& name, const std::vector<std::string>& args)
{
    std::string funcCall = name+"(";
    bool first = true;
    for (const auto& arg : args) {
        if (!first) {
            funcCall += ", ";
        }
        funcCall += arg;
        first = false;
    }
    return funcCall+")";
}

std::string CCodeGenerator::generateReturn(const std::string& value, const Type& type)
{
    return "return "+value+";\n";
}

std::string CCodeGenerator::generateScopeEntry()
{
    return "{\n";
}

std::string CCodeGenerator::generateScopeExit(const std::map<std::string, Variable>& scopeVars)
{
    return "}\n";
}

std::string CCodeGenerator::generateAlloc(const Type& type)
{
    return "runtime_alloc(sizeof("+type.toString()+"))";
}

std::string CCodeGenerator::generateIncRef(const Variable& var, std::string other)
{
    if (!m_useRefCounts) return "";
    std::string code = "runtime_inc_ref_count("+var.name;
    code += ", "+other;
    return code+");\n";
}

std::string CCodeGenerator::generateDecRef(const Variable& var)
{
    if (!m_useRefCounts) return "";
    std::string code = "runtime_dec_ref_count("+var.name+", ";
    if (!var.field_name.empty()) {
        code += "offsetof("+var.struct_name+", "+var.field_name+")";
    }
    else {
        code += "0";
    }
    return code+");\n";
}
