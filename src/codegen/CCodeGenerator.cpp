#include "jblang/codegen/CCodeGenerator.h"

namespace {
std::string outputVariable(const Type& type, const std::string& name)
{
    if (type.isArray()) {
        return type.toString();
    }
    return type.toString()+" "+name;
}
}

std::string CCodeGenerator::generateFunctionDecl(const std::shared_ptr<Function>& func)
{
    if (!func) {
        throw std::runtime_error("Cannot generate declaration for null function");
    }
    if (func->isVirtual && !func->params.empty()) {
        std::string signature = func->returnType.toString()+" "+func->name+"(void* this_param";
        for (size_t i = 1; i<func->params.size(); ++i) {
            signature += ", "+func->params[i].second.toString()+" "+func->params[i].first;
        }
        return signature+")";
    }
    return func->getSignature();
}

std::string CCodeGenerator::generateVarDecl(const std::string& name, const Type& type,
        const std::string& initializer)
{
    return outputVariable(type, name)+initializer+";\n";
}

std::string CCodeGenerator::generateStructDecl(const std::string& name, const Type& type,
        const std::string& initializer)
{
    std::string structDecl = "struct "+name+" {\n";
    for (const auto& member : type.getStructMembers()) {
        structDecl += "\t";

        if (member.second.isArray()) {
            Type baseType = member.second;
            std::string baseTypeStr;

            if (baseType.isStruct()) {
                baseTypeStr = "struct "+baseType.getStructName();
            }
            else {
                baseTypeStr = Type::baseTypeToString(baseType.getBaseType());
            }
            if (baseType.isPointer()) {
                baseTypeStr += "*";
            }
            structDecl += baseTypeStr+" "+member.first;
            for (int size : baseType.getArraySizes()) {
                structDecl += "["+std::to_string(size)+"]";
            }
        }
        else {
            structDecl += member.second.toString()+" "+member.first;
        }
        structDecl += ";\n";
    }
    structDecl += "}";
    return structDecl;
}

std::string CCodeGenerator::generateTypeDef(const std::string& name, const Type& type)
{
    std::string typedef_ = "typedef ";
    typedef_ += type.isStruct() ? generateStructDecl(type.getStructName(), type) : type.toString();
    return typedef_+" "+name+";\n";
}

std::string CCodeGenerator::generateFunctionCall(const std::string& name,
        const std::vector<std::string>& args)
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

std::string CCodeGenerator::generateIncRef(const Variable& var, const std::string& other)
{
    if (!m_useRefCounts) {
        return "";
    }

    std::string code = "runtime_inc_ref_count("+var.name+", "+other;
    return code+");\n";
}

std::string CCodeGenerator::generateDecRef(const Variable& var)
{
    if (!m_useRefCounts) {
        return "";
    }

    std::string code = "runtime_dec_ref_count("+var.name+", ";
    if (!var.field_name.empty()) {
        code += "offsetof("+var.struct_name+", "+var.field_name+")";
    }
    else {
        code += "0";
    }
    return code+");\n";
}

std::string CCodeGenerator::generateCast(const std::string& expr, const Type& fromType, const Type& toType)
{
    if (fromType.isPointer() && toType.isPointer() &&
            fromType.isClass() && toType.isClass()) {

        std::string fromClass = fromType.getClassName();
        std::string toClass = toType.getClassName();

        if (fromClass==toClass) {
            return expr;
        }

        // Upcast
        return "&(("+expr+")->parent)";
    }

    return expr;
}