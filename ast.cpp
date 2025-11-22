#include <stdexcept>
#include <set>
#include <format>

#include "ast.hpp"
#include "builder.hpp"

static inline constexpr std::string_view unaryOperandToString(UnaryOperand op) {
    switch (op) {
        case UnaryOperand::NEG: return "Neg";
        case UnaryOperand::NOT: return "Not";
    }
    
    return "";
}

static inline constexpr std::string_view binaryOperandToString(BinaryOperand op) {
    switch (op) {
        case BinaryOperand::ADD:    return "Add";  
        case BinaryOperand::SUB:    return "Sub"; 
        case BinaryOperand::MUL:    return "Mul";
        case BinaryOperand::DIV:    return "Div"; 
        case BinaryOperand::AND:    return "And"; 
        case BinaryOperand::OR:     return "Or"; 
        case BinaryOperand::EQ:     return "Eq"; 
        case BinaryOperand::NOT_EQ: return "NotEq"; 
        case BinaryOperand::LT:     return "Lt";
        case BinaryOperand::LTE:    return "Lte"; 
        case BinaryOperand::GT:     return "Gt";
        case BinaryOperand::GTE:    return "Gte";
    }

    return "";
}

/* Node declarations */

/* Declaration */
Declaration::Declaration(std::string name, std::shared_ptr<Type> type) {
    this->name = std::move(name);
    this->type = std::move(type);
}

std::string Declaration::toString() const {
    return type->toString() + " " + name;
}

void Declaration::accept(Visitor &visitor) {}

/* Expressions */

// Value
Value::Value(std::unique_ptr<Place> place) { 
    this->place = std::move(place); 
}

std::shared_ptr<Type> Value::check(const Gamma &gamma, const Delta &delta) const {
    return place->check(gamma, delta);
}

std::string Value::toString() const {
    return std::format("Val({})", place->toString());
}

void Value::accept(Visitor &visitor) {}

// Number
Number::Number(long long value) {
    this->value = std::move(value); 
}

std::shared_ptr<Type> Number::check(const Gamma &gamma, const Delta &delta) const { 
    return INT_TYPE; 
}

std::string Number::toString() const { 
    return std::format("Num({})", std::to_string(value)); 
}

void Number::accept(Visitor &visitor) {}

// Nil
std::shared_ptr<Type> Nil::check(const Gamma &gamma, const Delta &delta) const {
    return NIL_TYPE; 
}

std::string Nil::toString() const {
    return "Nil"; 
}

void Nil::accept(Visitor &visitor) {}

// Select
Select::Select(std::unique_ptr<Expression> guard, std::unique_ptr<Expression> ttCase, std::unique_ptr<Expression> ffCase) {
    this->guard = std::move(guard);
    this->ttCase = std::move(ttCase);
    this->ffCase = std::move(ffCase);
}

std::shared_ptr<Type> Select::check(const Gamma &gamma, const Delta &delta) const {
    std::shared_ptr<Type> guardType = guard->check(gamma, delta);

    if (guardType->getTypeKind() != TypeKind::INT) {
        std::string guardTypeStr = guardType->toStringPretty();
        std::string guardStr = guard->toString();
        throw std::runtime_error(std::format("non-int type {} for select guard '{}'", guardTypeStr, guardStr));
    }

    std::shared_ptr<Type> ttCaseType = trueCase->check(gamma, delta);
    std::shared_ptr<Type> ffCaseType = falseCase->check(gamma, delta);

    if (!typesEqual(ttCaseType, ffCaseType)) {
        std::string ttCaseTypeStr = ttCaseType->toStringPretty();
        std::string ttCaseStr = ttCase->toString();
        std::string ffCaseTypeStr = ffCaseType->toStringPretty();
        std::string ffCaseStr = ffCase->toString();
        throw std::runtime_error(std::format("incompatible types {} vs {} in select branches '{}' vs '{}'", ttCaseTypeStr, ffCaseTypeStr, ttCaseStr, ffCaseStr));
    }
    
    return ttCaseType->getTypeKind() == TypeKind::NIL ? ffCaseType : ttCaseType;
}

std::string Select::toString() const {
    std::string guardStr = guard->toString();
    std::string ttCaseStr = ttCase->toString();
    std::string ffCaseStr = ffCase->toString();
    return std::format("Select {{ guard: {}, tt: {}, ff: {} }}", guardStr, ttCaseStr, ffCaseStr);
}

