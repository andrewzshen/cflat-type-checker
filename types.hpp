#ifndef TYPES_HPP
#define TYPES_HPP

#include <vector>
#include <string>
#include <memory>
#include <unordered_map>

struct Type;
struct NilType;
struct StructType;
struct ArrayType;
struct PointerType;
struct FunctionType;

bool typesEqual(const std::shared_ptr<Type> &lhs, const std::shared_ptr<Type> &rhs);
bool typePointersEqual(const std::shared_ptr<Type> &lhs, const std::shared_ptr<Type> &rhs);
std::shared_ptr<Type> pickNonNil(const std::shared_ptr<Type> &lhs, const std::shared_ptr<Type> &rhs);

using Gamma = std::unordered_map<std::string, std::shared_ptr<Type>>;
using Delta = std::unordered_map<std::string, std::unordered_map<std::string, std::shared_ptr<Type>>>;

struct Type {
    virtual ~Type() = default;
    virtual std::string toString() const = 0;
    virtual bool equals(const Type& other) const = 0;
};

struct IntType : Type {
    std::string toString() const override; 
    bool equals(const Type& other) const override; 
};

struct NilType : Type {
    std::string toString() const override;
    bool equals(const Type& other) const override;
};

struct StructType : Type {
    std::string name;
    
    StructType(std::string n); 
    
    std::string toString() const override;
    bool equals(const Type& other) const override;
};

struct ArrayType : Type {
    std::shared_ptr<Type> elementType;
    
    ArrayType(std::shared_ptr<Type> elementType);
    
    std::string toString() const override; 
    bool equals(const Type& other) const override;
};

struct PointerType : Type {
    std::shared_ptr<Type> pointeeType;
    
    PointerType(std::shared_ptr<Type> pointeeType);
    
    std::string toString() const override;
    bool equals(const Type& other) const override;
};

struct FunctionType : Type {
    std::vector<std::shared_ptr<Type>> paramTypes;
    std::shared_ptr<Type> returnType;
    
    FunctionType(std::vector<std::shared_ptr<Type>> paramTypes, std::shared_ptr<Type> returnType);
    
    std::string toString() const override;
    bool equals(const Type& other) const override;
};

#endif
