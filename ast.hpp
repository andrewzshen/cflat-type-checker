#ifndef AST_HPP
#define AST_HPP

#include <optional>

#include "types.hpp"
#include "visitor.hpp"

// Node
struct Node;

// Declarations
struct Declaration;

// Expressions
struct Expression;
struct Value;
struct Number;
struct Nil;
struct Select;
struct UnaryOperation;
struct BinaryOperation;
struct NewSingleton;
struct NewArray;
struct CallExpression;

// Places
struct Place;
struct Identifier;
struct Dereference;
struct ArrayAccess;
struct FieldAccess;

// Function call
struct FunctionCall;

// Statements
struct Statement;
struct Statements;
struct Assignment;
struct CallStatement;
struct If;
struct While;
struct Break;
struct Continue;
struct Return;

// High level nodes
struct StructDefinition;
struct Extern;
struct FunctionDefinition;
struct Program;

// Node
struct Node {
    virtual ~Node() = default;
    virtual std::string toString() const = 0;
    virtual void accept(Visitor &visitor) = 0;
};

// Declaration
struct Declaration : public Node {
    std::string name;
    std::shared_ptr<Type> type;

    Declaration(std::string name, std::shared_ptr<Type> type);
    std::string toString() const override; 
    void accept(Visitor &visitor) override;
};

// --------------------------------- Expressions -----------------------------------------
struct Expression : public Node {
    virtual std::shared_ptr<Type> check(const Gamma &gamma, const Delta &delta) const = 0;
    virtual std::string toString() const override = 0;
    virtual void accept(Visitor &visitor) override = 0;
};

struct Value : public Expression {
    std::unique_ptr<Place> place;
    
    explicit Value(std::unique_ptr<Place> place);
    
    std::shared_ptr<Type> check(const Gamma &gamma, const Delta &delta) const override;
    std::string toString() const override;
    void accept(Visitor &visitor) override;
};

struct Number : public Expression {
    long long value;
    
    explicit Number(long long value);

    std::shared_ptr<Type> check(const Gamma &gamma, const Delta &delta) const override;
    std::string toString() const override;
    void accept(Visitor &visitor) override;
};

struct Nil : public Expression {
    std::shared_ptr<Type> check(const Gamma &gamma, const Delta &delta) const override;
    std::string toString() const override;
    void accept(Visitor &visitor) override;
};

struct Select : public Expression {
    std::unique_ptr<Expression> guard;
    std::unique_ptr<Expression> ttCase;
    std::unique_ptr<Expression> ffCase;
    
    Select(std::unique_ptr<Expression> guard, std::unique_ptr<Expression> ttCase, std::unique_ptr<Expression> ffCase);
    
    std::shared_ptr<Type> check(const Gamma &gamma, const Delta &delta) const override;
    std::string toString() const override;
    void accept(Visitor &visitor) override;
};

enum class UnaryOperand { 
    NEG, 
    NOT 
};

struct UnaryOperation : public Expression {
    UnaryOperand operand;
    std::unique_ptr<Expression> expression;
    
    UnaryOperation(UnaryOperand operand, std::unique_ptr<Expression> expression);
    
    std::shared_ptr<Type> check(const Gamma &gamma, const Delta &delta) const override;
    std::string toString() const override;
    void accept(Visitor &visitor) override;
};

enum class BinaryOperand {
    ADD, 
    SUB, 
    MUL,
    DIV, 
    AND, 
    OR, 
    EQ, 
    NOT_EQ, 
    LT, 
    LTE, 
    GT,
    GTE
};

struct BinaryOperation : public Expression {
    BinaryOperand operand;
    std::unique_ptr<Expression> lhs;
    std::unique_ptr<Expression> rhs;
    
    BinaryOperation(BinaryOperand operand, std::unique_ptr<Expression> lhs, std::unique_ptr<Expression> rhs);
    
    std::shared_ptr<Type> check(const Gamma &gamma, const Delta &delta) const override;
    std::string toString() const override;
    void accept(Visitor &visitor) override;
};

struct NewSingleton : public Expression {
    std::shared_ptr<Type> type;
    
    explicit NewSingleton(std::shared_ptr<Type> type);
    
    std::shared_ptr<Type> check(const Gamma &gamma, const Delta &delta) const override;
    std::string toString() const override;
    void accept(Visitor &visitor) override;
};

struct NewArray : public Expression {
    std::shared_ptr<Type> type;
    std::unique_ptr<Expression> size;
    
    NewArray(std::shared_ptr<Type> type, std::unique_ptr<Expression> size);
    
    std::shared_ptr<Type> check(const Gamma &gamma, const Delta &delta) const override;
    std::string toString() const override;
    void accept(Visitor &visitor) override;
};

struct CallExpression : public Expression {
    std::unique_ptr<FunctionCall> functionCall;
    
    explicit CallExpression(std::unique_ptr<FunctionCall> functionCall);

    std::shared_ptr<Type> check(const Gamma &gamma, const Delta &delta) const override;
    std::string toString() const override;
    void accept(Visitor &visitor) override;
};

// --------------------------------- Places -----------------------------------------
struct Place : public Node {
    virtual std::shared_ptr<Type> check(const Gamma &gamma, const Delta &delta) const = 0;
    virtual std::string toString() const override = 0;
    virtual void accept(Visitor &visitor) override = 0;
};

struct Identifier : public Place {
    std::string name;
    
    explicit Identifier(std::string name);
    
    std::shared_ptr<Type> check(const Gamma &gamma, const Delta &delta) const override;
    std::string toString() const override;
    void accept(Visitor &visitor) override;
};

struct Dereference : public Place {
    std::unique_ptr<Expression> expression;
    
