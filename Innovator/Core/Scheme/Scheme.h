#pragma once

#include <map>
#include <list>
#include <regex>
#include <vector>
#include <memory>
#include <numeric>
#include <typeinfo>
#include <iostream>
#include <algorithm>
#include <functional>

class Expression;
class Environment;

std::shared_ptr<class Expression> eval(std::shared_ptr<Expression> & exp,
                                       std::shared_ptr<Environment> & env);

class Expression : public std::list<std::shared_ptr<Expression>> {
public:
  Expression();
  Expression(Expression::iterator begin, Expression::iterator end);
  virtual ~Expression();

  virtual std::shared_ptr<Expression> eval(std::shared_ptr<Environment> & env);
  virtual std::string toString();

};

class Environment : public std::map<std::string, std::shared_ptr<Expression>> {
public:
  Environment();
  Environment(const std::shared_ptr<Expression> & parms,
              const std::shared_ptr<Expression> & args,
              const std::shared_ptr<Environment> & outer = std::make_shared<Environment>());
  
  Environment * getEnv(const std::string & sym);
  
  std::shared_ptr<Environment> outer;
};

class Symbol : public Expression {
public:
  Symbol(const std::string & token);
  
  virtual std::shared_ptr<Expression> eval(std::shared_ptr<Environment> & env);
  virtual std::string toString();
  
  std::string token;
};


Environment::Environment() {}
Environment::Environment(const std::shared_ptr<Expression> & parms,
                         const std::shared_ptr<Expression> & args,
                         const std::shared_ptr<Environment> & outer)
  : outer(outer)
{
  for (auto p = parms->begin(), a = args->begin(); p != parms->end() && a != args->end(); ++p, ++a) {
    auto parm = std::dynamic_pointer_cast<Symbol>(*p);
    (*this)[parm->token] = *a;
  }
}
  
Environment *
Environment::getEnv(const std::string & sym)
{
  if (this->find(sym) != this->end()) {
    return this;
  }
  if (!this->outer) {
    throw std::runtime_error("undefined symbol '" + sym + "'");
  }
  return this->outer->getEnv(sym);
}

Expression::Expression() {}

Expression::~Expression() {}

Expression::Expression(Expression::iterator begin, Expression::iterator end)
  : std::list<std::shared_ptr<Expression>>(begin, end) {}
  
std::string
Expression::toString()
{
  std::string s = "(";
  for (auto it = this->begin(); it != this->end(); it++) {
    s += (*it)->toString() + ' ';
  }
  s.back() = ')';
  return s;
}
  
std::shared_ptr<Expression>
Expression::eval(std::shared_ptr<Environment> & env)
{ 
  auto exp = std::make_shared<Expression>();
  for (auto it = this->begin(); it != this->end(); ++it) {
    exp->push_back(::eval(*it, env));
  }
  return exp;
}
  
Symbol::Symbol(const std::string & token)
  : token(token) 
{}
  
std::string
Symbol::toString()
{
  return this->token;
}
  
std::shared_ptr<Expression>
Symbol::eval(std::shared_ptr<Environment> & env)
{ 
  return (*env->getEnv(this->token))[this->token];
}

class Number : public Expression {
public:
  Number(double value) 
    : value(value) 
  {}

  Number(const std::string & token)
    : Number(stod(token)) {}
  
  virtual std::string toString()
  {
    return std::to_string(this->value);
  }

  double value;
};

class Boolean : public Expression {
public:
  Boolean(bool value) 
    : value(value) 
  {}
  
  Boolean(const std::string & token)
  {
    if (token == "#t") {
      this->value = true;
    }
    else if (token == "#f") {
      this->value = false;
    }
    else {
      throw std::invalid_argument("expression is not boolean");
    }
  }
  
  virtual std::string string()
  {
    return this->value ? "#t" : "#f";
  }
  
  bool value;
};

class String : public Expression {
public:
  String(const std::string & token)
  {
    if (token.front() != '"' || token.back() != '"') {
      throw std::invalid_argument("invalid string literal: " + token);
    }
    this->value = token.substr(1, token.size() - 2);
  }

  virtual std::string string()
  {
    return this->value;
  }

  std::string value;
};


class Quote : public Expression {
public:
  Quote(const std::shared_ptr<Expression> & exp)
    : exp(exp) 
  {}
  
  virtual std::shared_ptr<Expression> eval(std::shared_ptr<Environment> & env)
  {
    return this->exp;
  }
  
  std::shared_ptr<Expression> exp;
};

class Define : public Expression {
public:
  Define(const std::shared_ptr<Symbol> & var, 
         const std::shared_ptr<Expression> & exp)
    : var(var), exp(exp)
  {}
  
  virtual std::shared_ptr<Expression> eval(std::shared_ptr<Environment> & env)
  {
    (*env)[this->var->token] = ::eval(this->exp, env);
    return std::make_shared<Expression>();
  }
  
