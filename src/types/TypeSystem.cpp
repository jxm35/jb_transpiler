#include "jblang/types/TypeSystem.h"
#include <utility>

std::string outputVariable(const Type& type, const std::string& name)
{
    if (type.isArray()) {
        return type.toString();
    }
    return type.toString()+" "+name;
}

std::string Function::getSignature() const
{
    std::string funcName = this->name;
    if (funcName=="main") {
        funcName = "main_";
    }

    std::string signature = this->returnType.toString()+" "+funcName+"(";
    bool first = true;
    for (const auto& paramPair : this->params) {
        if (!first) {
            signature += ", ";
        }
        signature += outputVariable(paramPair.second, paramPair.first);
        first = false;
    }
    return signature+")";
}

std::string Type::baseTypeToString(Type::BaseType type)
{
    switch (type) {
    case Type::BaseType::Void: return "void";
    case Type::BaseType::Int: return "int";
    case Type::BaseType::String: return "char*";
    case Type::BaseType::Bool: return "bool";
    case Type::BaseType::Struct: return "struct";
    case Type::BaseType::Class: return "struct";
    default: throw CompilerError(CompilerError::ErrorType::TypeError, "unknown base type");
    }
}

std::string Type::toString() const
{
    std::string str;
    if (this->isStruct() || this->isClass()) {
        str = "struct "+this->getStructName();
    }
    else {
        str = baseTypeToString(this->m_baseType);
    }

    if (m_arrayInfo.isArray) {
        str += " "+m_arrayInfo.name;
        for (int size : m_arrayInfo.sizes) {
            str += "["+std::to_string(size)+"]";
        }
    }

    if (this->isPointer()) {
        str += "*";
    }
    return str;
}

Type TypeSystem::translateType(const std::string& sourceType)
{
    Type type;

    const auto arrayStart = sourceType.find('[');
    const bool isArray = arrayStart!=std::string::npos;
    std::string baseType = isArray ? sourceType.substr(0, arrayStart) : sourceType;

    const bool isPointer = sourceType.find('*')!=std::string::npos;
    if (isPointer) {
        baseType = sourceType.substr(0, sourceType.find('*'));
    }

    if (baseType=="void") {
        type = Type(Type::BaseType::Void);
    }
    else if (baseType=="int") {
        type = Type(Type::BaseType::Int);
    }
    else if (baseType=="string") {
        type = Type(Type::BaseType::String, true); // strings are char*
    }
    else if (baseType=="bool") {
        type = Type(Type::BaseType::Bool);
    }
    else if (m_typedefs.find(baseType)!=m_typedefs.end()) {
        type = m_typedefs[baseType];
    }
    else if (m_structs.find(baseType)!=m_structs.end()) {
        type = m_structs[baseType];
    }
    else if (m_classes.find(baseType)!=m_classes.end()) {
        type = m_classes[baseType];
    }
    else {
        throw std::runtime_error("Unknown type: "+sourceType);
    }

    if (isPointer) {
        type.setPointer(true);
    }

    if (isArray) {
        const auto arrayEnd = sourceType.find(']');
        if (arrayEnd!=std::string::npos) {
            const std::string sizeStr = sourceType.substr(arrayStart+1, arrayEnd-arrayStart-1);
            const int size = sizeStr.empty() ? 0 : std::stoi(sizeStr);
            type.setArray("james_fix", std::vector<int>{-1});
        }
        else {
            throw std::runtime_error("Invalid array type: "+sourceType);
        }
    }

    return type;
}

Type TypeSystem::registerStruct(const std::string& name)
{
    Type type(Type::BaseType::Struct);
    type.setStruct(name);
    m_structs[name] = type;
    return type;
}

Type TypeSystem::setStructMembers(const std::string& name, std::vector<std::pair<std::string, Type>> members)
{
    auto it = m_structs.find(name);
    if (it==m_structs.end()) {
        throw CompilerError(CompilerError::ErrorType::TypeError, "Struct not found: "+name);
    }
    Type& type = it->second;
    type.setStructMembers(std::move(members));
    return type;
}

