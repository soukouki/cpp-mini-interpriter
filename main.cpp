#include<iostream>
#include<string>
#include<vector>
#include<map>
#include<functional>

void run(std::string input);

int main() {
  run(R"(
    (print (concat "Hello, " "World!"))
    (print (getAt "Hello, World!" 4))
    (set x 1)
    (print (add x 2))
    (set add1 (fun (x) (add x 1)))
    (if (eq (add1 1) 1) (print "true") (print "false"))
    (set fact (fun (x) (if (eq x 0) 1 (mul x (fact (sub x 1))))))
    (print (fact 3))
    (set i (toInt (read)))
    (print (concat "fact = " (toStr (fact i))))
  )");
}

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
  Token(TokenType type) : type(type) {}
};

class StringToken : public Token {
  public:
  std::string value;
  StringToken(std::string value) : Token(TokenType::T_STRING), value(value) {}
};

class NumberToken : public Token {
  public:
  int value;
  NumberToken(int value) : Token(TokenType::T_NUMBER), value(value) {}
};

class SymbolToken : public Token {
  public:
  std::string value;
  SymbolToken(std::string value) : Token(TokenType::T_SYMBOL), value(value) {}
};

std::vector<Token*> tokenize(std::string input) {
  std::vector<Token*> tokens;
  for(int i = 0; i < input.size(); i++) {
    char c = input.at(i);
    if(c == '(') {
      tokens.push_back(new Token(TokenType::T_LPAREN));
    } else if(c == ')') {
      tokens.push_back(new Token(TokenType::T_RPAREN));
    } else if(c == '"') {
      for(int j = i + 1; j < input.size(); j++) {
        if(input.at(j) == '"') {
          tokens.push_back(new StringToken(input.substr(i + 1, j - i - 1)));
          i = j;
          break;
        }
      }
    } else if(c == '#') {
      for(; i < input.size() && input.at(i) != '\n'; i++);
    } else if(c >= '0' && c <= '9') {
      int value = 0;
      for (int j = i; j < input.size(); j++) {
        if(input.at(j) >= '0' && input.at(j) <= '9') {
          value = value * 10 + (input.at(j) - '0');
        } else {
          tokens.push_back(new NumberToken(value));
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
          tokens.push_back(new SymbolToken(value));
          i = j - 1;
          break;
        }
      }
    }
  }
  return tokens;
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
  protected:
  AST(ASTType type) : type(type) {}
};

class StringAST : public AST {
  public:
  std::string value;
  StringAST(std::string value) : AST(ASTType::A_STRING), value(value) {}
};

class NumberAST : public AST {
  public:
  int value;
  NumberAST(int value) : AST(ASTType::A_NUMBER), value(value) {}
};

class GetValueAST : public AST {
  public:
  std::string name;
  GetValueAST(std::string name) : AST(ASTType::A_GET_VALUE), name(name) {}
};

class SetValueAST : public AST {
  public:
  std::string name;
  AST* value;
  SetValueAST(std::string name, AST* value) : AST(ASTType::A_SET_VALUE), name(name), value(value) {}
};

class FuncCallAST : public AST {
  public:
  AST* func;
  std::vector<AST*> args;
  FuncCallAST(AST* func, std::vector<AST*> args) : AST(ASTType::A_FUNC_CALL), func(func), args(args) {}
};

class LambdaAST : public AST {
  public:
  std::vector<std::string> args;
  AST* body;
  LambdaAST(std::vector<std::string> args, AST* body) : AST(ASTType::A_LAMBDA), args(args), body(body) {}
};

class IfAST : public AST {
  public:
  AST* cond;
  AST* if_body;
  AST* else_body;
  IfAST(AST* cond, AST* if_body, AST* else_body) : AST(ASTType::A_IF), cond(cond), if_body(if_body), else_body(else_body) {}
};

class StatementsAST : public AST {
  public:
  std::vector<AST*> statements;
  StatementsAST(std::vector<AST*> statements) : AST(ASTType::A_STATEMENTS), statements(statements) {}
};

AST* parseValue(std::vector<Token*> tokens, int& index) {
  Token* token = tokens.at(index);
  if(token->type == TokenType::T_STRING) {
    index++;
    return new StringAST(((StringToken*)token)->value);
  } else if(token->type == TokenType::T_NUMBER) {
    index++;
    return new NumberAST(((NumberToken*)token)->value);
  } else if(token->type == TokenType::T_SYMBOL) {
    index++;
    return new GetValueAST(((SymbolToken*)token)->value);
  }
  return nullptr;
}

