#include "types.hpp"

#include <sstream>

/* Helper functions */
bool typesEqual(const std::shared_ptr<Type> &lhs, const std::shared_ptr<Type> &rhs) {
    if (!lhs || !rhs) return false;
    if (dynamic_cast<const NilType*>(lhs.get())) {
        return dynamic_cast<const NilType*>(rhs.get()) != nullptr ||
               dynamic_cast<const ArrayType*>(rhs.get()) != nullptr ||
               dynamic_cast<const PointerType*>(rhs.get()) != nullptr;
    }
    if (dynamic_cast<const NilType*>(rhs.get())) {
        return dynamic_cast<const ArrayType*>(lhs.get()) != nullptr || 
               dynamic_cast<const PointerType *>(lhs.get()) != nullptr;
    }
    return lhs->equals(*rhs);
}

bool typePointersEqual(const std::shared_ptr<Type> &lhs, const std::shared_ptr<Type> &rhs) {
    if (!lhs && !rhs) return true;
    if (!lhs || !rhs) return false;
    return lhs->equals(*rhs);
}

std::shared_ptr<Type> pickNonNil(const std::shared_ptr<Type> &lhs, const std::shared_ptr<Type> &rhs) {
    return !dynamic_cast<const NilType*>(lhs.get()) ? lhs : rhs;
}

/* Type definitions */

// IntType
std::string IntType::toString() const {
    return "int";
}

bool IntType::equals(const Type &other) const {
    return dynamic_cast<const IntType*>(&other) != nullptr;
}

std::string NilType::toString() const {
    return "nil";
}

// NilType
bool NilType::equals(const Type &other) const {
    return dynamic_cast<const NilType*>(&other) != nullptr ||
           dynamic_cast<const ArrayType*>(&other) != nullptr || 
           dynamic_cast<const PointerType*>(&other) != nullptr;
}

// StructType
StructType::StructType(std::string name) {
    this->name = std::move(name);
}

std::string StructType::toString() const {
    return name;
}

bool StructType::equals(const Type &other) const {
    if (dynamic_cast<const NilType*>(&other)) return false;
    
    const StructType *otherStruct = dynamic_cast<const StructType*>(&other);
    return otherStruct ? name == otherStruct->name : false;
}

// ArrayType
ArrayType::ArrayType(std::shared_ptr<Type> elementType) {
    this->elementType = std::move(elementType);
}

std::string ArrayType::toString() const {
    return elementType ? "[" + elementType->toString() + "]" : "[<null>]";
}

bool ArrayType::equals(const Type &other) const {
    if (dynamic_cast<const NilType*>(&other)) return true;

    const ArrayType* otherArray = dynamic_cast<const ArrayType*>(&other);
    return otherArray ? typePointersEqual(elementType, otherArray->elementType) : false;  
}

// PointerType
PointerType::PointerType(std::shared_ptr<Type> pointeeType) {
    this->pointeeType = std::move(pointeeType);
}
    
std::string PointerType::toString() const { 
    return pointeeType ? "&" + pointeeType->toString() : "ptr(<null>)";
};

bool PointerType::equals(const Type &other) const {
    if (dynamic_cast<const NilType*>(&other)) return true;
    
    const PointerType *otherPointer = dynamic_cast<const PointerType*>(&other);
    return otherPointer ? typePointersEqual(pointeeType, otherPointer->pointeeType) : false;
}

// FunctionType
FunctionType::FunctionType(std::vector<std::shared_ptr<Type>> paramTypes, std::shared_ptr<Type> returnType) {
    this->paramTypes = std::move(paramTypes);
    this->returnType = std::move(returnType);
}
    
std::string FunctionType::toString() const {
    std::stringstream ss;
    ss << "(";

    for (size_t i = 0; i < paramTypes.size(); i++) {
        ss << paramTypes[i]->toString();
        ss << (i == paramTypes.size() - 1 ? "" : ", ");
    }

    ss << ")";
    ss << " -> " << returnType->toString();
    return ss.str();
}

bool FunctionType::equals(const Type &other) const {
    if (dynamic_cast<const NilType*>(&other)) return false;
    
    if (const FunctionType *otherFunction = dynamic_cast<const FunctionType*>(&other)) {
        if (paramTypes.size() != otherFunction->paramTypes.size()) return false;
        
        for (size_t i = 0; i < paramTypes.size(); i++) {
            if (!typePointersEqual(paramTypes[i], otherFunction->paramTypes[i])) return false;
        }

        return typePointersEqual(returnType, otherFunction->returnType);
    }
    
    return false;
}
