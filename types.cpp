#include "types.hpp"

#include <sstream>

/* Helper functions */
bool typesEqual(const std::shared_ptr<Type> &lhs, const std::shared_ptr<Type> &rhs) {
    if (!lhs && !rhs) return true;
    if (!lhs || !rhs) return false;
    return lhs->equals(*rhs);
}

/* Type definitions */

// IntType
std::string IntType::toString() const {
    return "Int";
}

std::string IntType::toStringPretty() const {
    return "int";
}

bool IntType::equals(const Type &other) const {
    return other.getTypeKind() == TypeKind::INT;
}

TypeKind IntType::getTypeKind() const {
    return TypeKind::INT;
}

// NilType
std::string NilType::toString() const {
    return "Nil";
}

std::string NilType::toStringPretty() const {
    return "nil";
}

bool NilType::equals(const Type &other) const {
    const TypeKind otherTypeKind = other.getTypeKind();
    return otherTypeKind == TypeKind::NIL || 
           otherTypeKind == TypeKind::ARRAY || 
           otherTypeKind == TypeKind::POINTER;
}

TypeKind NilType::getTypeKind() const {
    return TypeKind::NIL;
}

// StructType
StructType::StructType(std::string name) {
    this->name = std::move(name);
}

std::string StructType::toString() const {
    return "Struct(\"" + name + "\")";
}

std::string StructType::toStringPretty() const {
    return name;
}

bool StructType::equals(const Type &other) const {
    if (other.getTypeKind() != TypeKind::STRUCT) return false; 
    return name == dynamic_cast<const StructType*>(&other)->name;
}

TypeKind StructType::getTypeKind() const {
    return TypeKind::STRUCT;
}

// ArrayType
ArrayType::ArrayType(std::shared_ptr<Type> elementType) {
    this->elementType = std::move(elementType);
}

std::string ArrayType::toString() const {
    return "Array(" + elementType->toString() + ")";
}

std::string ArrayType::toStringPretty() const {
    return elementType ? "[" + elementType->toStringPretty() + "]" : "[<null>]";
}

bool ArrayType::equals(const Type &other) const {
    if (other.getTypeKind() == TypeKind::NIL) return true;
    if (other.getTypeKind() != TypeKind::ARRAY) return false;
    return typesEqual(elementType, dynamic_cast<const ArrayType*>(&other)->elementType);  
}

TypeKind ArrayType::getTypeKind() const {
    return TypeKind::ARRAY;
}

// PointerType
PointerType::PointerType(std::shared_ptr<Type> pointeeType) {
    this->pointeeType = std::move(pointeeType);
}
    
std::string PointerType::toString() const { 
    return "Ptr(" + (pointeeType ? pointeeType->toString() : "<null>") + ")";
};

std::string PointerType::toStringPretty() const { 
    return "&" + (pointeeType ? pointeeType->toStringPretty() : "<null>");
};

bool PointerType::equals(const Type &other) const {
    if (other.getTypeKind() == TypeKind::NIL) return true;
    if (other.getTypeKind() != TypeKind::POINTER) return false; 
    return typesEqual(pointeeType, dynamic_cast<const PointerType*>(&other)->pointeeType);
}

TypeKind PointerType::getTypeKind() const {
    return TypeKind::POINTER;
}

// FunctionType
FunctionType::FunctionType(std::vector<std::shared_ptr<Type>> paramTypes, std::shared_ptr<Type> returnType) {
    this->paramTypes = std::move(paramTypes);
    this->returnType = std::move(returnType);
}
    
std::string FunctionType::toString() const {
    std::stringstream ss;
    ss << "Fn(";
    ss << "[";

    for (size_t i = 0; i < paramTypes.size(); i++) {
        ss << paramTypes[i]->toString();
        ss << (i == paramTypes.size() - 1 ? "" : ", ");
    }

    ss << "]";
    ss << ", " << returnType->toString();
    ss << ")";
    return ss.str();
}

std::string FunctionType::toStringPretty() const {
    std::stringstream ss;
    ss << "(";

    for (size_t i = 0; i < paramTypes.size(); i++) {
        ss << paramTypes[i]->toStringPretty();
        ss << (i == paramTypes.size() - 1 ? "" : ", ");
    }

    ss << ")";
    ss << " -> " << returnType->toStringPretty();
    return ss.str();
}

bool FunctionType::equals(const Type &other) const {
    if (other.getTypeKind() != TypeKind::FUNCTION) return false;
    
    const FunctionType *otherFunction = dynamic_cast<const FunctionType*>(&other);
    
    if (paramTypes.size() != otherFunction->paramTypes.size()) return false;
    
    for (size_t i = 0; i < paramTypes.size(); i++) {
        if (!typesEqual(paramTypes[i], otherFunction->paramTypes[i])) return false;
    }

    return typesEqual(returnType, otherFunction->returnType);
}

TypeKind FunctionType::getTypeKind() const { 
    return TypeKind::FUNCTION;
}