AST* parseExpression(std::vector<Token*> tokens, int& index) {
  Token* token = tokens.at(index);
  if(token->type == TokenType::T_LPAREN) {
    index++;
    token = tokens.at(index);
    if(token->type == TokenType::T_SYMBOL) {
      std::string symbol = ((SymbolToken*)token)->value;
      index++;
      if(symbol == "set") {
        token = tokens.at(index);
        if(token->type == TokenType::T_SYMBOL) {
          std::string name = ((SymbolToken*)token)->value;
          index++;
          AST* value = parseExpression(tokens, index);
          token = tokens.at(index);
          if(token->type == TokenType::T_RPAREN) {
            index++;
            return new SetValueAST(name, value);
          }
        }
      } else if(symbol == "fun") {
        token = tokens.at(index);
        if(token->type == TokenType::T_LPAREN) {
          index++;
          std::vector<std::string> args;
          token = tokens.at(index);
          while(token->type == TokenType::T_SYMBOL) {
            args.push_back(((SymbolToken*)token)->value);
            index++;
            token = tokens.at(index);
          }
          if(token->type == TokenType::T_RPAREN) {
            index++;
            AST* body = parseExpression(tokens, index);
            token = tokens.at(index);
            if(token->type == TokenType::T_RPAREN) {
              index++;
              return new LambdaAST(args, body);
            }
          }
        }
      } else if(symbol == "if") {
        AST* cond = parseExpression(tokens, index);
        AST* if_body = parseExpression(tokens, index);
        AST* else_body = parseExpression(tokens, index);
        token = tokens.at(index);
        if(token->type == TokenType::T_RPAREN) {
          index++;
          return new IfAST(cond, if_body, else_body);
        }
      } else {
        std::vector<AST*> args;
        while(true) {
          AST* arg = parseExpression(tokens, index);
          if(arg == nullptr) {
            break;
          }
          args.push_back(arg);
        }
        token = tokens.at(index);
        if(token->type == TokenType::T_RPAREN) {
          index++;
          return new FuncCallAST(new GetValueAST(symbol), args);
        }
      }
    }
  } else {
    return parseValue(tokens, index);
  }
  return nullptr;
}

AST* parse(std::vector<Token*> tokens) {
  int index = 0;
  std::vector<AST*> statements;
  while(index < tokens.size()) {
    AST* statement = parseExpression(tokens, index);
    if(statement == nullptr) {
      break;
    }
    statements.push_back(statement);
  }
  return new StatementsAST(statements);
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
    if(!find(name)) {
      std::cerr << "undefined value: " << name << std::endl;
      return new NullValue();
    }
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
  Value* apply(std::vector<Value*> args) {
    applied_args.insert(applied_args.end(), args.begin(), args.end());
    if(applied_args.size() == args_size) {
      return this->call(applied_args);
    } else if(applied_args.size() > args_size) {
      std::cerr << "too many args" << std::endl;
      return new NullValue();
    } else {
      return this;
    }
  }
  protected:
  virtual Value* call(std::vector<Value*> args) = 0;
  FunctionValue(ValueType type, int args_size) : Value(type), args_size(args_size) {}
};

Value* execute(AST* ast, Environment* env);

