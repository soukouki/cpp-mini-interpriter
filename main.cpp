#include<iostream>
#include<sstream>
#include<string>
#include<vector>
#include<map>
#include<functional>

void run(std::string input);

int main() {
  run(R"(
    (print (concat "Hello, " "World!"))
    (print (getAt "Hello, World!" 4))
    ((fun () (set x 1) (print x)))
    (set add1 (fun (x) (add x 1)))
    (if (eq (add1 1) 1) (print "true") (print "false"))
    (set fact (fun (x) (if (eq x 0) 1 (mul x (fact (sub x 1))))))
    (set i (toInt (read)))
    (print (concat "fact = " (toStr (fact i))))
  )");
}

class Where {
  public:
  int line;
  int column;
  Where(int line, int column) : line(line), column(column) {}
  std::string toString() {
    std::stringstream ss;
    ss << line << ":" << column;
    return ss.str();
  }
  friend std::ostream& operator<<(std::ostream& os, Where& where) {
    os << where.toString();
    return os;
  }
};

enum class TokenType {
    T_LPAREN,
    T_RPAREN,
    T_STRING,
    T_NUMBER,
    T_SYMBOL,
};

std::string tokenTypeToString(TokenType type) {
  switch(type) {
    case TokenType::T_LPAREN:
      return "LPAREN";
    case TokenType::T_RPAREN:
      return "RPAREN";
    case TokenType::T_STRING:
      return "STRING";
    case TokenType::T_NUMBER:
      return "NUMBER";
    case TokenType::T_SYMBOL:
      return "SYMBOL";
  }
  return "UNKNOWN";
}

class Token {
  public:
  TokenType type;
  Where where;
  Token(TokenType type, Where where) : type(type), where(where) {}
};

class StringToken : public Token {
  public:
  std::string value;
  StringToken(std::string value, Where where) : Token(TokenType::T_STRING, where), value(value) {}
};

class NumberToken : public Token {
  public:
  int value;
  NumberToken(int value, Where where) : Token(TokenType::T_NUMBER, where), value(value) {}
};

class SymbolToken : public Token {
  public:
  std::string value;
  SymbolToken(std::string value, Where where) : Token(TokenType::T_SYMBOL, where), value(value) {}
};

std::vector<Token*> tokenize(std::string input) {
  std::vector<Token*> tokens;
  int line = 1;
  int line_start = 0;
  for(int i = 0; i < input.size(); i++) {
    char c = input.at(i);
    if(c == '(') {
      tokens.push_back(new Token(TokenType::T_LPAREN, Where(line, i - line_start)));
    } else if(c == ')') {
      tokens.push_back(new Token(TokenType::T_RPAREN, Where(line, i - line_start)));
    } else if(c == '"') {
      for(int j = i + 1; j < input.size(); j++) {
        if(input.at(j) == '\n') {
          line++;
          line_start = j + 1;
        } else if(input.at(j) == '"') {
          tokens.push_back(new StringToken(input.substr(i + 1, j - i - 1), Where(line, i - line_start)));
          i = j;
          break;
        }
      }
    } else if(c == '#') {
      for(; i < input.size(); i++) {
        if(input.at(i) == '\n') {
          line++;
          line_start = i + 1;
          break;
        }
      }
    } else if(c >= '0' && c <= '9') {
      int value = 0;
      for (int j = i; j < input.size(); j++) {
        if(input.at(j) >= '0' && input.at(j) <= '9') {
          value = value * 10 + (input.at(j) - '0');
        } else {
          tokens.push_back(new NumberToken(value, Where(line, i - line_start)));
          i = j - 1;
          break;
        }
      }
    } else if(c >= 'a' && c <= 'z' || c >= 'A' && c <= 'Z' || c == '_') {
      std::string value = "";
      for(int j = i; j < input.size(); j++) {
        if(input.at(j) >= 'a' && input.at(j) <= 'z' || input.at(j) >= 'A' && input.at(j) <= 'Z' || input.at(j) == '_' || input.at(j) >= '0' && input.at(j) <= '9') {
          value += input.at(j);
        } else {
          tokens.push_back(new SymbolToken(value, Where(line, i - line_start)));
          i = j - 1;
          break;
        }
      }
    } else if(c == ' ' || c == '\t' || c == '\r' || c == '\n') {
      if(c == '\n') {
        line++;
        line_start = i + 1;
      }
    } else {
      auto where = Where(line, i - line_start);
      std::cerr << "unknown char: " << c << " at " << where << std::endl;
    }
  }
  return tokens;
}

