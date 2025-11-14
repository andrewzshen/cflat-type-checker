#include <stdexcept>

#include "json.hpp"
#include "checker.hpp"

std::shared_ptr<Type> buildType(const nlohmann::json &json) {
    if (json.is_string()) {
        const std::string &kind = json.get<std::string>();
        
        if (kind == "Int")
            return std::make_shared<IntType>();
        
        if (kind == "Nil")
            return std::make_shared<NilType>();
        
        throw std::runtime_error("Unknown simple type string: " + kind);
    }

    if (json.is_object()) {
        if (json.contains("Struct")) {
            return std::make_shared<StructType>(json.at("Struct").get<std::string>());
        }

        if (json.contains("Ptr")) {
            return std::make_shared<PointerType>(buildType(json.at("Ptr")));
        }

        if (json.contains("Array")) {
            return std::make_shared<ArrayType>(buildType(json.at("Array")));
        }

        if (json.contains("Fn")) {
            const auto &funcSig = json.at("Fn");
            
            if (!funcSig.is_array() || funcSig.size() != 2 ||
                !funcSig[0].is_array()) {
                throw std::runtime_error("Invalid JSON for Function type signature.");
            }
            
            std::vector<std::shared_ptr<Type>> params;
            
            for (const auto &param : funcSig[0]) {
                params.push_back(buildType(param));
            }
            
            return std::make_shared<FunctionType>(std::move(params), buildType(funcSig[1]));
        }

        if (json.contains("kind")) {
            const std::string &kind = json.at("kind").get<std::string>();
            if (kind == "Int")
                return std::make_shared<IntType>();
            if (kind == "Nil")
                return std::make_shared<NilType>();
        }
    }

    throw std::runtime_error("Invalid JSON for Type: " + json.dump());
}

std::unique_ptr<Place> buildPlace(const nlohmann::json &json) {
    if (!json.is_object() || json.empty()) {
        throw std::runtime_error("Invalid JSON for Place: Must be non-empty object");
    }

    const auto &key = json.begin().key();
    const auto &value = json.begin().value();

    if (key == "Id") {
        return std::make_unique<Identifier>(value.get<std::string>());
    }

    if (key == "Deref") {
        return std::make_unique<Dereference>(buildExpression(value));
    }

    if (key == "ArrayAccess") {
        if (!value.is_object() || !value.contains("array") || !value.contains("idx")) {
            throw std::runtime_error("Invalid JSON for ArrayAccess content");
        }
        return std::make_unique<ArrayAccess>(buildExpression(value.at("array")), buildExpression(value.at("idx")));
    }

    if (key == "FieldAccess") {
        if (!value.is_object() || !value.contains("ptr") || !value.contains("field")) {
            throw std::runtime_error("Invalid JSON for FieldAccess content");
        }
        return std::make_unique<FieldAccess>(buildExpression(value.at("ptr")), value.at("field").get<std::string>());
    }

    throw std::runtime_error("JSON node is not a valid Place kind: " + key);
}

