#ifndef CHECKER_HPP
#define CHECKER_HPP

#include "json.hpp"
#include "types.hpp"
#include "ast.hpp"

extern std::shared_ptr<Type> buildType(const nlohmann::json &json);
extern std::unique_ptr<Expression> buildExpression(const nlohmann::json &json);
extern std::unique_ptr<Place> buildPlace(const nlohmann::json &json);
extern std::unique_ptr<Statement> buildStatement(const nlohmann::json &json);
extern Declaration buildDeclaration(const nlohmann::json &json);
extern std::unique_ptr<FunctionDefinition> buildFunctionDefinition(const nlohmann::json &json);
extern std::unique_ptr<StructDefinition> buildStructDefinition(const nlohmann::json &json);
extern Extern buildExtern(const nlohmann::json &json);
extern std::unique_ptr<Program> buildProgram(const nlohmann::json &json);
extern std::unique_ptr<FunctionCall> buildFunctionCall(const nlohmann::json &json);

Gamma constructGamma(const std::vector<Extern>& externs, const std::vector<std::unique_ptr<FunctionDefinition>> &functions);
Delta constructDelta(const std::vector<std::unique_ptr<StructDefinition>> &structs);

#endif