    explicit Dereference(std::unique_ptr<Expression> expression);
    
    std::shared_ptr<Type> check(const Gamma &gamma, const Delta &delta) const override;
    std::string toString() const override;
    void accept(Visitor &visitor) override;
};

struct ArrayAccess : public Place {
    std::unique_ptr<Expression> array;
    std::unique_ptr<Expression> index;

    ArrayAccess(std::unique_ptr<Expression> array, std::unique_ptr<Expression> index);
    
    std::shared_ptr<Type> check(const Gamma &gamma, const Delta &delta) const override;
    std::string toString() const override;
    void accept(Visitor &visitor) override;
};

struct FieldAccess : public Place {
    std::unique_ptr<Expression> pointer;
    std::string field;

    FieldAccess(std::unique_ptr<Expression> pointer, std::string field);
    
    std::shared_ptr<Type> check(const Gamma &gamma, const Delta &delta) const override;
    std::string toString() const override;
    void accept(Visitor &visitor) override;
};

// Function call
struct FunctionCall: Node {
    std::unique_ptr<Expression> callee;
    std::vector<std::unique_ptr<Expression>> args;

    FunctionCall(std::unique_ptr<Expression> callee, std::vector<std::unique_ptr<Expression>> args);
        
    std::shared_ptr<Type> check(const Gamma &gamma, const Delta &delta) const;
    std::string toString() const override;
    void accept(Visitor &visitor) override;
};

// ------------------------------------ Statements --------------------------------
struct Statement : public Node {
    virtual bool check(const Gamma &gamma, const Delta &delta, const std::shared_ptr<Type> &returnType, bool inLoop) const = 0;
};

struct Statements : public Statement {
    std::vector<std::unique_ptr<Statement>> statements;
    
    bool check(const Gamma &gamma, const Delta &delta, const std::shared_ptr<Type> &returnType, bool inLoop) const override;
    std::string toString() const override;
    void accept(Visitor &visitor) override;
};

struct Assignment : public Statement {
    std::unique_ptr<Place> place;
    std::unique_ptr<Expression> expression;

    Assignment(std::unique_ptr<Place> place, std::unique_ptr<Expression> expression);
    
    bool check(const Gamma &gamma, const Delta &delta, const std::shared_ptr<Type> &returnType, bool inLoop) const override;
    std::string toString() const override;
    void accept(Visitor &visitor) override;
};

struct CallStatement : public Statement {
    std::unique_ptr<FunctionCall> functionCall;

    explicit CallStatement(std::unique_ptr<FunctionCall> functionCall);
    
    bool check(const Gamma &gamma, const Delta &delta, const std::shared_ptr<Type> &returnType, bool inLoop) const override;
    std::string toString() const override;
    void accept(Visitor &visitor) override;
};

struct If : public Statement {
    std::unique_ptr<Expression> guard;
    std::unique_ptr<Statement> happyPath;
    std::optional<std::unique_ptr<Statement>> unhappyPath;

    If(std::unique_ptr<Expression> guard, std::unique_ptr<Statement> happyPath, std::optional<std::unique_ptr<Statement>> unhappyPath);
    
    bool check(const Gamma &gamma, const Delta &delta, const std::shared_ptr<Type> &returnType, bool inLoop) const override;
    std::string toString() const override;
    void accept(Visitor &visitor) override;
};

struct While : public Statement {
    std::unique_ptr<Expression> guard;
    std::unique_ptr<Statement> body;

    While(std::unique_ptr<Expression> guard, std::unique_ptr<Statement> body);

    bool check(const Gamma &gamma, const Delta &delta, const std::shared_ptr<Type> &returnType, bool inLoop) const override;
    std::string toString() const override;
    void accept(Visitor &visitor) override;
};

struct Break : public Statement {
    bool check(const Gamma &gamma, const Delta &delta, const std::shared_ptr<Type> &returnType, bool inLoop) const override;
    std::string toString() const override;
    void accept(Visitor &visitor) override;
};

struct Continue : public Statement {
    bool check(const Gamma &gamma, const Delta &delta, const std::shared_ptr<Type> &returnType, bool inLoop) const override;
    std::string toString() const override;
    void accept(Visitor &visitor) override;
};

struct Return : public Statement {
    std::optional<std::unique_ptr<Expression>> expression;
    
    explicit Return(std::optional<std::unique_ptr<Expression>> expression);

    bool check(const Gamma &gamma, const Delta &delta, const std::shared_ptr<Type> &returnType, bool inLoop) const override;
    std::string toString() const override;
    void accept(Visitor &visitor) override;
};

// ------------------------------------ Top level nodes --------------------------------
struct StructDefinition : public Node {
    std::string name;
    std::vector<Declaration> fields;

    void check(const Gamma &gamma, const Delta &delta) const;
    std::string toString() const override;
    void accept(Visitor &visitor) override;
};

struct Extern : public Node {
    std::string name;
    std::vector<std::shared_ptr<Type>> paramTypes;
    std::shared_ptr<Type> returnType;

    std::string toString() const override;
    void accept(Visitor &visitor) override;
};


struct FunctionDefinition : public Node {
    std::string name;
    std::vector<Declaration> params;
    std::shared_ptr<Type> returnType;
    std::vector<Declaration> locals;
    std::unique_ptr<Statement> body;

    void check(const Gamma &gamma, const Delta &delta) const;
    std::string toString() const override;
    void accept(Visitor &visitor) override;
};


struct Program : public Node {
    std::vector<std::unique_ptr<StructDefinition>> structs;
    std::vector<Extern> externs;
    std::vector<std::unique_ptr<FunctionDefinition>> functions;

    void check() const;
    std::string toString() const override;
    void accept(Visitor &visitor) override;
};

#endif