class DefinedFunctionValue : public FunctionValue {
  public:
  std::vector<std::string> arg_names;
  AST* body;
  Environment* env;
  DefinedFunctionValue(int args_size, std::vector<std::string> arg_names, AST* body, Environment* env) : FunctionValue(ValueType::V_DEFINED_FUNCTION, args_size), arg_names(arg_names), body(body), env(env) {}
  Value* call(std::vector<Value*> args) {
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
  std::function<Value*(std::vector<Value*>)> func;
  BuildInFunctionValue(std::function<Value*(std::vector<Value*>)> func, int args_size) : FunctionValue(ValueType::V_BUILD_IN_FUNCTION, args_size), func(func) {}
  Value* call(std::vector<Value*> args) {
    return func(args);
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
    return env->get(((GetValueAST*)ast)->name)->copy();
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
      return ((FunctionValue*)func)->apply(args);
    } else {
      std::cerr << "not a function: " << valueTypeToString(func->type) << std::endl;
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

Value* printFunction(std::vector<Value*> args) {
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

Value* concatFunction(std::vector<Value*> args) {
  auto value1 = args.at(0);
  auto value2 = args.at(1);
  if(value1->type == ValueType::V_STRING && value2->type == ValueType::V_STRING) {
    return new StringValue(((StringValue*)value1)->value + ((StringValue*)value2)->value);
  }
  std::cerr << "type error at concat: 1st arg is " << valueTypeToString(value1->type) << ", 2nd arg is " << valueTypeToString(value2->type) << std::endl;
  return new NullValue();
}

Value* getAtFunction(std::vector<Value*> args) {
  auto value1 = args.at(0);
  auto value2 = args.at(1);
  if(value1->type == ValueType::V_STRING && value2->type == ValueType::V_NUMBER) {
    auto value = ((StringValue*)value1)->value;
    auto index = ((NumberValue*)value2)->value;
    if(index >= 0 && index < value.size()) {
      return new StringValue(value.substr(index, 1));
    }
  }
  std::cerr << "type error at getAt: 1st arg is " << valueTypeToString(value1->type) << ", 2nd arg is " << valueTypeToString(value2->type) << std::endl;
  return new NullValue();
}

Value* addFunction(std::vector<Value*> args) {
  auto value1 = args.at(0);
  auto value2 = args.at(1);
  if(value1->type == ValueType::V_NUMBER && value2->type == ValueType::V_NUMBER) {
    return new NumberValue(((NumberValue*)value1)->value + ((NumberValue*)value2)->value);
  }
  std::cerr << "type error at add: 1st arg is " << valueTypeToString(value1->type) << ", 2nd arg is " << valueTypeToString(value2->type) << std::endl;
  return new NullValue();
}

Value* subFunction(std::vector<Value*> args) {
  auto value1 = args.at(0);
  auto value2 = args.at(1);
  if(value1->type == ValueType::V_NUMBER && value2->type == ValueType::V_NUMBER) {
    return new NumberValue(((NumberValue*)value1)->value - ((NumberValue*)value2)->value);
  }
  std::cerr << "type error at sub: 1st arg is " << valueTypeToString(value1->type) << ", 2nd arg is " << valueTypeToString(value2->type) << std::endl;
  return new NullValue();
}

Value* mulFunction(std::vector<Value*> args) {
  auto value1 = args.at(0);
  auto value2 = args.at(1);
  if(value1->type == ValueType::V_NUMBER && value2->type == ValueType::V_NUMBER) {
    return new NumberValue(((NumberValue*)value1)->value * ((NumberValue*)value2)->value);
  }
  std::cerr << "type error at mul: 1st arg is " << valueTypeToString(value1->type) << ", 2nd arg is " << valueTypeToString(value2->type) << std::endl;
  return new NullValue();
}

Value* divFunction(std::vector<Value*> args) {
  auto value1 = args.at(0);
  auto value2 = args.at(1);
  if(value1->type == ValueType::V_NUMBER && value2->type == ValueType::V_NUMBER) {
    return new NumberValue(((NumberValue*)value1)->value / ((NumberValue*)value2)->value);
  }
  std::cerr << "type error at div: 1st arg is " << valueTypeToString(value1->type) << ", 2nd arg is " << valueTypeToString(value2->type) << std::endl;
  return new NullValue();
}

Value* modFunction(std::vector<Value*> args) {
  auto value1 = args.at(0);
  auto value2 = args.at(1);
  if(value1->type == ValueType::V_NUMBER && value2->type == ValueType::V_NUMBER) {
    return new NumberValue(((NumberValue*)value1)->value % ((NumberValue*)value2)->value);
  }
  std::cerr << "type error at mod: 1st arg is " << valueTypeToString(value1->type) << ", 2nd arg is " << valueTypeToString(value2->type) << std::endl;
  return new NullValue();
}

Value* eqFunction(std::vector<Value*> args) {
  auto value1 = args.at(0);
  auto value2 = args.at(1);
  if(value1->type == ValueType::V_NUMBER && value2->type == ValueType::V_NUMBER) {
    return new NumberValue(((NumberValue*)value1)->value == ((NumberValue*)value2)->value);
  }
  // TODO string, null, bool
  std::cerr << "type error at eq: 1st arg is " << valueTypeToString(value1->type) << ", 2nd arg is " << valueTypeToString(value2->type) << std::endl;
  return new NullValue();
}

Value* ltFunction(std::vector<Value*> args) {
  auto value1 = args.at(0);
  auto value2 = args.at(1);
  if(value1->type == ValueType::V_NUMBER && value2->type == ValueType::V_NUMBER) {
    return new NumberValue(((NumberValue*)value1)->value < ((NumberValue*)value2)->value);
  }
  std::cerr << "type error at lt: 1st arg is " << valueTypeToString(value1->type) << ", 2nd arg is " << valueTypeToString(value2->type) << std::endl;
  return new NullValue();
}

Value* gtFunction(std::vector<Value*> args) {
  auto value1 = args.at(0);
  auto value2 = args.at(1);
  if(value1->type == ValueType::V_NUMBER && value2->type == ValueType::V_NUMBER) {
    return new NumberValue(((NumberValue*)value1)->value > ((NumberValue*)value2)->value);
  }
  std::cerr << "type error at gt: 1st arg is " << valueTypeToString(value1->type) << ", 2nd arg is " << valueTypeToString(value2->type) << std::endl;
  return new NullValue();
}

Value* notFunction(std::vector<Value*> args) {
  auto value = args.at(0);
  if(value->type == ValueType::V_NUMBER) {
    return new NumberValue(!((NumberValue*)value)->value);
  }
  std::cerr << "type error at not: arg is " << valueTypeToString(value->type) << std::endl;
  return new NullValue();
}

Value* andFunction(std::vector<Value*> args) {
  auto value1 = args.at(0);
  auto value2 = args.at(1);
  if(value1->type == ValueType::V_NUMBER && value2->type == ValueType::V_NUMBER) {
    return new NumberValue(((NumberValue*)value1)->value && ((NumberValue*)value2)->value);
  }
  std::cerr << "type error at and: 1st arg is " << valueTypeToString(value1->type) << ", 2nd arg is " << valueTypeToString(value2->type) << std::endl;
  return new NullValue();
}

Value* orFunction(std::vector<Value*> args) {
  auto value1 = args.at(0);
  auto value2 = args.at(1);
  if(value1->type == ValueType::V_NUMBER && value2->type == ValueType::V_NUMBER) {
    return new NumberValue(((NumberValue*)value1)->value || ((NumberValue*)value2)->value);
  }
  std::cerr << "type error at or: 1st arg is " << valueTypeToString(value1->type) << ", 2nd arg is " << valueTypeToString(value2->type) << std::endl;
  return new NullValue();
}

Value* toIntFunction(std::vector<Value*> args) {
  auto value = args.at(0);
  if(value->type == ValueType::V_STRING) {
    return new NumberValue(std::stoi(((StringValue*)value)->value));
  }
  std::cerr << "type error at toInt: arg is " << valueTypeToString(value->type) << std::endl;
  return new NullValue();
}

Value* toStringFunction(std::vector<Value*> args) {
  auto value = args.at(0);
  if(value->type == ValueType::V_NUMBER) {
    return new StringValue(std::to_string(((NumberValue*)value)->value));
  }
  std::cerr << "type error at toString: arg is " << valueTypeToString(value->type) << std::endl;
  return new NullValue();
}

Value* readFunction(std::vector<Value*> args) {
  std::string value;
  std::cin >> value;
  return new StringValue(value);
}

Environment* defaultEnvironment() {
  Environment* env = new Environment();
  env->set("print", new BuildInFunctionValue(printFunction, 1));
  env->set("concat", new BuildInFunctionValue(concatFunction, 2));
  env->set("getAt", new BuildInFunctionValue(getAtFunction, 2));
  env->set("add", new BuildInFunctionValue(addFunction, 2));
  env->set("sub", new BuildInFunctionValue(subFunction, 2));
  env->set("mul", new BuildInFunctionValue(mulFunction, 2));
  env->set("div", new BuildInFunctionValue(divFunction, 2));
  env->set("mod", new BuildInFunctionValue(modFunction, 2));
  env->set("eq", new BuildInFunctionValue(eqFunction, 2));
  env->set("lt", new BuildInFunctionValue(ltFunction, 2));
  env->set("gt", new BuildInFunctionValue(gtFunction, 2));
  env->set("not", new BuildInFunctionValue(notFunction, 1));
  env->set("and", new BuildInFunctionValue(andFunction, 2));
  env->set("or", new BuildInFunctionValue(orFunction, 2));
  env->set("toInt", new BuildInFunctionValue(toIntFunction, 1));
  env->set("toStr", new BuildInFunctionValue(toStringFunction, 1));
  env->set("read", new BuildInFunctionValue(readFunction, 0));
  return env;
}

void run(std::string input) {
  std::vector<Token*> tokens = tokenize(input);
  AST* ast = parse(tokens);
  Environment* env = defaultEnvironment();
  execute(ast, env);
}

