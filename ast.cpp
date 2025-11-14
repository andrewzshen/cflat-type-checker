#include <stdexcept>
#include <set>

#include "ast.hpp"
#include "checker.hpp"

static inline bool isLowPrecedence(const Expression *exp) {
    if (!exp) return false;
    return dynamic_cast<const BinaryOperation*>(exp) || dynamic_cast<const Select*>(exp);
}

/* Declaration */
Declaration::Declaration(std::string name, std::shared_ptr<Type> type) {
    this->name = std::move(name);
    this->type = std::move(type);
}

std::string Declaration::toString() const {
    return "Decl { name: \"" + name + "\", typ: " + type->toString() + " }";
}

/* Expressions */

// Value
Value::Value(std::unique_ptr<Place> place) { 
    this->place = std::move(place); 
}

std::shared_ptr<Type> Value::check(const Gamma &gamma, const Delta &delta) const {
    return place->check(gamma, delta);
}

std::string Value::toString() const {
    return "Val(" + place->toString() + ")";
}

// Number
Number::Number(long long value) {
    this->value = std::move(value); 
}

std::shared_ptr<Type> Number::check(const Gamma &gamma, const Delta &delta) const { 
    return std::make_shared<IntType>();
}

std::string Number::toString() const { 
    return std::to_string(value); 
}

// Nil
std::shared_ptr<Type> Nil::check(const Gamma &gamma, const Delta &delta) const {
    return std::make_shared<NilType>();
}

std::string Nil::toString() const {
    return "Nil"; 
}

// Select
Select::Select(std::unique_ptr<Expression> guard, std::unique_ptr<Expression> happyCase, std::unique_ptr<Expression> unhappyCase) {
    this->guard = std::move(guard);
    this->happyCase = std::move(happyCase);
    this->unhappyCase = std::move(unhappyCase);
}

std::shared_ptr<Type> Select::check(const Gamma &gamma, const Delta &delta) const {
    std::shared_ptr<Type> guardType = guard->check(gamma, delta);

    if (!typesEqual(guardType, std::make_shared<IntType>())) {
        throw std::runtime_error("non-int type " + guardType->toString() + " for select guard '" + guard->toString() + "'");
    }

    std::shared_ptr<Type> happyCaseType = happyCase->check(gamma, delta);
    std::shared_ptr<Type> unhappyCaseType = unhappyCase->check(gamma, delta);

    if (!typesEqual(happyCaseType, unhappyCaseType)) {
        throw std::runtime_error(
            "incompatible types " + 
            happyCaseType->toString() + " vs " + unhappyCaseType->toString() + 
            " in select branches '" +
            happyCase->toString() + "' vs '" + unhappyCase->toString() + 
            "'"
        );
    }

    return pickNonNil(happyCaseType, unhappyCaseType);
}

std::string Select::toString() const {
    return "Select { guard: " + guard->toString() + ", true: " + happyCase->toString() + ", false: " + unhappyCase->toString() + " }";
}

UnaryOperation::UnaryOperation(UnaryOperand operand, std::unique_ptr<Expression> expression) {
    this->operand = operand;
    this->expression = std::move(expression);
}

std::shared_ptr<Type> UnaryOperation::check(const Gamma &gamma, const Delta &delta) const {
    std::shared_ptr<Type> operandType = expression->check(gamma, delta);
    
    if (!typesEqual(operandType, std::make_shared<IntType>())) {
        throw std::runtime_error("non-int operand type " + operandType->toString() + " in unary op '" + toString() + "'");
    }

    return std::make_shared<IntType>();
}

std::string UnaryOperation::toString() const {
    std::string op;
    switch (operand) {
        case UnaryOperand::NEG:
            op = "- ";
            break;
        case UnaryOperand::NOT:
            op = "not ";
            break;
    }

    std::string str = expression ? expression->toString() : "<null>";
    return op + "(" + str + ")";
}

BinaryOperation::BinaryOperation(BinaryOperand operand, std::unique_ptr<Expression> lhs, std::unique_ptr<Expression> rhs) {
    this->operand = operand;
    this->lhs = std::move(lhs);
    this->rhs = std::move(rhs);
}

