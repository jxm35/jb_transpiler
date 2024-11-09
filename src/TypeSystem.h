#ifndef TYPESYSTEM_H
#define TYPESYSTEM_H

#include <string>
#include <map>
#include <utility>
#include <vector>

#include "CompilerError.h"

class Type {
public:
    enum class BaseType {
        Void,
        Int,
        String,
        Bool,
        Struct,
        NO_TYPE
    };
    struct ArrayInfo {
        bool isArray = false;
        std::vector<int> sizes;
        std::string name;
    };

    Type() : m_baseType(BaseType::Void), m_isPointer(false), m_isConst(false){}
    explicit Type(BaseType base) : m_baseType(base), m_isPointer(false), m_isConst(false) {}

    Type(BaseType base, bool isPointer, bool isConst = false, bool isStruct = false)
            : m_baseType(base), m_isPointer(isPointer), m_isConst(isConst) {}
    bool isPointer() const { return m_isPointer; }

    bool isConst() const { return m_isConst; }
    BaseType getBaseType() const { return m_baseType; }
    std::string toString() const;
    void setPointer(bool isPtr) { m_isPointer = true; }
    bool isArray() const { return m_arrayInfo.isArray; }
    bool isStruct() const { return m_baseType == BaseType::Struct; }
    std::string getStructName() const {return m_structName;}
    std::string getArrayName() const {return m_arrayInfo.name;}
    std::vector<std::pair<std::string, Type>> getStructMembers() const {
        return m_structMembers;
    }

    void setArray(std::string arrayName, std::vector<int> sizes) {
        m_arrayInfo.isArray = true;
        m_arrayInfo.sizes = std::move(sizes);
        m_arrayInfo.name = std::move(arrayName);
    }

    void setStruct(const std::string& name) {
        m_structName = name;
    }

    void setStructMembers(std::vector<std::pair<std::string, Type>> members) {
        m_structMembers = std::move(members);
    }

private:
    ArrayInfo m_arrayInfo;
    BaseType m_baseType;
    bool m_isPointer;
    bool m_isConst;
    std::string m_structName;
    std::vector<std::pair<std::string, Type>> m_structMembers;
};

class Function {
public:
    Function() : returnType(Type::BaseType::Void), isStatic(false) {}

    Function(const Function& other) = default;

    std::string name;
    void* block;
    std::vector<std::pair<std::string, Type>> params;
    Type returnType;
    bool isStatic;

    std::string getSignature() const;
};

class TypeSystem {
public:
    Type resolveType(const std::string& typeStr) {
        return translateType(typeStr);
    }

    Type  registerStruct(const std::string&);
    Type  setStructMembers(const std::string&, std::vector<std::pair<std::string, Type>>);

    void registerTypeDef(const std::string& name, Type type);

    std::string getDefineValue(const std::string& key) {
        return m_defines[key];
    }
    void registerDefine(const std::string& key, std::string val) {
        m_defines[key] = std::move(val);
    }

    void registerFunction(const std::shared_ptr<Function>& func);

    bool isCompatible(const Type& from, const Type& to) {
        return false;
    }

    Type getCommonType(const Type& t1, const Type& t2) {
        return t1;
    }
private:
    std::map<std::string, Type> m_typedefs;
    std::map<std::string, std::shared_ptr<Function>> m_funcs;
    std::map<std::string, std::string> m_defines;
    std::map<std::string, Type> m_structs;
    Type translateType(const std::string& sourceType);
};


#endif //TYPESYSTEM_H