void Select::accept(Visitor &visitor) {}

// UnaryOperation
UnaryOperation::UnaryOperation(UnaryOperand operand, std::unique_ptr<Expression> expression) {
    this->operand = operand;
    this->expression = std::move(expression);
}

std::shared_ptr<Type> UnaryOperation::check(const Gamma &gamma, const Delta &delta) const {
    std::shared_ptr<Type> operandType = expression->check(gamma, delta);
    
    if (operandType->getTypeKind() != TypeKind::INT) {
        std::string opTypeStr = operandType->toStringPretty();
        std::string thisStr = toString();
        throw std::runtime_error(std::format("non-int operand type {} in unary op '{}'", opTypeStr, thisStr));
    }

    return INT_TYPE; 
}

std::string UnaryOperation::toString() const {
    return std::format("UnOp({}, {})", std::string(unaryOperandToString(operand)), expression->toString());
}

void UnaryOperation::accept(Visitor &visitor) {}

// BinaryOperation
BinaryOperation::BinaryOperation(BinaryOperand operand, std::unique_ptr<Expression> lhs, std::unique_ptr<Expression> rhs) {
    this->operand = operand;
    this->lhs = std::move(lhs);
    this->rhs = std::move(rhs);
}

std::shared_ptr<Type> BinaryOperation::check(const Gamma &gamma, const Delta &delta) const {
    std::shared_ptr<Type> lhsType = lhs->check(gamma, delta);
    std::shared_ptr<Type> rhsType = rhs->check(gamma, delta);

    std::string lhsTypeStr = lhsType->toStringPretty();
    std::string rhsTypeStr = rhsType->toStringPretty();
    std::string lhsStr = lhs->toString();
    std::string rhsStr = rhs->toString();

    if (operand == BinaryOperand::EQ || operand == BinaryOperand::NOT_EQ) {
        if (!typesEqual(lhsType, rhsType)) {
            throw std::runtime_error(std::format("incompatible types {} vs {} in binary op '{}'", lhsTypeStr, rhsTypeStr, toString()));
        }
        
        if (lhsType->getTypeKind() == TypeKind::STRUCT || lhsType->getTypeKind() == TypeKind::FUNCTION) {
            throw std::runtime_error(std::format("invalid type {} used in binary op '{}'", lhsTypeStr, toString()));
        }
        
        if (rhsType->getTypeKind() == TypeKind::STRUCT || rhsType->getTypeKind() == TypeKind::FUNCTION) {
            throw std::runtime_error(std::format("invalid type {} used in binary op '{}'", rhsTypeStr, toString()));
        }
    } else {
        if (!typesEqual(lhsType, INT_TYPE)) {
            throw std::runtime_error(std::format("non-int type {} for left operand of binary op '{}'", lhsTypeStr, toString()));
        }

        if (!typesEqual(rhsType, INT_TYPE)) {
            throw std::runtime_error(std::format("non-int type {} for right operand of binary op '{}'", rhsTypeStr, toString()));
        }
    }
    
    return INT_TYPE; 
}

std::string BinaryOperation::toString() const {
    std::string opStr = std::string(binaryOperandToString(operand));
    std::string lhsStr = lhs->toString();
    std::string rhsStr = rhs->toString();
    return std::format("BinOp {{ op: {}, left: {}, right: {} }}", opStr, lhsStr, rhsStr);
}

void BinaryOperation::accept(Visitor &visitor) {}

// NewSingleton
NewSingleton::NewSingleton(std::shared_ptr<Type> type) {
    this->type = std::move(type);
}

std::shared_ptr<Type> NewSingleton::check(const Gamma &gamma, const Delta &delta) const {
    if (type->getTypeKind() == TypeKind::NIL || type->getTypeKind() == TypeKind::FUNCTION) {
        throw std::runtime_error(std::format("invalid type used for allocation '{}'", toString()));
    }
    
    return std::make_shared<PointerType>(type);
}

std::string NewSingleton::toString() const {
    return std::format("NewSingle({})", type->toString());
}

void NewSingleton::accept(Visitor &visitor) {}

// NewArray
NewArray::NewArray(std::shared_ptr<Type> type, std::unique_ptr<Expression> size) {
    this->type = std::move(type);
    this->size = std::move(size);
}