std::shared_ptr<Type> BinaryOperation::check(const Gamma &gamma, const Delta &delta) const {
    std::shared_ptr<Type> lhsType = lhs->check(gamma, delta);
    std::shared_ptr<Type> rhsType = rhs->check(gamma, delta);

    if (operand == BinaryOperand::EQ || operand == BinaryOperand::NOT_EQ) {
        if (!typesEqual(lhsType, rhsType)) {
            throw std::runtime_error("incompatible types " + lhsType->toString() + " vs " + rhsType->toString() + " in binary op '" + toString() + "'");
        }
        
        if (dynamic_cast<StructType*>(lhsType.get()) || dynamic_cast<FunctionType*>(lhsType.get())) {
             throw std::runtime_error("invalid type " + lhsType->toString() + " used in binary op '" + toString() + "'");
        }
        
        if (dynamic_cast<StructType*>(rhsType.get()) || dynamic_cast<FunctionType*>(rhsType.get())) {
             throw std::runtime_error("invalid type " + rhsType->toString() + " used in binary op '" + toString() + "'");
        }
    } else {
        if (!typesEqual(lhsType, std::make_shared<IntType>())) {
            throw std::runtime_error("non-int type " + lhsType->toString() + " for left operand of binary op '" + toString() + "'");
        }

        if (!typesEqual(rhsType, std::make_shared<IntType>())) {
            throw std::runtime_error("right operand of binary op '" + toString() + "' has type " + rhsType->toString() + ", should be int");
        }
    }
    
    return std::make_shared<IntType>(); 
}

std::string BinaryOperation::toString() const {
    std::string op;
    switch (operand) {
        case BinaryOperand::ADD:
            op = " + ";
            break;
        case BinaryOperand::SUB:
            op = " - ";
            break;
        case BinaryOperand::MUL:
            op = " * ";
            break;
        case BinaryOperand::DIV:
            op = " / ";
            break;
        case BinaryOperand::EQ:
            op = " == ";
            break;
        case BinaryOperand::NOT_EQ:
            op = " != ";
            break;
        case BinaryOperand::LT:
            op = " < ";
            break;
        case BinaryOperand::LTE:
            op = " <= ";
            break;
        case BinaryOperand::GT:
            op = " > ";
            break;
        case BinaryOperand::GTE:
            op = " >= ";
            break;
        case BinaryOperand::AND:
            op = " and ";
            break;
        case BinaryOperand::OR:
            op = " or ";
            break;
    }

    std::string left = lhs ? lhs->toString() : "<null>";
    std::string right = rhs ? rhs->toString() : "<null>";
    
    if (dynamic_cast<const Select*>(rhs.get())) {
        right = "(" + right + ")";
    }
    
    return left + op + right;
}

// NewSingleton
NewSingleton::NewSingleton(std::shared_ptr<Type> type) {
    this->type = std::move(type);
}

std::shared_ptr<Type> NewSingleton::check(const Gamma &gamma, const Delta &delta) const {
    if (dynamic_cast<NilType*>(type.get()) || dynamic_cast<FunctionType*>(type.get())) {
        throw std::runtime_error("invalid type used for allocation '" + toString() + "'");
    }
    return std::make_shared<PointerType>(type);
}

std::string NewSingleton::toString() const {
    return "new " + type->toString();
}

// NewArray
NewArray::NewArray(std::shared_ptr<Type> type, std::unique_ptr<Expression> size) {
    this->type = std::move(type);
    this->size = std::move(size);
}

std::shared_ptr<Type> NewArray::check(const Gamma &gamma, const Delta &delta) const {
    std::shared_ptr<Type> sizeType = size->check(gamma, delta);
    
    if (!typesEqual(sizeType, std::make_shared<IntType>())) {
        throw std::runtime_error("non-int type " + sizeType->toString() + " used for second argument of allocation '" + toString() + "'");
    }

    if (dynamic_cast<NilType*>(type.get()) || dynamic_cast<FunctionType*>(type.get()) || dynamic_cast<StructType*>(type.get())) {
        throw std::runtime_error("invalid type used for first argument of allocation '" + toString() + "'");
    }

    return std::make_shared<ArrayType>(type);
}

std::string NewArray::toString() const {
    return "NewArray(" + type->toString() + ", " + size->toString() + ")";
}

// CallExpression
CallExpression::CallExpression(std::unique_ptr<FunctionCall> functionCall) {
    this->functionCall = std::move(functionCall);
}

std::shared_ptr<Type> CallExpression::check(const Gamma &gamma, const Delta &delta) const {
    return functionCall->check(gamma, delta); 
}

std::string CallExpression::toString() const {
    return functionCall->toString();
}

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
        throw std::runtime_error("id " + name + " does not exist in this scope");
    }
}

std::string Identifier::toString() const {
    return "Id(\"" + name + "\")";
}

// Dereference
Dereference::Dereference(std::unique_ptr<Expression> expression) {
    this->expression = std::move(expression);
}

std::shared_ptr<Type> Dereference::check(const Gamma &gamma, const Delta &delta) const {
    std::shared_ptr<Type> pointee = expression->check(gamma, delta);
    
    if (auto pointerType = std::dynamic_pointer_cast<PointerType>(pointee)) {
        return pointerType->pointeeType;
    }

    throw std::runtime_error("non-pointer type " + pointee->toString() + " for dereference '" + expression->toString() + ".*'");
}