  std::shared_ptr<Symbol> var;
  std::shared_ptr<Expression> exp;
};

class If : public Expression {
public:
  If(const std::shared_ptr<Expression> & test,
     const std::shared_ptr<Expression> & conseq,
     const std::shared_ptr<Expression> & alt)
    : test(test), conseq(conseq), alt(alt)
  {}
  
  virtual std::shared_ptr<Expression> eval(std::shared_ptr<Environment> & env)
  {
    return (std::static_pointer_cast<Boolean>(::eval(this->test, env))->value) ? this->conseq : this->alt;
  }
  
  std::shared_ptr<Expression> test, conseq, alt;
};

class Function : public Expression {
public:
  Function(const std::shared_ptr<Expression> & parms,
           const std::shared_ptr<Expression> & body,
           const std::shared_ptr<Environment> & env)
    : parms(parms), body(body), env(env) 
  {}
  
  std::shared_ptr<Environment> env;
  std::shared_ptr<Expression> parms, body;
};

class Lambda : public Expression {
public:
  Lambda(const std::shared_ptr<Expression> & parms, 
         const std::shared_ptr<Expression> & body)
    : parms(parms), body(body) 
  {}
  
  virtual std::shared_ptr<Expression> eval(std::shared_ptr<Environment> & env)
  {
    return std::make_shared<Function>(this->parms, this->body, env);
  }
  
  std::shared_ptr<Expression> parms, body;
};

class Callable : public Expression {
public:
  virtual std::shared_ptr<Expression> operator()(const std::shared_ptr<Expression> & args) = 0;
 
  static std::vector<double> argvec(const std::shared_ptr<Expression> & args)
  {
    std::vector<double> dargs(args->size());
    std::transform(args->begin(), args->end(), dargs.begin(), [](auto & arg) {
      return std::static_pointer_cast<Number>(arg)->value;
    });
    return dargs;
  }

  void checkNumArgs(const std::shared_ptr<Expression> & args, size_t num)
  {
    if (args->size() != num) {
      throw std::invalid_argument("invalid number of arguments");
    }
  }

  std::shared_ptr<String> getString(const std::shared_ptr<Expression> & arg)
  {
    auto string = std::dynamic_pointer_cast<String>(arg);
    if (!string) {
      throw std::invalid_argument("parameter must be a string");
    }
    return string;
  }

  std::shared_ptr<Number> getNumber(const std::shared_ptr<Expression> & arg)
  {
    auto number = std::dynamic_pointer_cast<Number>(arg);
    if (!number) {
      throw std::invalid_argument("parameter must be a string");
    }
    return number;
  }

};

template <typename T>
class Operator : public Callable {
public:
  virtual std::shared_ptr<Expression> operator()(const std::shared_ptr<Expression> & args)
  {
    std::vector<double> dargs = Callable::argvec(args);
    return std::make_shared<Number>(std::accumulate(next(dargs.begin()), dargs.end(), dargs[0], op));
  }
  
  T op;
};

typedef Operator<std::plus<double>> Add;
typedef Operator<std::minus<double>> Sub;
typedef Operator<std::divides<double>> Div;
typedef Operator<std::multiplies<double>> Mul;

class Less : public Callable {
public:
  virtual std::shared_ptr<Expression> operator()(const std::shared_ptr<Expression> & args)
  {
    std::vector<double> dargs = Callable::argvec(args);
    return std::make_shared<Boolean>(dargs[0] < dargs[1]);
  }
};

class More : public Callable {
public:
  virtual std::shared_ptr<Expression> operator()(const std::shared_ptr<Expression> & args)
  {
    std::vector<double> dargs = Callable::argvec(args);
    return std::make_shared<Boolean>(dargs[0] > dargs[1]);
  }
};

class Same: public Callable {
public:
  virtual std::shared_ptr<Expression> operator()(const std::shared_ptr<Expression> & args)
  {
    std::vector<double> dargs = Callable::argvec(args);
    return std::make_shared<Boolean>(dargs[0] == dargs[1]);
  }
};

class Car : public Callable {
public:
  virtual std::shared_ptr<Expression> operator()(const std::shared_ptr<Expression> & args)
  {
    return args->front();
  }
};

class Cdr : public Callable {
public:
  virtual std::shared_ptr<Expression> operator()(const std::shared_ptr<Expression> & args)
  {
    return std::make_shared<Expression>(next(args->begin()), args->end());
  }
};