void printTokens(std::vector<Token*> tokens, int index) {
  for(int i = 0; i < tokens.size(); i++) {
    if(i == index) {
      std::cout << " >>";
    } else {
      std::cout << "   ";
    }
    if(tokens.at(i)->type == TokenType::T_LPAREN) {
      std::cout << "(";
    } else if(tokens.at(i)->type == TokenType::T_RPAREN) {
      std::cout << ")";
    } else if(tokens.at(i)->type == TokenType::T_STRING) {
      std::cout << "<str: " << ((StringToken*)tokens.at(i))->value << ">";
    } else if(tokens.at(i)->type == TokenType::T_NUMBER) {
      std::cout << "<num: " << ((NumberToken*)tokens.at(i))->value << ">";
    } else if(tokens.at(i)->type == TokenType::T_SYMBOL) {
      std::cout << ((SymbolToken*)tokens.at(i))->value;
    }
  }
  std::cout << std::endl;
}


enum class ASTType {
  A_STRING,
  A_NUMBER,
  A_GET_VALUE,
  A_SET_VALUE,
  A_FUNC_CALL,
  A_LAMBDA,
  A_IF,
  A_STATEMENTS,
};

std::string astTypeToString(ASTType type) {
  switch(type) {
    case ASTType::A_STRING:
      return "STRING";
    case ASTType::A_NUMBER:
      return "NUMBER";
    case ASTType::A_GET_VALUE:
      return "GET_VALUE";
    case ASTType::A_SET_VALUE:
      return "SET_VALUE";
    case ASTType::A_FUNC_CALL:
      return "FUNC_CALL";
    case ASTType::A_LAMBDA:
      return "LAMBDA";
    case ASTType::A_IF:
      return "IF";
    case ASTType::A_STATEMENTS:
      return "STATEMENTS";
  }
  return "UNKNOWN";
}

class AST {
  public:
  ASTType type;
  Where where;
  protected:
  AST(ASTType type, Where where) : type(type), where(where) {}
};

class StringAST : public AST {
  public:
  std::string value;
  StringAST(std::string value, Where where) : AST(ASTType::A_STRING, where), value(value) {}
};

class NumberAST : public AST {
  public:
  int value;
  NumberAST(int value, Where where) : AST(ASTType::A_NUMBER, where), value(value) {}
};

class GetValueAST : public AST {
  public:
  std::string name;
  GetValueAST(std::string name, Where where) : AST(ASTType::A_GET_VALUE, where), name(name) {}
};

class SetValueAST : public AST {
  public:
  std::string name;
  AST* value;
  SetValueAST(std::string name, AST* value, Where where) : AST(ASTType::A_SET_VALUE, where), name(name), value(value) {}
};

class FuncCallAST : public AST {
  public:
  AST* func;
  std::vector<AST*> args;
  FuncCallAST(AST* func, std::vector<AST*> args, Where where) : AST(ASTType::A_FUNC_CALL, where), func(func), args(args) {}
};

class LambdaAST : public AST {
  public:
  std::vector<std::string> args;
  AST* body;
  LambdaAST(std::vector<std::string> args, AST* body, Where where) : AST(ASTType::A_LAMBDA, where), args(args), body(body) {}
};

class IfAST : public AST {
  public:
  AST* cond;
  AST* if_body;
  AST* else_body;
  IfAST(AST* cond, AST* if_body, AST* else_body, Where where) : AST(ASTType::A_IF, where), cond(cond), if_body(if_body), else_body(else_body) {}
};

class StatementsAST : public AST {
  public:
  std::vector<AST*> statements;
  StatementsAST(std::vector<AST*> statements, Where where) : AST(ASTType::A_STATEMENTS, where), statements(statements) {}
};