std::shared_ptr<Type> NewArray::check(const Gamma &gamma, const Delta &delta) const {
    std::shared_ptr<Type> sizeType = size->check(gamma, delta);
    
    std::string sizeTypeStr = sizeType->toStringPretty();
    std::string thisStr = toString();

    if (!typesEqual(sizeType, INT_TYPE)) {
        throw std::runtime_error(std::format("non-int type {} used for second argument of allocation '{}'", sizeTypeStr, thisStr));
    }

    if (type->getTypeKind() == TypeKind::NIL || type->getTypeKind() == TypeKind::FUNCTION || type->getTypeKind() == TypeKind::STRUCT) {
        throw std::runtime_error(std::format("invalid type used for first argument of allocation '{}'", thisStr));
    }

    return std::make_shared<ArrayType>(type);
}

std::string NewArray::toString() const {
    std::string typeStr = type->toString();
    std::string sizeStr = size->toString();
    return std::format("NewArray({}, {})", typeStr, sizeStr);
}

void NewArray::accept(Visitor &visitor) {}

// CallExpression
CallExpression::CallExpression(std::unique_ptr<FunctionCall> functionCall) {
    this->functionCall = std::move(functionCall);
}

std::shared_ptr<Type> CallExpression::check(const Gamma &gamma, const Delta &delta) const {
    return functionCall->check(gamma, delta); 
}

std::string CallExpression::toString() const {
    return std::format("Call({})", functionCall->toString());
}

void CallExpression::accept(Visitor &visitor) {}

/* Places */

// Identifier
Identifier::Identifier(std::string name) {
    this->name = std::move(name); 
}

std::shared_ptr<Type> Identifier::check(const Gamma &gamma, const Delta &delta) const {
    auto it = gamma.find(name);
    
    if (it != gamma.end()) {
        return it->second;
    } else {
        throw std::runtime_error(std::format("id {} does not exist in this scope", name));
    }
}

std::string Identifier::toString() const {
    return std::format("Id(\"{}\")", name);
}

void Identifier::accept(Visitor &visitor) {}

// Dereference
Dereference::Dereference(std::unique_ptr<Expression> expression) {
    this->expression = std::move(expression);
}

std::shared_ptr<Type> Dereference::check(const Gamma &gamma, const Delta &delta) const {
    std::shared_ptr<Type> pointeeType = expression->check(gamma, delta);
    
    if (auto pointerType = std::dynamic_pointer_cast<PointerType>(pointeeType)) {
        return pointerType->pointeeType;
    }

    std::string pointeeTypeStr = pointeeType->toStringPretty();
    std::string thisStr = toString();

    throw std::runtime_error(std::format("non-pointer type {} for dereference 'Val({})'", pointeeTypeStr, thisStr));
}

std::string Dereference::toString() const {
    return std::format("Deref({})", expression->toString());
}

void Dereference::accept(Visitor &visitor) {}

// ArrayAccess
ArrayAccess::ArrayAccess(std::unique_ptr<Expression> array, std::unique_ptr<Expression> index) {
    this->array = std::move(array);
    this->index = std::move(index);
}

std::shared_ptr<Type> ArrayAccess::check(const Gamma &gamma, const Delta &delta) const {
    std::shared_ptr<Type> arrayType = array->check(gamma, delta); 
    std::shared_ptr<Type> indexType = index->check(gamma, delta); 

    std::string arrayTypeStr = arrayType->toStringPretty();
    std::string indexTypeStr = indexType->toStringPretty();
    std::string thisStr = toString();

    if (!typesEqual(indexType, INT_TYPE)) {
        throw std::runtime_error(std::format("non-int index type {} for array access '{}'", indexTypeStr, thisStr));
    }

    if (auto actualArrayType = std::dynamic_pointer_cast<ArrayType>(arrayType)) {
        return actualArrayType->elementType;
    }
    
    if (typesEqual(arrayType, NIL_TYPE)) {
        throw std::runtime_error(std::format("non-array type {} for array access '{}'", arrayTypeStr, thisStr));
    }

    throw std::runtime_error(std::format("non-array type {} for array access '{}'", arrayTypeStr, thisStr));
}

std::string ArrayAccess::toString() const {
    std::string arrayStr = array->toString();
    std::string indexStr = index->toString();
    return std::format("ArrayAccess {{ array: {}, idx: {} }}", arrayStr, indexStr);
}

void ArrayAccess::accept(Visitor &visitor) {}

