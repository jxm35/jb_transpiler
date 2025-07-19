#ifndef TYPESYSTEM_H
#define TYPESYSTEM_H

#include <string>
#include <map>
#include <utility>
#include <vector>
#include <memory>

#include "jblang/core/CompilerError.h"

class Type {
public:
    enum class BaseType {
      Void,
      Int,
      String,
      Bool,
      Struct,
      Class,
      NO_TYPE
    };

    struct ArrayInfo {
      bool isArray = false;
      std::vector<int> sizes;
      std::string name;
    };

    Type() noexcept
            :m_baseType(BaseType::Void), m_isPointer(false), m_isConst(false) { }

    explicit Type(BaseType base) noexcept
            :m_baseType(base), m_isPointer(false), m_isConst(false) { }

    Type(BaseType base, bool isPointer, bool isConst = false) noexcept
            :m_baseType(base), m_isPointer(isPointer), m_isConst(isConst) { }

    bool isPointer() const noexcept { return m_isPointer; }
    bool isConst() const noexcept { return m_isConst; }
    bool isArray() const noexcept { return m_arrayInfo.isArray; }
    bool isStruct() const noexcept { return m_baseType==BaseType::Struct; }
    bool isClass() const noexcept { return m_baseType==BaseType::Class; }
    const std::string& getClassName() const noexcept { return m_structName; }

    BaseType getBaseType() const noexcept { return m_baseType; }
    const std::string& getStructName() const noexcept { return m_structName; }
    const std::string& getArrayName() const noexcept { return m_arrayInfo.name; }
    const std::vector<std::pair<std::string, Type>>& getStructMembers() const noexcept
    {
        return m_structMembers;
    }
    static std::string baseTypeToString(BaseType type);
    std::string toString() const;

    void setPointer(bool isPtr) noexcept { m_isPointer = isPtr; }
    void setArray(std::string arrayName, std::vector<int> sizes)
    {
        m_arrayInfo.isArray = true;
        m_arrayInfo.sizes = std::move(sizes);
        m_arrayInfo.name = std::move(arrayName);
    }
    void setStruct(std::string name)
    {
        m_structName = std::move(name);
    }
    void setStructMembers(std::vector<std::pair<std::string, Type>> members)
    {
        m_structMembers = std::move(members);
    }

    const std::vector<int>& getArraySizes() const noexcept
    {
        return m_arrayInfo.sizes;
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
    Function() noexcept
            :returnType(Type::BaseType::Void), isStatic(false), block(nullptr) { }

    Function(const Function& other) = default;
    Function(Function&& other) noexcept = default;
    Function& operator=(const Function& other) = default;
    Function& operator=(Function&& other) noexcept = default;

    std::string getSignature() const;

    std::string name;
    void* block;
    std::vector<std::pair<std::string, Type>> params;
    Type returnType;
    bool isStatic;
};

class TypeSystem {
public:
    TypeSystem() = default;
    ~TypeSystem() = default;

    TypeSystem(const TypeSystem&) = delete;
    TypeSystem& operator=(const TypeSystem&) = delete;
    TypeSystem(TypeSystem&&) = default;
    TypeSystem& operator=(TypeSystem&&) = default;

    Type resolveType(const std::string& typeStr)
    {
        return translateType(typeStr);
    }

    Type registerStruct(const std::string& name);
    Type registerClass(const std::string& name);
    Type setStructMembers(const std::string& name, std::vector<std::pair<std::string, Type>> members);
    Type setClassMembers(const std::string& name, std::vector<std::pair<std::string, Type>> members);
    void registerClassMethod(const std::string& className, std::shared_ptr<Function> method);
    void registerClassConstructor(const std::string& className, std::shared_ptr<Function> constructor);
    std::shared_ptr<Function> getClassConstructor(const std::string& className) const;
    std::vector<std::shared_ptr<Function>> getClassMethods(const std::string& className) const;
    void registerTypeDef(const std::string& name, Type type);
    void registerFunction(std::shared_ptr<Function> func);

    const std::string& getDefineValue(const std::string& key) const
    {
        static const std::string empty;
        auto it = m_defines.find(key);
        return it!=m_defines.end() ? it->second : empty;
    }

    void registerDefine(const std::string& key, std::string val)
    {
        m_defines[key] = std::move(val);
    }

    bool isCompatible(const Type& from, const Type& to) const noexcept
    {
        return false;
    }

    Type getCommonType(const Type& t1, const Type& t2) const noexcept
    {
        return t1;
    }

private:
    std::map<std::string, Type> m_typedefs;
    std::map<std::string, std::shared_ptr<Function>> m_funcs;
    std::map<std::string, std::string> m_defines;
    std::map<std::string, Type> m_structs;
    std::map<std::string, Type> m_classes;
    std::map<std::string, std::vector<std::shared_ptr<Function>>> m_classMethods;
    std::map<std::string, std::shared_ptr<Function>> m_classConstructors;

    Type translateType(const std::string& sourceType);
};

#endif //TYPESYSTEM_H