AST* parseValue(std::vector<Token*> tokens, int& index) {
  Token* token = tokens.at(index);
  if(token->type == TokenType::T_STRING) {
    index++;
    return new StringAST(((StringToken*)token)->value, token->where);
  } else if(token->type == TokenType::T_NUMBER) {
    index++;
    return new NumberAST(((NumberToken*)token)->value, token->where);
  } else if(token->type == TokenType::T_SYMBOL) {
    index++;
    return new GetValueAST(((SymbolToken*)token)->value, token->where);
  }
  return nullptr;
}

AST* parseExpression(std::vector<Token*> tokens, int& index);
AST* parseStatements(std::vector<Token*> tokens, int& index);

// "set name value"
AST* parseSet(std::vector<Token*> tokens, int& index) {
  auto set_token_where = tokens.at(index)->where;
  if(tokens.at(index)->type == TokenType::T_SYMBOL && ((SymbolToken*)tokens.at(index))->value == "set") {
    index++;
    if(tokens.at(index)->type == TokenType::T_SYMBOL) {
      std::string name = ((SymbolToken*)tokens.at(index))->value;
      index++;
      AST* value = parseExpression(tokens, index);
      if(value != nullptr) {
        return new SetValueAST(name, value, set_token_where);
      }
    }
  }
  return nullptr;
}

// "if cond if_body else_body"
AST* parseIf(std::vector<Token*> tokens, int& index) {
  auto if_token_where = tokens.at(index)->where;
  if(tokens.at(index)->type == TokenType::T_SYMBOL && ((SymbolToken*)tokens.at(index))->value == "if") {
    index++;
    AST* cond = parseExpression(tokens, index);
    if(cond != nullptr) {
      AST* if_body = parseExpression(tokens, index);
      if(if_body != nullptr) {
        AST* else_body = parseExpression(tokens, index);
        if(else_body != nullptr) {
          return new IfAST(cond, if_body, else_body, if_token_where);
        }
      }
    }
  }
  return nullptr;
}

// "fun (arg1 arg2 ...) body"
AST* parseLambda(std::vector<Token*> tokens, int& index) {
  auto fun_token_where = tokens.at(index)->where;
  if(tokens.at(index)->type == TokenType::T_SYMBOL && ((SymbolToken*)tokens.at(index))->value == "fun") {
    index++;
    if(tokens.at(index)->type == TokenType::T_LPAREN) {
      index++;
      std::vector<std::string> args;
      while(tokens.at(index)->type == TokenType::T_SYMBOL) {
        args.push_back(((SymbolToken*)tokens.at(index))->value);
        index++;
      }
      if(tokens.at(index)->type == TokenType::T_RPAREN) {
        index++;
        AST* body = parseStatements(tokens, index);
        if(body != nullptr) {
          return new LambdaAST(args, body, fun_token_where);
        }
      }
    }
  }
  return nullptr;
}

// "expr arg1 arg2 ..."
AST* parseFuncCall(std::vector<Token*> tokens, int& index) {
  auto func_call_token_where = tokens.at(index)->where;
  AST* func = parseExpression(tokens, index);
  if(func != nullptr) {
    std::vector<AST*> args;
    while(true) {
      AST* arg = parseExpression(tokens, index);
      if(arg == nullptr) {
        break;
      }
      args.push_back(arg);
    }
    return new FuncCallAST(func, args, func_call_token_where);
  }
  return nullptr;
}

// (expr ...) or value
AST* parseExpression(std::vector<Token*> tokens, int& index) {
  if(tokens.at(index)->type == TokenType::T_LPAREN) {
    index++;
    AST* ast = parseSet(tokens, index);
    if(ast == nullptr) {
      ast = parseIf(tokens, index);
    }
    if(ast == nullptr) {
      ast = parseLambda(tokens, index);
    }
    if(ast == nullptr) {
      ast = parseFuncCall(tokens, index);
    }
    if(ast == nullptr) {
      std::cerr << "unknown expression at" << tokens.at(index)->where << std::endl;
      return nullptr;
    }
    if(tokens.at(index)->type == TokenType::T_RPAREN) {
      index++;
      return ast;
    }
    return nullptr;
  } else {
    return parseValue(tokens, index);
  }
}