// FieldAccess
FieldAccess::FieldAccess(std::unique_ptr<Expression> pointer, std::string field) {
    this->pointer = std::move(pointer);
    this->field = std::move(field);
}

std::shared_ptr<Type> FieldAccess::check(const Gamma &gamma, const Delta &delta) const {
    std::shared_ptr<Type> baseType = pointer->check(gamma, delta);
    
    std::string baseTypeStr = baseType->toStringPretty();
    std::string thisStr = this->toString();

    auto ptrType = std::dynamic_pointer_cast<PointerType>(baseType);

    if (!ptrType) {
        throw std::runtime_error(std::format("{} is not a struct pointer type in field access '{}'", baseTypeStr, thisStr));
    }
    
    auto structPtrType = std::dynamic_pointer_cast<StructType>(ptrType->pointeeType);
    
    if (!structPtrType) {
        throw std::runtime_error(std::format("{} is not a struct pointer type in field access '{}'", baseTypeStr, thisStr));
    }

    std::string structPtrTypeStr = structPtrType->toStringPretty();
    
    if (!delta.count(structPtrType->name)) {
        throw std::runtime_error(std::format("non-existent struct type {} in field access '{}'", structPtrTypeStr, thisStr));
    }

    const auto &fields = delta.at(structPtrTypeStr);
    
    if (!fields.count(field)) {
         throw std::runtime_error(std::format("non-existent field {}::{} in field access '{}'", structPtrTypeStr, field, thisStr));
    }
    
    return fields.at(field);
}

std::string FieldAccess::toString() const {
    std::string ptrStr = pointer->toString();
    return std::format("FieldAccess {{ ptr: {}, field: \"{}\" }}", ptrStr, field);
}

void FieldAccess::accept(Visitor &visitor) {}

// Function call
FunctionCall::FunctionCall(std::unique_ptr<Expression> callee, std::vector<std::unique_ptr<Expression>> args) {
    this->callee = std::move(callee);
    this->args = std::move(args);
}

std::shared_ptr<Type> FunctionCall::check(const Gamma &gamma, const Delta &delta) const {
    const Identifier *directId = nullptr;

    if (auto id = dynamic_cast<Identifier*>(callee.get())) {
        directId = id;
    } else if (auto value = dynamic_cast<Value*>(callee.get())) {
        directId = dynamic_cast<Identifier*>(value->place.get());
    }

    if (directId) {
        if (directId->name == "main") {
            throw std::runtime_error("trying to call 'main'");
        }
    }
    
    std::shared_ptr<Type> calleeType = callee->check(gamma, delta);
    std::shared_ptr<FunctionType> functionType = nullptr;
    
    if (auto directFunction = std::dynamic_pointer_cast<FunctionType>(calleeType)) {
        functionType = directFunction;
    } else if (auto pointerFunction = std::dynamic_pointer_cast<PointerType>(calleeType)) {
        functionType = std::dynamic_pointer_cast<FunctionType>(pointerFunction->pointeeType);
    }

    if (!functionType) {
         throw std::runtime_error("trying to call type " + calleeType->toStringPretty() + " as function pointer in call '" + toString() + "'");
    }

    if (args.size() != functionType->paramTypes.size()) {
         throw std::runtime_error("incorrect number of arguments (" + std::to_string(args.size()) + " vs " + std::to_string(functionType->paramTypes.size()) + 
                 ") in call '" + toString() + 
                 "'");
    }

    for (size_t i = 0; i < args.size(); i++) {
        std::shared_ptr<Type> argType = args[i]->check(gamma, delta);
        const auto& paramType = functionType->paramTypes[i];
        
        if (!typesEqual(argType, paramType)) {
             throw std::runtime_error("incompatible argument type " + argType->toStringPretty() + " vs parameter type " + paramType->toStringPretty() + 
                " for argument '" + args[i]->toString() + "' in call '" + toString() + 
                "'");
        }
    }

    return functionType->returnType;
}

std::string FunctionCall::toString() const {
    std::string calleeStr = callee ? callee->toString() : "<null>";

    std::stringstream ss;
    ss << "FunCall { "; 
    ss << "callee: " << callee->toString() << ", ";
    ss << "args: [";
    
    for (size_t i = 0; i < args.size(); i++) {
        ss << args[i]->toString();
        if (i < args.size() - 1) ss << ", ";
    }

    ss << "] ";
    ss << "}";
    return ss.str();
}

