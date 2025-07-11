#include <gtest/gtest.h>
#include "jblang/types/TypeSystem.h"
#include "jblang/codegen/CCodeGenerator.h"

TEST(CoreTest, TypeResolution)
{
    TypeSystem ts;
    EXPECT_EQ(ts.resolveType("int").getBaseType(), Type::BaseType::Int);
    EXPECT_EQ(ts.resolveType("bool").getBaseType(), Type::BaseType::Bool);
    EXPECT_EQ(ts.resolveType("void").getBaseType(), Type::BaseType::Void);
    EXPECT_TRUE(ts.resolveType("int*").isPointer());
    EXPECT_TRUE(ts.resolveType("string").isPointer()); // char*
}

TEST(CoreTest, FunctionCodeGen)
{
    CCodeGenerator gen(false);
    auto func = std::make_shared<Function>();
    func->name = "test";
    func->returnType = Type(Type::BaseType::Int);
    EXPECT_EQ(gen.generateFunctionDecl(func), "int test()");

    func->params = {{"a", Type(Type::BaseType::Int)}, {"b", Type(Type::BaseType::Bool)}};
    EXPECT_EQ(gen.generateFunctionDecl(func), "int test(int a, bool b)");

    func->name = "main";
    EXPECT_TRUE(gen.generateFunctionDecl(func).find("main_")!=std::string::npos);
}

TEST(CoreTest, StructHandling)
{
    TypeSystem ts;
    auto structType = ts.registerStruct("Point");
    EXPECT_TRUE(structType.isStruct());
    EXPECT_EQ(structType.getStructName(), "Point");

    std::vector<std::pair<std::string, Type>> members = {
            {"x", Type(Type::BaseType::Int)},
            {"y", Type(Type::BaseType::Int)}
    };
    auto updatedType = ts.setStructMembers("Point", members);
    EXPECT_EQ(updatedType.getStructMembers().size(), 2);
    EXPECT_EQ(updatedType.getStructMembers()[0].first, "x");
}

TEST(CoreTest, VarDeclaration)
{
    CCodeGenerator gen(false);
    auto result = gen.generateVarDecl("x", Type(Type::BaseType::Int));
    EXPECT_EQ(result, "int x;\n");

    // Test with initializer
    auto result2 = gen.generateVarDecl("y", Type(Type::BaseType::Bool), " = true");
    EXPECT_EQ(result2, "bool y = true;\n");

    // Test pointer variable
    Type ptrType(Type::BaseType::Int);
    ptrType.setPointer(true);
    auto result3 = gen.generateVarDecl("ptr", ptrType);
    EXPECT_EQ(result3, "int* ptr;\n");
}

TEST(CoreTest, AllocationGen)
{
    CCodeGenerator gen(false);
    auto result = gen.generateAlloc(Type(Type::BaseType::Int));
    EXPECT_EQ(result, "runtime_alloc(sizeof(int))");

    Type structType(Type::BaseType::Struct);
    structType.setStruct("Point");
    auto result2 = gen.generateAlloc(structType);
    EXPECT_EQ(result2, "runtime_alloc(sizeof(Point))");

    auto result3 = gen.generateFunctionCall("printf", {"\"hello\""});
    EXPECT_EQ(result3, "printf(\"hello\")");
}