AST* parseStatements(std::vector<Token*> tokens, int& index) {
  auto statements_token_where = tokens.at(index)->where;
  std::vector<AST*> statements;
  while(index < tokens.size()) {
    AST* statement = parseExpression(tokens, index);
    if(statement == nullptr) {
      break;
    }
    statements.push_back(statement);
  }
  return new StatementsAST(statements, statements_token_where);
}

AST* parse(std::vector<Token*> tokens) {
  int index = 0;
  auto ast = parseStatements(tokens, index);
  if(index < tokens.size()) {
    std::cerr << "parse error at " << tokens.at(index)->where << std::endl;
    return nullptr;
  }
  return ast;
}


enum class ValueType {
  V_STRING,
  V_NUMBER,
  V_NULL,
  V_DEFINED_FUNCTION,
  V_BUILD_IN_FUNCTION,
};

std::string valueTypeToString(ValueType type) {
  switch(type) {
    case ValueType::V_STRING:
      return "STRING";
    case ValueType::V_NUMBER:
      return "NUMBER";
    case ValueType::V_NULL:
      return "NULL";
    case ValueType::V_DEFINED_FUNCTION:
      return "DEFINED_FUNCTION";
    case ValueType::V_BUILD_IN_FUNCTION:
      return "BUILD_IN_FUNCTION";
  }
  return "UNKNOWN";
}

class Value {
  public:
  ValueType type;
  virtual Value* copy() = 0;
  protected:
  Value(ValueType type) : type(type) {}
};

class NullValue : public Value {
  public:
  NullValue() : Value(ValueType::V_NULL) {}
  NullValue* copy() {
    return this;
  }
};

class Environment {
  std::map<std::string, Value*> values;
  Environment* parent;
  public:
  Environment() : parent(nullptr) {}
  Environment(Environment* parent) : parent(parent) {}
  Environment* createChild() {
    return new Environment(this);
  }
  bool find(std::string name) {
    if(values.find(name) == values.end()) {
      if(parent == nullptr) {
        return false;
      }
      return parent->find(name);
    }
    return true;
  }
  Value* get(std::string name) {
    if(values.find(name) == values.end()) {
      return parent->get(name);
    }
    return values.at(name);
  }
  void set(std::string name, Value* value) {
    values[name] = value;
  }
};

class StringValue : public Value {
  public:
  std::string value;
  StringValue(std::string value) : Value(ValueType::V_STRING), value(value) {}
  StringValue* copy() {
    return new StringValue(value);
  }
};

class NumberValue : public Value {
  public:
  int value;
  NumberValue(int value) : Value(ValueType::V_NUMBER), value(value) {}
  NumberValue* copy() {
    return new NumberValue(value);
  }
};

class FunctionValue : public Value {
  public:
  int args_size;
  std::vector<Value*> applied_args;
  Value* apply(std::vector<Value*> args, Where where) {
    applied_args.insert(applied_args.end(), args.begin(), args.end());
    if(applied_args.size() == args_size) {
      return this->call(applied_args, where);
    } else if(applied_args.size() > args_size) {
      std::cerr << "too many args at " << where << std::endl;
      return new NullValue();
    } else {
      return this;
    }
  }
  protected:
  virtual Value* call(std::vector<Value*> args, Where where) = 0;
  FunctionValue(ValueType type, int args_size) : Value(type), args_size(args_size) {}
};

Value* execute(AST* ast, Environment* env);

class DefinedFunctionValue : public FunctionValue {
  public:
  std::vector<std::string> arg_names;
  AST* body;
  Environment* env;
  DefinedFunctionValue(int args_size, std::vector<std::string> arg_names, AST* body, Environment* env) : FunctionValue(ValueType::V_DEFINED_FUNCTION, args_size), arg_names(arg_names), body(body), env(env) {}
  Value* call(std::vector<Value*> args, Where where) {
    auto new_env = env->createChild();
    for(int i = 0; i < args.size(); i++) {
      new_env->set(arg_names.at(i), args.at(i));
    }
    return execute(body, new_env);
  }
  DefinedFunctionValue* copy() {
    return new DefinedFunctionValue(args_size, arg_names, body, env);
  }
};