void FunctionCall::accept(Visitor &visitor) {}

/* Statements */

// Statements
bool Statements::check(const Gamma &gamma, const Delta &delta, const std::shared_ptr<Type> &returnType, bool inLoop) const {
    bool doesReturn = false;

    for (const auto &statement : statements) {
        if (doesReturn) {
            statement->check(gamma, delta, returnType, inLoop);
        } else {
            doesReturn = statement->check(gamma, delta, returnType, inLoop);
        }
    }

    return doesReturn;
}

std::string Statements::toString() const {
    std::stringstream ss;
    ss << "[";
    
    for (size_t i = 0; i < statements.size(); i++) {
        ss << statements[i]->toString();
        if (i < statements.size())
            ss << ", ";
    }

    ss << "]";
    return ss.str();
}

void Statements::accept(Visitor &visitor) {}

// Assignment
Assignment::Assignment(std::unique_ptr<Place> place, std::unique_ptr<Expression> expression) { 
    this->place = std::move(place);
    this->expression = std::move(expression);
}

bool Assignment::check(const Gamma &gamma, const Delta &delta, const std::shared_ptr<Type> &returnType, bool inLoop) const {
    std::shared_ptr<Type> lhsType = place->check(gamma, delta);
    std::shared_ptr<Type> rhsType = expression->check(gamma, delta);

    if (dynamic_cast<StructType*>(lhsType.get()) || dynamic_cast<FunctionType*>(lhsType.get()) || dynamic_cast<NilType*>(lhsType.get())) {
        throw std::runtime_error("invalid type " + lhsType->toStringPretty() + " for left-hand side of assignment '" + toString() + "'");
    }

    if (!typesEqual(lhsType, rhsType)) {
         throw std::runtime_error("incompatible types " + lhsType->toStringPretty() + " vs " + rhsType->toStringPretty() + " for assignment '" + toString() + "'");
    }

    return false;
}

std::string Assignment::toString() const {
    return "Assign(" + place->toString() + ", " + expression->toString() + ")";
}

void Assignment::accept(Visitor &visitor) {}

// CallStatement
CallStatement::CallStatement(std::unique_ptr<FunctionCall> functionCall) {
    this->functionCall = std::move(functionCall);
}

bool CallStatement::check(const Gamma &gamma, const Delta &delta, const std::shared_ptr<Type> &returnType, bool inLoop) const {
    functionCall->check(gamma, delta);
    return false;
}

std::string CallStatement::toString() const {
    return "Call(" + functionCall->toString() + ")";
}

void CallStatement::accept(Visitor &visitor) {}

// If
If::If(std::unique_ptr<Expression> guard, std::unique_ptr<Statement> happyPath, std::optional<std::unique_ptr<Statement>> unhappyPath) {
    this->guard = std::move(guard);
    this->happyPath = std::move(happyPath);
    this->unhappyPath = std::move(unhappyPath);
}

bool If::check(const Gamma &gamma, const Delta &delta, const std::shared_ptr<Type> &returnType, bool inLoop) const {
    std::shared_ptr<Type> guardType = guard->check(gamma, delta);

    if (!typesEqual(guardType, std::make_shared<IntType>())) {
        throw std::runtime_error("non-int type " + guardType->toStringPretty() + " for if guard '" + guard->toString() + "'");
    }

    bool happyPathReturns = happyPath->check(gamma, delta, returnType, inLoop); 
    bool unhappyPathReturns = unhappyPath.has_value() ? (*unhappyPath)->check(gamma, delta, returnType, inLoop) : false; 
    return happyPathReturns && unhappyPathReturns;
}

std::string If::toString() const {
    std::stringstream ss;
    ss << "If { "; 
    ss << "guard: " << guard->toString() << ", ";
    ss << "true: " << happyPath->toString() << ", ";
    ss << "unhappyPath: " << (unhappyPath ? (*unhappyPath)->toString() : "<None>");
    ss << " }";
    return ss.str();
}

void If::accept(Visitor &visitor) {}

// While
While::While(std::unique_ptr<Expression> guard, std::unique_ptr<Statement> body) {
    this->guard = std::move(guard);
    this->body = std::move(body);
}