void TypeSystem::registerTypeDef(const std::string& name, Type type)
{
    m_typedefs[name] = std::move(type);
}

void TypeSystem::registerFunction(std::shared_ptr<Function> func)
{
    if (!func) {
        throw CompilerError(CompilerError::ErrorType::Other, "Cannot register null function");
    }
    m_funcs[func->name] = std::move(func);
}

Type TypeSystem::registerClass(const std::string& name)
{
    Type type(Type::BaseType::Class);
    type.setStruct(name);
    m_classes[name] = type;
    return type;
}

Type TypeSystem::setClassMembers(const std::string& name, std::vector<std::pair<std::string, Type>> members)
{
    auto it = m_classes.find(name);
    if (it==m_classes.end()) {
        throw CompilerError(CompilerError::ErrorType::TypeError, "Class not found: "+name);
    }
    Type& type = it->second;
    type.setStructMembers(std::move(members));
    return type;
}

void TypeSystem::registerClassMethod(const std::string& className, std::shared_ptr<Function> method)
{
    if (!method) {
        throw CompilerError(CompilerError::ErrorType::Other, "Cannot register null method");
    }
    m_classMethods[className].push_back(std::move(method));
}

std::vector<std::shared_ptr<Function>> TypeSystem::getClassMethods(const std::string& className) const
{
    auto it = m_classMethods.find(className);
    return it!=m_classMethods.end() ? it->second : std::vector<std::shared_ptr<Function>>{};
}

void TypeSystem::registerClassConstructor(const std::string& className, std::shared_ptr<Function> constructor)
{
    if (!constructor) {
        throw CompilerError(CompilerError::ErrorType::Other, "Cannot register null constructor");
    }
    m_classConstructors[className] = std::move(constructor);
}

std::shared_ptr<Function> TypeSystem::getClassConstructor(const std::string& className) const
{
    auto it = m_classConstructors.find(className);
    return it!=m_classConstructors.end() ? it->second : nullptr;
}

Type TypeSystem::setClassParent(const std::string& className, const std::string& parentName)
{
    auto it = m_classes.find(className);
    if (it==m_classes.end()) {
        throw CompilerError(CompilerError::ErrorType::TypeError, "Class not found: "+className);
    }

    auto parentIt = m_classes.find(parentName);
    if (parentIt==m_classes.end()) {
        throw CompilerError(CompilerError::ErrorType::TypeError, "Parent class not found: "+parentName);
    }

    Type& type = it->second;
    type.setParentClass(parentName);
    return type;
}

bool TypeSystem::isSubclassOf(const std::string& child, const std::string& parent) const
{
    auto it = m_classes.find(child);
    if (it==m_classes.end()) return false;

    std::string current = child;
    while (true) {
        auto currentIt = m_classes.find(current);
        if (currentIt==m_classes.end()) break;

        if (!currentIt->second.hasParent()) break;

        std::string parentClass = currentIt->second.getParentClass();
        if (parentClass==parent) return true;
        current = parentClass;
    }
    return false;
}

std::vector<std::pair<std::string, Type>> TypeSystem::getAllClassMembers(const std::string& className) const
{
    auto it = m_classes.find(className);
    if (it==m_classes.end()) {
        throw CompilerError(CompilerError::ErrorType::TypeError, "Class not found: "+className);
    }

    std::vector<std::pair<std::string, Type>> allMembers;

    if (it->second.hasParent()) {
        auto parentMembers = getAllClassMembers(it->second.getParentClass());
        allMembers.insert(allMembers.end(), parentMembers.begin(), parentMembers.end());
    }

    const auto& ownMembers = it->second.getStructMembers();
    allMembers.insert(allMembers.end(), ownMembers.begin(), ownMembers.end());

    return allMembers;
}