class BuildInFunctionValue : public FunctionValue {
  public:
  std::function<Value*(std::vector<Value*>, Where)> func;
  BuildInFunctionValue(std::function<Value*(std::vector<Value*>, Where)> func, int args_size) : FunctionValue(ValueType::V_BUILD_IN_FUNCTION, args_size), func(func) {}
  Value* call(std::vector<Value*> args, Where where) {
    return func(args, where);
  }
  BuildInFunctionValue* copy() {
    return new BuildInFunctionValue(func, args_size);
  }
};

Value* execute(AST* ast, Environment* env) {
  if(ast->type == ASTType::A_STRING) {
    return new StringValue(((StringAST*)ast)->value);
  } else if(ast->type == ASTType::A_NUMBER) {
    return new NumberValue(((NumberAST*)ast)->value);
  } else if(ast->type == ASTType::A_GET_VALUE) {
    auto get_value_ast = (GetValueAST*)ast;
    if(env->find(get_value_ast->name)) {
      return env->get(get_value_ast->name)->copy();
    } else {
      std::cerr << "undefined variable: " << get_value_ast->name << " at " << get_value_ast->where << std::endl;
    }
  } else if(ast->type == ASTType::A_SET_VALUE) {
    auto set_value_ast = (SetValueAST*)ast;
    env->set(set_value_ast->name, execute(set_value_ast->value, env));
    return new NullValue();
  } else if(ast->type == ASTType::A_FUNC_CALL) {
    auto func_call_ast = (FuncCallAST*)ast;
    Value* func = execute(func_call_ast->func, env);
    if(func->type == ValueType::V_DEFINED_FUNCTION || func->type == ValueType::V_BUILD_IN_FUNCTION) {
      std::vector<Value*> args;
      for(int i = 0; i < func_call_ast->args.size(); i++) {
        args.push_back(execute(func_call_ast->args.at(i), env));
      }
      return ((FunctionValue*)func)->apply(args, func_call_ast->where);
    } else {
      std::cerr << "not a function: " << valueTypeToString(func->type) << " at " << func_call_ast->where << std::endl;
    }
  } else if(ast->type == ASTType::A_LAMBDA) {
    auto lambda_ast = (LambdaAST*)ast;
    return new DefinedFunctionValue(lambda_ast->args.size(), lambda_ast->args, lambda_ast->body, env);
  } else if(ast->type == ASTType::A_IF) {
    auto if_ast = (IfAST*)ast;
    Value* cond = execute(if_ast->cond, env);
    if(cond->type == ValueType::V_NUMBER) {
      if(((NumberValue*)cond)->value != 0) {
        return execute(if_ast->if_body, env);
      } else {
        return execute(if_ast->else_body, env);
      }
    }
  } else if(ast->type == ASTType::A_STATEMENTS) {
    auto statements_ast = (StatementsAST*)ast;
    Value* value = new NullValue();
    for(int i = 0; i < statements_ast->statements.size(); i++) {
      value = execute(statements_ast->statements.at(i), env);
    }
    return value;
  }
  return new NullValue();
}

Value* printFunction(std::vector<Value*> args, Where where) {
  auto value = args.at(0);
  if(value->type == ValueType::V_STRING) {
    std::cout << ((StringValue*)value)->value;
  } else if(value->type == ValueType::V_NUMBER) {
    std::cout << ((NumberValue*)value)->value;
  } else {
    std::cout << valueTypeToString(value->type);
  }
  std::cout << std::endl;
  return new NullValue();
}

Value* concatFunction(std::vector<Value*> args, Where where) {
  auto value1 = args.at(0);
  auto value2 = args.at(1);
  if(value1->type == ValueType::V_STRING && value2->type == ValueType::V_STRING) {
    return new StringValue(((StringValue*)value1)->value + ((StringValue*)value2)->value);
  }
  std::cerr << "type error at concat: 1st arg is " << valueTypeToString(value1->type) << ", 2nd arg is " << valueTypeToString(value2->type) << " at " << where << std::endl;
  return new NullValue();
}