bool While::check(const Gamma &gamma, const Delta &delta, const std::shared_ptr<Type> &returnType, bool inLoop) const {
    std::shared_ptr<Type> guardType = guard->check(gamma, delta);
    
    if (!typesEqual(guardType, std::make_shared<IntType>())) {
        throw std::runtime_error("non-int type " + guardType->toStringPretty() + " for while guard '" + guard->toString() + "'");
    }

    body->check(gamma, delta, returnType, true);
    return false;
}

std::string While::toString() const {
    return "While(" + guard->toString() + ", " + body->toString() + ")";
}

void While::accept(Visitor &visitor) {}

// Break
bool Break::check(const Gamma &gamma, const Delta &delta, const std::shared_ptr<Type> &returnType, bool inLoop) const {
    if (!inLoop) {
        throw std::runtime_error("break outside loop");
    }
    return false;
}

std::string Break::toString() const { 
    return "Break"; 
}

void Break::accept(Visitor &visitor) {}

// Continue
bool Continue::check(const Gamma &gamma, const Delta &delta, const std::shared_ptr<Type> &returnType, bool inLoop) const {
    if (!inLoop) {
        throw std::runtime_error("continue outside loop");
    }
    return false;
}

std::string Continue::toString() const {
    return "Continue";
}

void Continue::accept(Visitor &visitor) {}

// Return
Return::Return(std::optional<std::unique_ptr<Expression>> expression) {
    this->expression = std::move(expression);
}

bool Return::check(const Gamma &gamma, const Delta &delta, const std::shared_ptr<Type> &returnType, bool inLoop) const {
    if (expression.has_value()) {
        std::shared_ptr<Type> expressionType = (*expression)->check(gamma, delta);
        
        if (!typesEqual(expressionType, returnType)) {
             throw std::runtime_error(
                "incompatible return type " + 
                expressionType->toStringPretty() + 
                " for 'return " + (*expression)->toString() + 
                "', should be " + returnType->toStringPretty()
            );
        }
    } else {
        if (!typesEqual(returnType, std::make_shared<IntType>())) {
            throw std::runtime_error("missing return expression for non-int function type " + returnType->toStringPretty());
        }
        throw std::runtime_error("return statement requires an expression in this function");
    }
    
    return true; 
}

std::string Return::toString() const {
    std::stringstream ss;
    ss << "Return(";
    ss << (expression ? (*expression)->toString() : "<void>");
    ss << ")";
    return ss.str();
}

void Return::accept(Visitor &visitor) {}

/* Top level nodes*/

// StructDefinition
void StructDefinition::check(const Gamma &gamma, const Delta &delta) const {
    if (fields.empty()) {
        throw std::runtime_error("empty struct " + name);
    }

    std::set<std::string> fieldNames;

    for (const auto& field : fields) {
        if (dynamic_cast<NilType*>(field.type.get()) || dynamic_cast<StructType*>(field.type.get()) || dynamic_cast<FunctionType*>(field.type.get())) {
             throw std::runtime_error("invalid type " + field.type->toStringPretty() + " for struct field " + name + "::" + field.name);
        }
        
        if (fieldNames.find(field.name) != fieldNames.end()) {
             throw std::runtime_error("Duplicate field name '" + field.name + "' in struct '" + name + "'");
        }
    }
}

std::string StructDefinition::toString() const {
    std::stringstream ss;
    ss << name << " {";
    
    for (size_t i = 0; i < fields.size(); i++) {
        ss << fields[i].toString();
        if (i < fields.size() - 1)
            ss << ", ";
    }

    ss << "}";
    
    return ss.str();
}

void StructDefinition::accept(Visitor &visitor) {}

// Extern
std::string Extern::toString() const {
    std::stringstream ss;
    ss << "Extern { ";
    ss << "name: \"" << name << "\", "; 
    
    ss << "params: [";
    
    for (size_t i = 0; i < paramTypes.size(); i++) {
        ss << paramTypes[i]->toString();
        if (i < paramTypes.size() - 1) {
            ss << ", ";
        }
    }

    ss << "], "; 
    ss << "returnType: " << returnType->toString() << " }";
    
    return ss.str();
}

void Extern::accept(Visitor &visitor) {}