std::unique_ptr<Expression> buildExpression(const nlohmann::json &json) {
    if (!json.is_object() || json.empty()) {
        if (json.is_string() && json.get<std::string>() == "Nil") { // Check if Nil is just a string
            return std::make_unique<Nil>();
        }
        
        if (json.is_object() && json.contains("kind") &&
            json.at("kind") == "Nil") {
            return std::make_unique<Nil>();
        }

        throw std::runtime_error("Invalid JSON for Exp: Must be non-empty object or known literal");
    }

    const auto &key = json.begin().key();
    const auto &value = json.begin().value();

    if (key == "Id" || key == "Deref" || key == "ArrayAccess" || key == "FieldAccess") {
        nlohmann::json placeObj;
        placeObj[key] = value;
        return std::make_unique<Value>(buildPlace(placeObj));
    }
    
    if (key == "Num") {
        return std::make_unique<Number>(value.get<long long>());
    }
    
    if (key == "Nil") {
        return std::make_unique<Nil>();
    }
    
    if (key == "Select") {
        if (!value.is_object() || !value.contains("guard") || !value.contains("tt") || !value.contains("ff")) {
            throw std::runtime_error("Invalid JSON for Select content");
        }
        return std::make_unique<Select>(buildExpression(value.at("guard")),
                                        buildExpression(value.at("tt")),
                                        buildExpression(value.at("ff")));
    }
    if (key == "UnOp") { 
        if (!value.is_array() || value.size() != 2) {
            throw std::runtime_error("Invalid JSON for UnOp content: Expected 2-element array [op, exp]");
        }
        if (!value[0].is_string()) {
            throw std::runtime_error("Invalid JSON for UnOp content: Operator name must be a string");
        }

        UnaryOperand operand;
        std::string opStr = value[0].get<std::string>();
        if (opStr == "Neg")
            operand = UnaryOperand::NEG;
        else if (opStr == "Not")
            operand = UnaryOperand::NOT;
        else
            throw std::runtime_error("Unknown unary operator: " + opStr);
        return std::make_unique<UnaryOperation>(operand, buildExpression(value[1]));
    }
    if (key == "BinOp") {
        if (!value.is_object() || !value.contains("op") || !value.contains("left") || !value.contains("right")) {
            throw std::runtime_error("Invalid JSON for BinaryOperation content");
        }
        BinaryOperand operand;
        std::string opStr = value.at("op").get<std::string>();
        if (opStr == "Add")
            operand = BinaryOperand::ADD;
        else if (opStr == "Sub")
            operand = BinaryOperand::SUB;
        else if (opStr == "Mul")
            operand = BinaryOperand::MUL;
        else if (opStr == "Div")
            operand = BinaryOperand::DIV;
        else if (opStr == "And")
            operand = BinaryOperand::AND;
        else if (opStr == "Or")
            operand = BinaryOperand::OR;
        else if (opStr == "Eq")
            operand = BinaryOperand::EQ;
        else if (opStr == "NotEq")
            operand = BinaryOperand::NOT_EQ;
        else if (opStr == "Lt")
            operand = BinaryOperand::LT;
        else if (opStr == "Lte")
            operand = BinaryOperand::LTE;
        else if (opStr == "Gt")
            operand = BinaryOperand::GT;
        else if (opStr == "Gte")
            operand = BinaryOperand::GTE;
        else
            throw std::runtime_error("Unknown binary operator: " + opStr);
        return std::make_unique<BinaryOperation>(operand, buildExpression(value.at("left")), buildExpression(value.at("right")));
    }
    
    if (key == "NewSingle") {
        return std::make_unique<NewSingleton>(buildType(value));
    }
    
    if (key == "NewArray") {
        if (!value.is_array() || value.size() != 2) {
            throw std::runtime_error("Invalid JSON for NewArray content: Expected 2-element array [Type, Exp]");
        }
        auto type = buildType(value[0]);
        auto sizeExp = buildExpression(value[1]);
        return std::make_unique<NewArray>(std::move(type), std::move(sizeExp));
    }
    
    if (key == "Call") {
        return std::make_unique<CallExpression>(buildFunctionCall(value));
    }
    
    if (key == "Val") {
        return std::make_unique<Value>(buildPlace(value));
    }

    throw std::runtime_error("Unknown/Unhandled expression kind: " + key + " with value " + value.dump());
}

std::unique_ptr<FunctionCall> buildFunctionCall(const nlohmann::json &json) {
    if (!json.is_object() || !json.contains("callee") || !json.contains("args") || !json.at("args").is_array()) {
        throw std::runtime_error("Invalid JSON for FunctionCall");
    }
    
    std::vector<std::unique_ptr<Expression>> args;
    
    for (const auto &arg : json.at("args")) {
        args.push_back(buildExpression(arg));
    }
    
    return std::make_unique<FunctionCall>(buildExpression(json.at("callee")), std::move(args));
}

std::unique_ptr<Statement> buildStatement(const nlohmann::json &json) {
    if (json.is_array()) {
        auto statementsNode = std::make_unique<Statements>();
        
        for (const auto &element : json) {
            statementsNode->statements.push_back(buildStatement(element));
        }
        
        return statementsNode;
    }

    if (json.is_string()) {
        const std::string &kind = json.get<std::string>();
        
        if (kind == "Break") {
            return std::make_unique<Break>();
        }

        if (kind == "Continue") {
            return std::make_unique<Continue>();
        }

        throw std::runtime_error("Unknown simple string statement: " + kind);
    }

    if (!json.is_object() || json.empty()) {
        throw std::runtime_error("Invalid JSON for Statement: Expected non-empty object, array, or specific string (Break/Continue), got: " + json.dump());
    }

    const auto &key = json.begin().key();
    const auto &value = json.begin().value();

    if (key == "Assign") {
        if (!value.is_array() || value.size() != 2) {
            throw std::runtime_error("Invalid JSON for Assign content: Expected [Place, Exp]");
        }
        return std::make_unique<Assignment>(buildPlace(value[0]), buildExpression(value[1]));
    }

    if (key == "Call") {
        return std::make_unique<CallStatement>(buildFunctionCall(value));
    }

    if (key == "If") {
        if (!value.is_object() || !value.contains("guard") || !value.contains("tt")) {
            throw std::runtime_error("Invalid JSON for If content: Missing guard or tt");
        }
        
        std::optional<std::unique_ptr<Statement>> ff = std::nullopt;
        nlohmann::json ffJson = value.value("ff", nlohmann::json());
        
        if (!ffJson.is_null() && !(ffJson.is_array() && ffJson.empty())) {
            ff = buildStatement(ffJson);
        }
        
        return std::make_unique<If>(buildExpression(value.at("guard")), buildStatement(value.at("tt")), std::move(ff));
    }

    if (key == "While") {
        if (!value.is_array() || value.size() != 2) {
            throw std::runtime_error("Invalid JSON for While content: Expected [GuardExp, BodyStmtArray]");
        }

        return std::make_unique<While>(buildExpression(value[0]), buildStatement(value[1]));
    }

    if (key == "Return") {
        std::optional<std::unique_ptr<Expression>> expression = std::nullopt;
        
        if (!value.is_null()) {
            expression = buildExpression(value);
        }

        return std::make_unique<Return>(std::move(expression));
    }

    if (key == "Stmts") {
        if (!value.is_array()) {
            throw std::runtime_error("Invalid JSON for nested Stmts content");
        }

        auto statementsNode = std::make_unique<Statements>();
        
        for (const auto &statement : value) {
            statementsNode->statements.push_back(buildStatement(statement));
        }
        
        return statementsNode;
    }

    throw std::runtime_error("Unknown statement kind object: " + key);
}