Value* getAtFunction(std::vector<Value*> args, Where where) {
  auto value1 = args.at(0);
  auto value2 = args.at(1);
  if(value1->type == ValueType::V_STRING && value2->type == ValueType::V_NUMBER) {
    auto value = ((StringValue*)value1)->value;
    auto index = ((NumberValue*)value2)->value;
    if(index >= 0 && index < value.size()) {
      return new StringValue(value.substr(index, 1));
    } else {
      std::cerr << "index out of range: " << index << " at " << where << std::endl;
    }
  }
  std::cerr << "type error at getAt: 1st arg is " << valueTypeToString(value1->type) << ", 2nd arg is " << valueTypeToString(value2->type) << " at " << where << std::endl;
  return new NullValue();
}

Value* lengthFunction(std::vector<Value*> args, Where where) {
  auto value = args.at(0);
  if(value->type == ValueType::V_STRING) {
    return new NumberValue(((StringValue*)value)->value.size());
  }
  std::cerr << "type error at length: arg is " << valueTypeToString(value->type) << " at " << where << std::endl;
  return new NullValue();
}

Value* addFunction(std::vector<Value*> args, Where where) {
  auto value1 = args.at(0);
  auto value2 = args.at(1);
  if(value1->type == ValueType::V_NUMBER && value2->type == ValueType::V_NUMBER) {
    return new NumberValue(((NumberValue*)value1)->value + ((NumberValue*)value2)->value);
  }
  std::cerr << "type error at add: 1st arg is " << valueTypeToString(value1->type) << ", 2nd arg is " << valueTypeToString(value2->type) << " at " << where << std::endl;
  return new NullValue();
}

Value* subFunction(std::vector<Value*> args, Where where) {
  auto value1 = args.at(0);
  auto value2 = args.at(1);
  if(value1->type == ValueType::V_NUMBER && value2->type == ValueType::V_NUMBER) {
    return new NumberValue(((NumberValue*)value1)->value - ((NumberValue*)value2)->value);
  }
  std::cerr << "type error at sub: 1st arg is " << valueTypeToString(value1->type) << ", 2nd arg is " << valueTypeToString(value2->type) << " at " << where << std::endl;
  return new NullValue();
}

Value* mulFunction(std::vector<Value*> args, Where where) {
  auto value1 = args.at(0);
  auto value2 = args.at(1);
  if(value1->type == ValueType::V_NUMBER && value2->type == ValueType::V_NUMBER) {
    return new NumberValue(((NumberValue*)value1)->value * ((NumberValue*)value2)->value);
  }
  std::cerr << "type error at mul: 1st arg is " << valueTypeToString(value1->type) << ", 2nd arg is " << valueTypeToString(value2->type) << " at " << where << std::endl;
  return new NullValue();
}

Value* divFunction(std::vector<Value*> args, Where where) {
  auto value1 = args.at(0);
  auto value2 = args.at(1);
  if(value1->type == ValueType::V_NUMBER && value2->type == ValueType::V_NUMBER) {
    if(((NumberValue*)value2)->value == 0) {
      std::cerr << "zero division at " << where << std::endl;
      return new NullValue();
    }
    return new NumberValue(((NumberValue*)value1)->value / ((NumberValue*)value2)->value);
  }
  std::cerr << "type error at div: 1st arg is " << valueTypeToString(value1->type) << ", 2nd arg is " << valueTypeToString(value2->type) << " at " << where << std::endl;
  return new NullValue();
}

Value* modFunction(std::vector<Value*> args, Where where) {
  auto value1 = args.at(0);
  auto value2 = args.at(1);
  if(value1->type == ValueType::V_NUMBER && value2->type == ValueType::V_NUMBER) {
    return new NumberValue(((NumberValue*)value1)->value % ((NumberValue*)value2)->value);
  }
  std::cerr << "type error at mod: 1st arg is " << valueTypeToString(value1->type) << ", 2nd arg is " << valueTypeToString(value2->type) << " at " << where << std::endl;
  return new NullValue();
}

