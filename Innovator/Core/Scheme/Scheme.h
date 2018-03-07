#pragma once

#include <Innovator/Core/Misc/Defines.h>

#include <map>
#include <list>
#include <regex>
#include <vector>
#include <memory>
#include <numeric>
#include <typeinfo>
#include <algorithm>
#include <functional>

class Expression;
class Environment;

std::shared_ptr<class Expression> eval(std::shared_ptr<Expression> & exp, Environment env);

typedef std::list<std::shared_ptr<Expression>> list;

class Expression {
public:
  NO_COPY_OR_ASSIGNMENT(Expression);

  Expression() = default;

  explicit Expression(const list::const_iterator & begin, const list::const_iterator & end)
  {
    this->children = list(begin, end);
  }

  virtual std::shared_ptr<Expression> eval(Environment & env);

  virtual std::string string()
  {
    std::string s = "(";
    for (auto& it : this->children) {
      s += it->string() + ' ';
    }
    s.back() = ')';
    return s;
  }

  list children;
};

class Symbol : public Expression {
public:
  NO_COPY_OR_ASSIGNMENT(Symbol);

  explicit Symbol(std::string token)
    : token(std::move(token))
  {}

  std::shared_ptr<Expression> eval(Environment & env) override;

  std::string token;
};

class Environment : public std::map<std::string, std::shared_ptr<Expression>> {
public:
  Environment() : outer(nullptr) {}
  Environment(const Expression * parms, const Expression * args, Environment * outer)
    : outer(outer)
  {
    for (auto p = parms->children.begin(), a = args->children.begin(); p != parms->children.end() && a != args->children.end(); ++p, ++a) {
      const auto parm = std::dynamic_pointer_cast<Symbol>(*p);
      (*this)[parm->token] = *a;
    }
  }
  
  Environment * get_env(const std::string & sym)
  {
    if (this->find(sym) != this->end()) {
      return this;
    }
    if (!this->outer) {
      throw std::runtime_error("undefined symbol '" + sym + "'");
    }
    return this->outer->get_env(sym);
  }
  
  Environment * outer;
};

inline std::shared_ptr<Expression>
Expression::eval(Environment & env)
{ 
  auto result = std::make_shared<Expression>();
  for (auto& exp : this->children) {
    result->children.push_back(::eval(exp, env));
  }
  return result;
}

inline std::shared_ptr<Expression>
Symbol::eval(Environment & env)
{ 
  return (*env.get_env(this->token))[this->token];
}

class Number : public Expression {
public:
  NO_COPY_OR_ASSIGNMENT(Number);

  explicit Number(const double value) 
    : value(value) 
  {}

  explicit Number(const std::string & token)
    : Number(stod(token)) {}
  
  double value;
};

class Boolean : public Expression {
public:
  NO_COPY_OR_ASSIGNMENT(Boolean);

  explicit Boolean(const bool value) 
    : value(value) 
  {}
  
  explicit Boolean(const std::string & token)
  {
    static std::map<std::string, bool> tokenmap = { { "#t", true }, { "#f", false } };
    if (tokenmap.find(token) == tokenmap.end()) {
      throw std::invalid_argument("expression is not boolean");
    }
    this->value = tokenmap[token];
  }
  
  bool value;
};

class String : public Expression {
public:
  NO_COPY_OR_ASSIGNMENT(String);

  explicit String(const std::string & token)
  {
    if (token.front() != '"' || token.back() != '"') {
      throw std::invalid_argument("invalid string literal: " + token);
    }
    this->value = token.substr(1, token.size() - 2);
  }

  std::string value;
};

class Quote : public Expression {
public:
  NO_COPY_OR_ASSIGNMENT(Quote);

  explicit Quote(std::shared_ptr<Expression> exp)
    : exp(std::move(exp)) 
  {}
  
  std::shared_ptr<Expression> eval(Environment & env) override
  {
    return this->exp;
  }
  