Declaration buildDeclaration(const nlohmann::json &json) {
    if (!json.is_object() || !json.contains("name") || !json.contains("typ")) {
        throw std::runtime_error("Invalid JSON for Decl");
    }
    return {json.at("name").get<std::string>(), buildType(json.at("typ"))};
}

std::unique_ptr<FunctionDefinition> buildFunctionDefintion(const nlohmann::json &json) {
    if (!json.is_object() || !json.contains("name") || !json.contains("prms") || !json.contains("rettyp") || !json.contains("locals") || !json.contains("stmts")) {
        throw std::runtime_error("Invalid JSON for Function definition");
    }
    
    auto function = std::make_unique<FunctionDefinition>();
    function->name = json.at("name").get<std::string>();
    function->returnType = buildType(json.at("rettyp"));
    
    for (const auto &param : json.at("prms")) {
        function->params.push_back(buildDeclaration(param));
    }
    
    for (const auto &local : json.at("locals")) {
        function->locals.push_back(buildDeclaration(local));
    }
    
    auto bodyStatements = std::make_unique<Statements>();
    
    if (!json.at("stmts").is_array()) {
        throw std::runtime_error("Invalid JSON: function 'stmts' must be an array");
    }
    
    for (const auto &statement : json.at("stmts")) {
        bodyStatements->statements.push_back(buildStatement(statement));
    }
    
    function->body = std::move(bodyStatements);

    return function;
}

std::unique_ptr<StructDefinition> buildStructDef(const nlohmann::json &json) {
    if (!json.is_object() || !json.contains("name") || !json.contains("fields") || !json.at("fields").is_array()) {
        throw std::runtime_error("Invalid JSON for Struct definition");
    }
    auto struc = std::make_unique<StructDefinition>();
    struc->name = json.at("name").get<std::string>();
    for (const auto &field : json.at("fields")) {
        struc->fields.push_back(buildDeclaration(field));
    }
    return struc;
}

Extern buildExtern(const nlohmann::json &json) {
    if (!json.is_object() || !json.contains("name") || !json.contains("typ")) {
        throw std::runtime_error("Invalid JSON for Extern definition: missing 'name' or 'typ'");
    }

    Extern e;
    e.name = json.at("name").get<std::string>();

    auto builtType = buildType(json.at("typ"));

    if (auto funcType = std::dynamic_pointer_cast<FunctionType>(builtType)) {
        e.returnType = funcType->returnType;
        e.paramTypes = funcType->paramTypes;
    } else {
        throw std::runtime_error("Invalid JSON for Extern definition: 'typ' field is not a function type (Fn)");
    }

    return e;
}

std::unique_ptr<Program> buildProgram(const nlohmann::json &json) {
    if (!json.is_object() || !json.contains("structs") || !json.contains("externs") || !json.contains("functions")) {
        throw std::runtime_error("Invalid JSON for Program root object");
    }
    
    auto program = std::make_unique<Program>();
    for (const auto &s: json.at("structs")) {
        program->structs.push_back(buildStructDefinition(s));
    }

    for (const auto &e: json.at("externs")) {
        program->externs.push_back(buildExtern(e));
    }
    
    for (const auto &f: json.at("functions")) {
        program->functions.push_back(buildFunctionDefintion(f));
    }
    
    return program;
}

Gamma constructGamma(const std::vector<Extern> &externs, const std::vector<std::unique_ptr<FunctionDefinition>> &functions) {
    Gamma gamma;
    for (const auto &e: externs) {
        gamma[e.name] = std::make_shared<FunctionType>(e.paramTypes, e.returnType);
    }
    for (const auto &f: functions) {
        if (f->name != "main") {
            std::vector<std::shared_ptr<Type>> paramTypes;
            for (const auto &param : f->params) {
                paramTypes.push_back(param.type);
            }
            auto functionType = std::make_shared<FunctionType>(std::move(paramTypes), f->returnType);
            gamma[f->name] = std::make_shared<PointerType>(functionType);
        }
    }
    
    return gamma;
}

Delta constructDelta(const std::vector<std::unique_ptr<StructDefinition>> &structs) {
    Delta delta;
    for (const auto &s: structs) {
        std::unordered_map<std::string, std::shared_ptr<Type>> fields;
        for (const auto &field : s->fields) {
            fields[field.name] = field.type;
        }
        delta[s->name] = std::move(fields);
    }
    
    return delta;
}