Value* eqFunction(std::vector<Value*> args, Where where) {
  auto value1 = args.at(0);
  auto value2 = args.at(1);
  if(value1->type == ValueType::V_NUMBER && value2->type == ValueType::V_NUMBER) {
    return new NumberValue(((NumberValue*)value1)->value == ((NumberValue*)value2)->value);
  }
  if(value1->type == ValueType::V_STRING && value2->type == ValueType::V_STRING) {
    return new NumberValue(((StringValue*)value1)->value == ((StringValue*)value2)->value);
  }
  if(value1->type == ValueType::V_NULL && value2->type == ValueType::V_NULL) {
    return new NumberValue(1);
  }
  return new NumberValue(0);
}

Value* ltFunction(std::vector<Value*> args, Where where) {
  auto value1 = args.at(0);
  auto value2 = args.at(1);
  if(value1->type == ValueType::V_NUMBER && value2->type == ValueType::V_NUMBER) {
    return new NumberValue(((NumberValue*)value1)->value < ((NumberValue*)value2)->value);
  }
  std::cerr << "type error at lt: 1st arg is " << valueTypeToString(value1->type) << ", 2nd arg is " << valueTypeToString(value2->type) << " at " << where << std::endl;
  return new NullValue();
}

Value* gtFunction(std::vector<Value*> args, Where where) {
  auto value1 = args.at(0);
  auto value2 = args.at(1);
  if(value1->type == ValueType::V_NUMBER && value2->type == ValueType::V_NUMBER) {
    return new NumberValue(((NumberValue*)value1)->value > ((NumberValue*)value2)->value);
  }
  std::cerr << "type error at gt: 1st arg is " << valueTypeToString(value1->type) << ", 2nd arg is " << valueTypeToString(value2->type) << " at " << where << std::endl;
  return new NullValue();
}

Value* toIntFunction(std::vector<Value*> args, Where where) {
  auto value = args.at(0);
  if(value->type == ValueType::V_STRING) {
    try {
      return new NumberValue(std::stoi(((StringValue*)value)->value));
    } catch(std::invalid_argument e) {
      return new NumberValue(0);
    }
  }
  std::cerr << "type error at toInt: arg is " << valueTypeToString(value->type) << " at " << where << std::endl;
  return new NullValue();
}

Value* toStringFunction(std::vector<Value*> args, Where where) {
  auto value = args.at(0);
  if(value->type == ValueType::V_NUMBER) {
    return new StringValue(std::to_string(((NumberValue*)value)->value));
  }
  std::cerr << "type error at toString: arg is " << valueTypeToString(value->type) << " at " << where << std::endl;
  return new NullValue();
}

Value* typeFunction(std::vector<Value*> args, Where where) {
  auto value = args.at(0);
  return new StringValue(valueTypeToString(value->type));
}

Value* readFunction(std::vector<Value*> args, Where where) {
  std::string value;
  std::getline(std::cin, value);
  return new StringValue(value);
}

Environment* defaultEnvironment() {
  Environment* env = new Environment();
  env->set("print", new BuildInFunctionValue(printFunction, 1));
  env->set("concat", new BuildInFunctionValue(concatFunction, 2));
  env->set("getAt", new BuildInFunctionValue(getAtFunction, 2));
  env->set("length", new BuildInFunctionValue(lengthFunction, 1));
  env->set("add", new BuildInFunctionValue(addFunction, 2));
  env->set("sub", new BuildInFunctionValue(subFunction, 2));
  env->set("mul", new BuildInFunctionValue(mulFunction, 2));
  env->set("div", new BuildInFunctionValue(divFunction, 2));
  env->set("mod", new BuildInFunctionValue(modFunction, 2));
  env->set("eq", new BuildInFunctionValue(eqFunction, 2));
  env->set("lt", new BuildInFunctionValue(ltFunction, 2));
  env->set("gt", new BuildInFunctionValue(gtFunction, 2));
  env->set("toInt", new BuildInFunctionValue(toIntFunction, 1));
  env->set("toStr", new BuildInFunctionValue(toStringFunction, 1));
  env->set("type", new BuildInFunctionValue(typeFunction, 1));
  env->set("read", new BuildInFunctionValue(readFunction, 0));
  env->set("null", new NullValue());
  return env;
}

void run(std::string input) {
  std::vector<Token*> tokens = tokenize(input);
  AST* ast = parse(tokens);
  if(ast == nullptr) {
    return;
  }
  Environment* env = defaultEnvironment();
  execute(ast, env);
}