  std::shared_ptr<Expression> exp;
};

class Define : public Expression {
public:
  NO_COPY_OR_ASSIGNMENT(Define);

  explicit Define(std::shared_ptr<Symbol> var, 
                  std::shared_ptr<Expression> exp)
    : var(std::move(var)), 
      exp(std::move(exp))
  {}
  
  std::shared_ptr<Expression> eval(Environment & env) override
  {
    env[this->var->token] = ::eval(this->exp, env);
    return std::make_shared<Expression>();
  }
  
  std::shared_ptr<Symbol> var;
  std::shared_ptr<Expression> exp;
};

class If : public Expression {
public:
  NO_COPY_OR_ASSIGNMENT(If);

  If(std::shared_ptr<Expression> test,
     std::shared_ptr<Expression> conseq,
     std::shared_ptr<Expression> alt)
    : test(std::move(test)), 
      conseq(std::move(conseq)), 
      alt(std::move(alt))
  {}
  
  std::shared_ptr<Expression> eval(Environment & env) override
  {
    const auto exp = ::eval(this->test, env);
    const auto boolexp = std::dynamic_pointer_cast<Boolean>(exp);
    if (!boolexp) {
      throw std::invalid_argument("expression did not evaluate to a boolean");
    }
    return (boolexp->value) ? this->conseq : this->alt;
  }
  
  std::shared_ptr<Expression> test, conseq, alt;
};

class Function : public Expression {
public:
  NO_COPY_OR_ASSIGNMENT(Function);

  Function(std::shared_ptr<Expression> parms,
           std::shared_ptr<Expression> body,
           Environment & env)
    : parms(std::move(parms)), body(std::move(body)), env(env) 
  {}
  
  std::shared_ptr<Expression> parms, body;
  Environment env;
};

class Lambda : public Expression {
public:
  NO_COPY_OR_ASSIGNMENT(Lambda);

  Lambda(std::shared_ptr<Expression> parms, 
         std::shared_ptr<Expression> body)
    : parms(std::move(parms)), body(std::move(body)) 
  {}
  
  std::shared_ptr<Expression> eval(Environment & env) override
  {
    return std::make_shared<Function>(this->parms, this->body, env);
  }
  
  std::shared_ptr<Expression> parms, body;
};

class Callable : public Expression {
public:
  virtual std::shared_ptr<Expression> operator()(const Expression * args) const = 0;
 
  static std::vector<double> 
  get_argvec(const Expression * args)
  {
    std::vector<double> dargs(args->children.size());
    std::transform(args->children.begin(), args->children.end(), dargs.begin(), [](auto & arg) {
      return std::static_pointer_cast<Number>(arg)->value;
    });
    return dargs;
  }

  static void 
  check_num_args(const Expression * args, size_t num)
  {
    if (args->children.size() != num) {
      throw std::invalid_argument("invalid number of arguments");
    }
  }

  static const String * 
  get_string(const Expression * args, size_t n)
  {
    const auto it = std::next(args->children.begin(), n);
    const auto string = dynamic_cast<const String*>(&**it);
    if (!string) {
      throw std::invalid_argument("parameter must be a string");
    }
    return string;
  }

  static const Number * 
  get_number(const Expression * args, size_t n)
  {
    const auto it = std::next(args->children.begin(), n);
    const auto number = dynamic_cast<const Number*>(&**it);
    if (!number) {
      throw std::invalid_argument("parameter must be a number");
    }
    return number;
  }
};

template <typename T>
class Operator : public Callable {
public:
  NO_COPY_OR_ASSIGNMENT(Operator);

  Operator() = default;

  std::shared_ptr<Expression> operator()(const Expression * args) const override
  {
    auto dargs = get_argvec(args);
    return std::make_shared<Number>(std::accumulate(next(dargs.begin()), dargs.end(), dargs[0], op));
  }
  
  T op;
};