std::string Dereference::toString() const {
    return "Deref(" + expression->toString() + ")";
}

// ArrayAccess
ArrayAccess::ArrayAccess(std::unique_ptr<Expression> array, std::unique_ptr<Expression> index) {
    this->array = std::move(array);
    this->index = std::move(index);
}

std::shared_ptr<Type> ArrayAccess::check(const Gamma &gamma, const Delta &delta) const {
    std::shared_ptr<Type> arrayType = array->check(gamma, delta); 
    std::shared_ptr<Type> indexType = index->check(gamma, delta); 

    if (!typesEqual(indexType, std::make_shared<IntType>())) {
        throw std::runtime_error("non-int index type " + indexType->toString() + " for array access '" + array->toString() + "'");
    }

    if (auto actualArrayType = std::dynamic_pointer_cast<ArrayType>(arrayType)) {
        return actualArrayType->elementType;
    }
    
    if (typesEqual(arrayType, std::make_shared<NilType>())) {
        throw std::runtime_error("non-array type " + arrayType->toString() + " for array access '" + array->toString() + "'");
    }

    throw std::runtime_error("non-array type " + arrayType->toString() + " for array access '" + array->toString() + "'");
}

std::string ArrayAccess::toString() const {
    return "ArrayAccess { array: " + array->toString() + ", index: " + index->toString() + "}";
}

// FieldAccess
FieldAccess::FieldAccess(std::unique_ptr<Expression> pointer, std::string field) {
    this->pointer = std::move(pointer);
    this->field = std::move(field);
}

std::shared_ptr<Type> FieldAccess::check(const Gamma &gamma, const Delta &delta) const {
    std::shared_ptr<Type> baseType = pointer->check(gamma, delta);
    auto pointerType = std::dynamic_pointer_cast<PointerType>(baseType);

    if (!pointerType) {
        throw std::runtime_error("<" + baseType->toString() + "> is not a struct pointer type in field access '" + toString() + "'");
    }
    
    auto structPointerType = std::dynamic_pointer_cast<StructType>(pointerType->pointeeType);
    
    if (!structPointerType) {
        throw std::runtime_error("pointer type <" + baseType->toString() + "> does not point to a struct in field access '" + toString() + "'");
    }
    
    if (!delta.count(structPointerType->name)) {
        throw ("non-existent struct type " + structPointerType->name + " in field access '" + toString() + "'");
    }

    const auto &fields = delta.at(structPointerType->name);
    
    if (!fields.count(field)) {
         throw std::runtime_error("non-existent field " + structPointerType->name + "::" + field + " in field access '" + toString() + "'");
    }
    
    return fields.at(field);
}

std::string FieldAccess::toString() const {
    return "FieldAccess { ptr: " + pointer->toString() + ", field: \"" + field + "\" }";
}

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
         throw std::runtime_error("trying to call type " + calleeType->toString() + " as function pointer in call '" + toString() + "'");
    }

    if (args.size() != functionType->paramTypes.size()) {
         throw std::runtime_error(
                 "incorrect number of arguments (" + 
                 std::to_string(args.size()) + 
                 " vs " + 
                 std::to_string(functionType->paramTypes.size()) + 
                 ") in call '" + toString() + 
                 "'"
            );
    }

    for (size_t i = 0; i < args.size(); i++) {
        std::shared_ptr<Type> argType = args[i]->check(gamma, delta);
        const auto& paramType = functionType->paramTypes[i];
        
        if (!typesEqual(argType, paramType)) {
             throw std::runtime_error(
                "incompatible argument type " + 
                argType->toString() + 
                " vs parameter type " + paramType->toString() + 
                " for argument '" + args[i]->toString() + "' in call '" + toString() + 
                "'"
            );
        }
    }

    return functionType->returnType;
}

std::string FunctionCall::toString() const {
    std::string calleeStr = callee ? callee->toString() : "<null>";

    std::stringstream ss;
    ss << calleeStr << "(";
    for (size_t i = 0; i < args.size(); i++) {
        ss << args[i]->toString();
        if (i < args.size() - 1)
            ss << ", ";
    }
    ss << ")";
    return ss.str();
}

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

// Assignment
Assignment::Assignment(std::unique_ptr<Place> place, std::unique_ptr<Expression> expression) { 
    this->place = std::move(place);
    this->expression = std::move(expression);
}