// FunctionDefinition
void FunctionDefinition::check(const Gamma &gamma, const Delta &delta) const {
    Gamma localGamma = gamma;
    std::set<std::string> localNames; 

    for(const auto& param : params) {
        if (dynamic_cast<NilType*>(param.type.get()) || 
            dynamic_cast<StructType*>(param.type.get()) || 
            dynamic_cast<FunctionType*>(param.type.get())) {
            throw std::runtime_error("invalid type " + param.type->toStringPretty() + " for variable " + param.name + " in function " + name);
        }

        if (localNames.find(param.name) != localNames.end()) {
            throw std::runtime_error("Duplicate parameter/local name '" + param.name + "' in function '" + name + "'");
        }

        localGamma[param.name] = param.type;
    }
    
    for(const auto& local : locals) {
        if (dynamic_cast<NilType*>(local.type.get()) || 
            dynamic_cast<StructType*>(local.type.get()) || 
            dynamic_cast<FunctionType*>(local.type.get())) {
            throw std::runtime_error("invalid type " + local.type->toStringPretty() + " for variable " + local.name + " in function " + name);
        }

        if (localNames.find(local.name) != localNames.end()) {
            throw std::runtime_error("Duplicate parameter/local name '" + local.name + "' in function '" + name + "'");
        }

        localGamma[local.name] = local.type;
    }

    if (!body) {
        throw std::runtime_error("function " + name + " has an empty body");
    }

    if (auto statementsPointer = dynamic_cast<Statements*>(body.get())) {
        if (statementsPointer->statements.empty()) {
            throw std::runtime_error("function " + name + " has an empty body");
        }
    } else {
        throw std::runtime_error("function " + name + " has an invalid body structure (expected Stmts)");
    }

    bool doesReturn = body->check(localGamma, delta, returnType, false);

    if (!doesReturn) {
        throw std::runtime_error("function " + name + " may not execute a return");
    }
}

std::string FunctionDefinition::toString() const {
    std::stringstream ss;
    ss << "Function { name: \"" << name << "\", "; 
    ss << " params: [";
    
    for (size_t i = 0; i < params.size(); i++) {
        ss << params[i].toString();
        if (i < params.size() - 1)
            ss << ", ";
    }
    
    ss << "], "; 
    ss << "returnType: " << returnType << ", "; 
    
    ss << "locals: {";
    
    for (size_t i = 0; i < locals.size(); i++) {
        ss << locals[i].toString();
        if (i < locals.size() - 1)
            ss << ", ";
    }

    ss << "}, ";
    ss << "body: " << body->toString() << " }";
    
    return ss.str();
}

void FunctionDefinition::accept(Visitor &visitor) {}

// Program
void Program::check() const {
    std::set<std::string> topLevelNames;
    
    for (const auto &s : structs) {
        if (topLevelNames.find(s->name) != topLevelNames.end()) {
            throw std::runtime_error("Duplicate name: " + s->name);
        } 
    }
    
    for (const auto &e : externs) {
        if (topLevelNames.find(e.name) != topLevelNames.end()) {
            throw std::runtime_error("Duplicate name: " + e.name);
        }
    }
    
    for (const auto &f : functions) {
        if (f->name != "main" && topLevelNames.find(f->name) != topLevelNames.end()) {
            throw std::runtime_error("Duplicate name: " + f->name);
        } 
    }

    Gamma gamma = constructGamma(externs, functions);
    Delta delta = constructDelta(structs);

    bool mainFound = false;
    
    for (const auto &f : functions) {
        if (f->name == "main") {
            if (f->params.empty() && typesEqual(f->returnType, std::make_shared<IntType>())) {
                mainFound = true;
            }         
        }
    }

    if (!mainFound) {
        throw std::runtime_error("no 'main' function with type '() -> int' exists");
    }

    for (const auto &s : structs) {
        s->check(gamma, delta);
    }

    for (const auto &f : functions) {
        f->check(gamma, delta);
    }
}

std::string Program::toString() const {
    std::stringstream ss;
    ss << "Program { ";

    ss << "structs: { ";
    
    for (size_t i = 0; i < structs.size(); i++) {    
        ss << structs[i]->toString();
        if (i < structs.size() - 1) ss << ", ";
    }
    
    ss << "}, ";
    ss << "externs: {";
    
    for (size_t i = 0; i < externs.size(); i++) {
        ss << externs[i].toString();
        if (i < externs.size() - 1) ss << ", ";
    }
    
    ss << "}, "; 
    ss << "functions: {";
    
    for (size_t i = 0; i < functions.size(); i++) {
        ss << functions[i]->toString();
        if (i < functions.size() - 1) ss << ", ";
    }

    ss << "} }";
    
    return ss.str();
}

void Program::accept(Visitor &visitor) {}