std::shared_ptr<Expression> eval(std::shared_ptr<Expression> & exp, 
                                 std::shared_ptr<Environment> & env)
{
  while (true) {
    const type_info &type = typeid(*exp.get());
    if (type == typeid(Number) ||
        type == typeid(String) ||
        type == typeid(Boolean)) {
      return exp;
    }
    if (type == typeid(Quote) ||
        type == typeid(Symbol) ||
        type == typeid(Define) ||
        type == typeid(Lambda)) {
      return exp->eval(env);
    }
    if (type == typeid(If)) {
      exp = exp->eval(env);
      continue;
    }

    auto exps = exp->eval(env);
    auto front = exps->front();
    exps->pop_front();

    if (typeid(*front.get()) == typeid(Function)) {
      auto func = std::dynamic_pointer_cast<Function>(front);
      exp = func->body;
      env = std::make_shared<Environment>(func->parms, exps, func->env);
    }
    else {
      auto builtin = std::dynamic_pointer_cast<Callable>(front);
      return (*builtin)(exps);
    }
  }
}

class ParseTree : public std::vector<ParseTree> {
public:
  ParseTree(std::sregex_token_iterator & it)
  {
    if (it->str() == "(") {
      while (*(++it) != ")") {
        this->push_back(ParseTree(it));
      }
    }
    this->token = *it;
  }
  std::string token;
};

std::shared_ptr<Expression> parse(const ParseTree & parsetree)
{
  if (parsetree.empty()) {
    try {
      return std::make_shared<Number>(parsetree.token);
    }
    catch (const std::invalid_argument &) {
      try {
        return std::make_shared<String>(parsetree.token);
      }
      catch (const std::invalid_argument &) {
        try {
          return std::make_shared<Boolean>(parsetree.token);
        }
        catch (const std::invalid_argument &) {
          return std::make_shared<Symbol>(parsetree.token);
        }
      }
    }
  }
  
  if (parsetree[0].token == "quote") {
    if (parsetree.size() != 2) {
      throw std::invalid_argument("invalid num args to quote");
    }
    return std::make_shared<Quote>(parse(parsetree[1]));
  }
  
  if (parsetree[0].token == "if") {
    if (parsetree.size() != 4) {
      throw std::invalid_argument("invalid num args to if");
    }
    return std::make_shared<If>(parse(parsetree[1]), parse(parsetree[2]), parse(parsetree[3]));
  }
  
  if (parsetree[0].token == "lambda") {
    if (parsetree.size() != 3) {
      throw std::invalid_argument("invalid num arguments to lambda");
    }
    return std::make_shared<Lambda>(parse(parsetree[1]), parse(parsetree[2]));
  }
  
  if (parsetree[0].token == "define") {
    if (parsetree.size() != 3) {
      throw std::invalid_argument("invalid num arguments to define");
    }
    auto var = std::dynamic_pointer_cast<Symbol>(parse(parsetree[1]));
    if (!var) {
      throw std::invalid_argument("variable name must be a symbol");
    }
    auto exp = parse(parsetree[2]);
    return std::make_shared<Define>(var, exp);
  }
  
  if (parsetree[0].token == "list") {
    if (parsetree.size() < 2) {
      throw std::invalid_argument("too few arguments to function list");
    }
  }
  
  if (parsetree[0].token == "<") {
    if (parsetree.size() != 3) {
      throw std::invalid_argument("wrong number of arguments to <");
    }
  }
  
  if (parsetree[0].token == ">") {
    if (parsetree.size() != 3) {
      throw std::invalid_argument("wrong number of arguments to >");
    }
  }
  
  if (parsetree[0].token == "=") {
    if (parsetree.size() != 3) {
      throw std::invalid_argument("wrong number of arguments to =");
    }
  }

  auto list = std::make_shared<Expression>();
  for (const ParseTree & pt : parsetree) {
    list->push_back(parse(pt));
  }
  
  return list;
}

class Scheme {
public:
  Scheme()
  {
    this->tokenizer = std::regex("[()]|\"([^\\\"]|\\.)*\"|[a-zA-Z_]+|[0-9]+|[+*-/<>=]");
    this->environment = std::make_shared<Environment>();

    (*this->environment)["+"] = std::make_shared<Add>();
    (*this->environment)["-"] = std::make_shared<Sub>();
    (*this->environment)["/"] = std::make_shared<Div>();
    (*this->environment)["*"] = std::make_shared<Mul>();
    (*this->environment)["<"] = std::make_shared<Less>();
    (*this->environment)[">"] = std::make_shared<More>();
    (*this->environment)["="] = std::make_shared<Same>();
    (*this->environment)["car"] = std::make_shared<Car>();
    (*this->environment)["cdr"] = std::make_shared<Cdr>();
    (*this->environment)["pi"] = std::make_shared<Number>(3.14159265358979323846);
  }

  ~Scheme() {}

  std::shared_ptr<Expression> eval(const std::string & input)
  {
    std::sregex_token_iterator tokens(input.begin(), input.end(), this->tokenizer);
    ParseTree parsetree(tokens);
    auto exp = parse(parsetree);
    std::string s = exp->toString();
    return ::eval(exp, this->environment);
  }

  std::regex tokenizer;
  std::shared_ptr<Environment> environment;
};