bool Assignment::check(const Gamma &gamma, const Delta &delta, const std::shared_ptr<Type> &returnType, bool inLoop) const {
    std::shared_ptr<Type> lhsType = place->check(gamma, delta);
    std::shared_ptr<Type> rhsType = expression->check(gamma, delta);

    if (dynamic_cast<StructType*>(lhsType.get()) || dynamic_cast<FunctionType*>(lhsType.get()) || dynamic_cast<NilType*>(lhsType.get())) {
        throw std::runtime_error(
                "invalid type " + 
                lhsType->toString() + 
                " for left-hand side of assignment '" + 
                place->toString() + " = " + expression->toString() + 
                "'"
            );
    }

    if (!typesEqual(lhsType, rhsType)) {
         throw std::runtime_error(
                 "incompatible types " + 
                 lhsType->toString() + 
                 " vs " + 
                 rhsType->toString() + 
                 " for assignment '" + 
                 place->toString() + " = " + expression->toString() + 
                 "'"
            );
    }

    return false;
}

std::string Assignment::toString() const {
    return "Assign(" + place->toString() + ", " + expression->toString() + ")";
}

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

// If
If::If(std::unique_ptr<Expression> guard, std::unique_ptr<Statement> happyPath, std::optional<std::unique_ptr<Statement>> unhappyPath) {
    this->guard = std::move(guard);
    this->happyPath = std::move(happyPath);
    this->unhappyPath = std::move(unhappyPath);
}

bool If::check(const Gamma &gamma, const Delta &delta, const std::shared_ptr<Type> &returnType, bool inLoop) const {
    std::shared_ptr<Type> guardType = guard->check(gamma, delta);

    if (!typesEqual(guardType, std::make_shared<IntType>())) {
        throw std::runtime_error("non-int type " + guardType->toString() + " for if guard '" + guard->toString() + "'");
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

// While
While::While(std::unique_ptr<Expression> guard, std::unique_ptr<Statement> body) {
    this->guard = std::move(guard);
    this->body = std::move(body);
}

bool While::check(const Gamma &gamma, const Delta &delta, const std::shared_ptr<Type> &returnType, bool inLoop) const {
    std::shared_ptr<Type> guardType = guard->check(gamma, delta);
    
    if (!typesEqual(guardType, std::make_shared<IntType>())) {
         throw std::runtime_error("non-int type " + guardType->toString() + " for while guard '" + guard->toString() + "'");
    }

    body->check(gamma, delta, returnType, true);
    return false;
}

std::string While::toString() const {
    return "While(" + guard->toString() + ", " + body->toString() + ")";
}

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
                expressionType->toString() + 
                " for 'return " + (*expression)->toString() + 
                "', should be " + returnType->toString()
            );
        }
    } else {
        if (!typesEqual(returnType, std::make_shared<IntType>())) {
            throw std::runtime_error("missing return expression for non-int function type " + returnType->toString());
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

/* Top level nodes*/

// StructDefinition
void StructDefinition::check(const Gamma &gamma, const Delta &delta) const {
    if (fields.empty()) {
        throw std::runtime_error("empty struct " + name);
    }

    std::set<std::string> fieldNames;

    for (const auto& field : fields) {
        if (dynamic_cast<NilType*>(field.type.get()) || dynamic_cast<StructType*>(field.type.get()) || dynamic_cast<FunctionType*>(field.type.get())) {
             throw std::runtime_error("invalid type " + field.type->toString() + " for struct field " + name + "::" + field.name);
        }
        
        if (fieldNames.find(field.name) != fieldNames.end()) {
             throw std::runtime_error("Duplicate field name '" + field.name + "' in struct '" + name + "'");
        }
    }
}

std::string StructDefinition::toString() const {
    std::stringstream ss;
    ss << "Struct " << " { name \"" + name + "\", fields: {";
    
    for (size_t i = 0; i < fields.size(); i++) {
        ss << fields[i].toString();
        if (i < fields.size() - 1)
            ss << ", ";
    }

    ss << "} }";
    return ss.str();
}

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

// FunctionDefinition
void FunctionDefinition::check(const Gamma &gamma, const Delta &delta) const {
    Gamma localGamma = gamma;
    std::set<std::string> localNames; 

    for(const auto& param : params) {
        if (dynamic_cast<NilType*>(param.type.get()) || 
            dynamic_cast<StructType*>(param.type.get()) || 
            dynamic_cast<FunctionType*>(param.type.get())) {
            throw std::runtime_error("invalid type " + param.type->toString() + " for variable " + param.name + " in function " + name);
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
            throw std::runtime_error("invalid type " + local.type->toString() + " for variable " + local.name + " in function " + name);
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
            } else {
                throw std::runtime_error("function 'main' exists but has wrong type, should be '() -> int'");
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