typedef Operator<std::plus<>> Add;
typedef Operator<std::minus<>> Sub;
typedef Operator<std::divides<>> Div;
typedef Operator<std::multiplies<>> Mul;

class Less : public Callable {
public:
  NO_COPY_OR_ASSIGNMENT(Less);

  Less() = default;

  std::shared_ptr<Expression> operator()(const Expression * args) const override
  {
    auto dargs = get_argvec(args);
    return std::make_shared<Boolean>(dargs[0] < dargs[1]);
  }
};

class More : public Callable {
public:
  NO_COPY_OR_ASSIGNMENT(More);

  More() = default;

  std::shared_ptr<Expression> operator()(const Expression * args) const override
  {
    auto dargs = get_argvec(args);
    return std::make_shared<Boolean>(dargs[0] > dargs[1]);
  }
};

class Same: public Callable {
public:
  NO_COPY_OR_ASSIGNMENT(Same);

  Same() = default;

  std::shared_ptr<Expression> operator()(const Expression * args) const override
  {
    auto dargs = get_argvec(args);
    return std::make_shared<Boolean>(dargs[0] == dargs[1]);
  }
};

class Car : public Callable {
public:
  NO_COPY_OR_ASSIGNMENT(Car);

  Car() = default;

  std::shared_ptr<Expression> operator()(const Expression * args) const override
  {
    return args->children.front();
  }
};

class Cdr : public Callable {
public:
  NO_COPY_OR_ASSIGNMENT(Cdr);

  Cdr() = default;

  std::shared_ptr<Expression> operator()(const Expression * args) const override
  {
    return std::make_shared<Expression>(next(args->children.begin()), args->children.end());
  }
};

inline std::shared_ptr<Expression> eval(std::shared_ptr<Expression> & exp, Environment env)
{
  while (true) {
    const auto e = exp.get();
    const auto & type = typeid(*e);

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
    const auto front = exps->children.front();
    exps->children.pop_front();

    const auto f = front.get();
    if (typeid(*f) == typeid(Function)) {
      auto func = std::dynamic_pointer_cast<Function>(front);
      exp = func->body;
      env = Environment(func->parms.get(), exps.get(), &func->env);
    }
    else {
      const auto builtin = std::dynamic_pointer_cast<Callable>(front);
      return (*builtin)(exps.get());
    }
  }
}

class ParseTree : public std::vector<ParseTree> {
public:
  explicit ParseTree(std::sregex_token_iterator & it)
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

inline std::shared_ptr<Expression> parse(const ParseTree & parsetree)
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
  
  if (parsetree[0].token == "+") {
    if (parsetree.size() != 3) {
      throw std::invalid_argument("+ needs at least two arguments <");
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
    list->children.push_back(parse(pt));
  }
  
  return list;
}

class Scheme {
public:
  Scheme()
  {
    this->tokenizer = std::regex(R"([()]|"([^\"]|\.)*"|[a-zA-Z_-]+|[0-9]+|[+*-/<>=])");

    this->environment["+"] = std::make_shared<Add>();
    this->environment["-"] = std::make_shared<Sub>();
    this->environment["/"] = std::make_shared<Div>();
    this->environment["*"] = std::make_shared<Mul>();
    this->environment["<"] = std::make_shared<Less>();
    this->environment[">"] = std::make_shared<More>();
    this->environment["="] = std::make_shared<Same>();
    this->environment["car"] = std::make_shared<Car>();
    this->environment["cdr"] = std::make_shared<Cdr>();
    this->environment["pi"] = std::make_shared<Number>(3.14159265358979323846);
  }

  std::shared_ptr<Expression> eval(const std::string & input) const
  {
    std::sregex_token_iterator tokens(input.begin(), input.end(), this->tokenizer);
    const ParseTree parsetree(tokens);
    auto exp = parse(parsetree);
    return ::eval(exp, this->environment);
  }

  std::regex tokenizer;
  Environment environment;
};
