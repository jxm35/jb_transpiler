#include "TypeSystem.h"

std::string outputVariable(const Type& type, const std::string& name) {
    if (type.isArray()) {
        return type.toString();
    } else {
        return type.toString() + " " + name;
    }
}

std::string Function::getSignature() const {
    std::string name = this->name;
    if (name == "main") {
        name = "main_";
    }
    std::string signature =  this->returnType.toString() + " " + name + "(";
    bool first = true;
    for (const std::pair<std::string, Type>& paramPair : this->params) {
        if (!first) {
            signature += ", ";
        }
        signature += outputVariable(paramPair.second, paramPair.first);
        first = false;
    }
    return signature + ")";
}

std::string baseTypeToString(const Type::BaseType& type) {
    switch (type) {
        case Type::BaseType::Void:
            return "void";
        case Type::BaseType::Int:
            return "int";
        case Type::BaseType::String:
            return "char*";
        case Type::BaseType::Bool:
            return "bool";
        case Type::BaseType::Struct:
            return "struct";
        default:
            throw CompilerError(CompilerError::ErrorType::TypeError, "unknown base type");
    }
}
std::string Type::toString() const {
    std::string str;
    if (this->isStruct()) {
        str = this->getStructName();
    } else {
        str = baseTypeToString(this->m_baseType);
    }

    if (m_arrayInfo.isArray) {
        str += " " + m_arrayInfo.name;
        for (int size : m_arrayInfo.sizes) {
            str += "[" + std::to_string(size) + "]";

        }
    }

    if (this->isPointer()) {
        str += "*";
    }
    return str;
}

Type TypeSystem::translateType(const std::string& sourceType) {
    Type type;

    size_t arrayStart = sourceType.find('[');
    bool isArray = arrayStart != std::string::npos;
    std::string baseType = isArray ? sourceType.substr(0, arrayStart) : sourceType;

    bool isPointer = sourceType.find('*') != std::string::npos;
    baseType = isPointer ? sourceType.substr(0, sourceType.find('*')) : sourceType;

    if (baseType == "void") type = Type(Type::BaseType::Void);
    else if (baseType == "int") type = Type(Type::BaseType::Int);
    else if (baseType == "string") type = Type(Type::BaseType::String, true); // strings are char*
    else if (baseType == "bool") type = Type(Type::BaseType::Bool);
    else if (m_typedefs.find(baseType) != m_typedefs.end())type = m_typedefs[baseType];
    else throw std::runtime_error("Unknown type: " + sourceType);

    if (isPointer) {
        type.setPointer(true);
    }

    if (isArray) {
        size_t arrayEnd = sourceType.find(']');
        if (arrayEnd != std::string::npos) {
            std::string sizeStr = sourceType.substr(arrayStart + 1, arrayEnd - arrayStart - 1);
            int size = sizeStr.empty() ? 0 : std::stoi(sizeStr);
            type.setArray("james_fix", std::vector<int>{-1});
        } else {
            throw std::runtime_error("Invalid array type: " + sourceType);
        }
    }

    return type;
}

Type TypeSystem::registerStruct(std::string name, std::vector<std::pair<std::string, Type>> members) {
    Type type = Type(Type::BaseType::Struct);
    type.setStruct(name, members);
    m_structs[name] = type;
    return type;

}

void TypeSystem::registerTypeDef(std::string name, Type type) {
    m_typedefs[name] = type;
}

void TypeSystem::registerFunction(const std::shared_ptr<Function> &func) {
    m_funcs[func->name] = func;
}
