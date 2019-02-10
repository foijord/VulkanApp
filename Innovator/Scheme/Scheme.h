#pragma once

#include <Innovator/Misc/Defines.h>

#include <map>
#include <list>
#include <regex>
#include <vector>
#include <memory>
#include <sstream>
#include <numeric>
#include <iostream>
#include <typeinfo>
#include <algorithm>
#include <functional>

class Expression;
class Environment;

typedef std::shared_ptr<Expression> exp_ptr;
typedef std::shared_ptr<Environment> env_ptr;

exp_ptr eval(exp_ptr & exp, env_ptr env);

typedef std::list<exp_ptr> exp_list;
typedef std::vector<exp_ptr> exp_vec;

class Expression {
public:
  NO_COPY_OR_ASSIGNMENT(Expression)
  Expression() = default;
  virtual ~Expression() = default;

  explicit Expression(exp_list::const_iterator begin, exp_list::const_iterator end)
    : children(exp_list(begin, end)) {}

  explicit Expression(exp_vec::const_iterator begin, exp_vec::const_iterator end)
    : children(exp_list(begin, end)) {}

  virtual exp_ptr eval(env_ptr env);

  virtual std::string toString() const
  {
    std::string s("(");
    for (auto child : this->children) {
      s += child->toString();
    }
    return s + ")";
  }

  exp_list children;
};

class Symbol : public Expression {
public:
  NO_COPY_OR_ASSIGNMENT(Symbol)
  Symbol() = delete;
  virtual ~Symbol() = default;

  explicit Symbol(std::string token)
    : token(std::move(token)) {}

  exp_ptr eval(env_ptr env) override;
  
  std::string toString() const override
  {
    return this->token;
  }

  std::string token;
};

class Environment : public std::map<std::string, exp_ptr> {
public:
  Environment() = default;

  explicit Environment(std::initializer_list<value_type> il)
    : std::map<std::string, exp_ptr>(il)
  {}

  explicit Environment(iterator begin, iterator end)
    : std::map<std::string, exp_ptr>(begin, end)
  {}

  Environment(const Expression * parms, const Expression * args, env_ptr outer)
    : outer(std::move(outer))
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
  
  env_ptr outer { nullptr };
};

inline exp_ptr
Expression::eval(env_ptr env)
{ 
  auto result = std::make_shared<Expression>();
  for (auto& exp : this->children) {
    result->children.push_back(::eval(exp, env));
  }
  return result;
}

inline exp_ptr
Symbol::eval(env_ptr env)
{ 
  return (*env->get_env(this->token))[this->token];
}

class Number : public Expression {
public:
  NO_COPY_OR_ASSIGNMENT(Number)
  Number() = delete;
  virtual ~Number() = default;

  explicit Number(const double value) 
    : value(value) {}

  explicit Number(const std::string & token)
    : Number(stod(token)) {}

  std::string toString() const override
  {
    return std::to_string(this->value);
  }

  double value;
};

class Boolean : public Expression {
public:
  NO_COPY_OR_ASSIGNMENT(Boolean)
  Boolean() = delete;
  virtual ~Boolean() = default;

  explicit Boolean(const bool value) 
    : value(value) {}
  
  explicit Boolean(const std::string & token)
  {
    static std::map<std::string, bool> tokenmap = { { "#t", true }, { "#f", false } };
    if (tokenmap.find(token) == tokenmap.end()) {
      throw std::invalid_argument("expression is not boolean");
    }
    this->value = tokenmap[token];
  }

  std::string toString() const override
  {
    return this->value ? "#t" : "#f";
  }
  
  bool value;
};

class String : public Expression {
public:
  NO_COPY_OR_ASSIGNMENT(String)
  String() = delete;
  virtual ~String() = default;

  explicit String(const std::string & token)
  {
    if (token.front() != '"' || token.back() != '"') {
      throw std::invalid_argument("invalid string literal: " + token);
    }
    this->value = token.substr(1, token.size() - 2);
  }

  std::string toString() const override
  {
    return this->value;
  }

  std::string value;
};

class Quote : public Expression {
public:
  NO_COPY_OR_ASSIGNMENT(Quote)
  Quote() = delete;
  virtual ~Quote() = default;

  explicit Quote(const std::vector<exp_ptr> & args)
  {
    if (args.size() != 2) {
      throw std::invalid_argument("wrong number of arguments to quote");
    }
    this->exp = args[0];
  }
  
  exp_ptr eval(env_ptr) override
  {
    return this->exp;
  }
  
  exp_ptr exp;
};

class Define : public Expression {
public:
  NO_COPY_OR_ASSIGNMENT(Define)
  Define() = delete;
  virtual ~Define() = default;

  explicit Define(const std::vector<exp_ptr> & args)
  {
    if (args.size() != 3) {
      throw std::invalid_argument("wrong number of arguments to define");
    }
    this->var = std::dynamic_pointer_cast<Symbol>(args[1]);
    this->exp = args[2];

    if (!this->var) {
      throw std::invalid_argument("variable name must be a symbol");
    }
  }
  
  exp_ptr eval(env_ptr env) override
  {
    (*env)[this->var->token] = ::eval(this->exp, env);
    return std::make_shared<Expression>();
  }
  
  std::shared_ptr<Symbol> var;
  exp_ptr exp;
};

class If : public Expression {
public:
  NO_COPY_OR_ASSIGNMENT(If)
  If() = delete;
  virtual ~If() = default;

  explicit If(const std::vector<exp_ptr> & args)
  {
    if (args.size() != 4) {
      throw std::invalid_argument("wrong number of arguments to if");
    }

    this->test = args[1];
    this->conseq = args[2];
    this->alt = args[3];
  }
  
  exp_ptr eval(env_ptr env) override
  {
    const auto exp = ::eval(this->test, env);
    const auto boolexp = std::dynamic_pointer_cast<Boolean>(exp);
    if (!boolexp) {
      throw std::invalid_argument("expression did not evaluate to a boolean");
    }
    return (boolexp->value) ? this->conseq : this->alt;
  }
  
  exp_ptr test, conseq, alt;
};

class Function : public Expression {
public:
  NO_COPY_OR_ASSIGNMENT(Function)
  Function() = delete;
  virtual ~Function() = default;

  Function(exp_ptr parms, exp_ptr body, env_ptr env)
    : parms(std::move(parms)), body(std::move(body)), env(std::move(env)) 
  {}
  
  exp_ptr parms, body;
  env_ptr env;
};

class Lambda : public Expression {
public:
  NO_COPY_OR_ASSIGNMENT(Lambda)
  Lambda() = delete;
  virtual ~Lambda() = default;

  explicit Lambda(const std::vector<exp_ptr> & args)
  {
    if (args.size() != 3) {
      throw std::invalid_argument("wrong number of arguments to lambda");
    }
    this->parms = args[1];
    this->body = args[2];
  }
  
  exp_ptr eval(env_ptr env) override
  {
    return std::make_shared<Function>(this->parms, this->body, env);
  }
  
  exp_ptr parms, body;
};

class Callable : public Expression {
public:
  virtual exp_ptr operator()(const Expression * args) const = 0;

  static std::vector<std::shared_ptr<Number>> get_numbers(const Expression * args)
  {
    std::vector<std::shared_ptr<Number>> numbers(args->children.size());
    std::transform(args->children.begin(), args->children.end(), numbers.begin(), [](auto arg) {
      const auto number = std::dynamic_pointer_cast<Number>(arg);
      if (!number) {
        throw std::invalid_argument("parameter must be a number");
      }
      return number;
    });
    return numbers;
  }

  template <typename T>
  static std::vector<T> get_values(const Expression * args)
  {
    std::vector<std::shared_ptr<Number>> numbers = get_numbers(args);
    std::vector<T> values(numbers.size());

    std::transform(numbers.begin(), numbers.end(), values.begin(), [](auto number) {
      return static_cast<T>(number->value);
    });
    return values;
  }

  template <typename T>
  static std::vector<T> get_values(const Expression * args, size_t expected_size)
  {
    auto argvec = get_values<T>(args);
    if (argvec.size() != expected_size) {
      throw std::logic_error("internal error: wrong number of arguments!");
    }
    return argvec;
  }

  static void
  check_num_args(const Expression * args)
  {
    if (args->children.empty()) {
      throw std::invalid_argument("invalid number of arguments");
    }
  }

  static void 
  check_num_args(const Expression * args, size_t num)
  {
    if (args->children.size() != num) {
      throw std::invalid_argument("invalid number of arguments");
    }
  }

  static std::shared_ptr<String> 
  get_string(const Expression * args, size_t n)
  {
    const auto it = std::next(args->children.begin(), n);
    const auto string = std::dynamic_pointer_cast<String>(*it);
    if (!string) {
      throw std::invalid_argument("parameter must be a string");
    }
    return string;
  }

  static std::shared_ptr<Number>
  get_number(const Expression * args, size_t n)
  {
    const auto it = std::next(args->children.begin(), n);
    const auto number = std::dynamic_pointer_cast<Number>(*it);
    if (!number) {
      throw std::invalid_argument("parameter must be a number");
    }
    return number;
  }
};

template <typename T>
class Operator : public Callable {
public:
  NO_COPY_OR_ASSIGNMENT(Operator)
  Operator() = default;
  virtual ~Operator() = default;

  exp_ptr operator()(const Expression * args) const override
  {
    auto dargs = get_values<double>(args);
    if (dargs.empty()) {
      throw std::logic_error("logic error: no arguments to function call");
    }
    return std::make_shared<Number>(std::accumulate(next(dargs.begin()), dargs.end(), dargs.front(), this->op));
  }
  
  T op;
};

typedef Operator<std::plus<>> Add;
typedef Operator<std::minus<>> Sub;
typedef Operator<std::divides<>> Div;
typedef Operator<std::multiplies<>> Mul;

class Less : public Callable {
public:
  NO_COPY_OR_ASSIGNMENT(Less)
  Less() = default;
  virtual ~Less() = default;

  exp_ptr operator()(const Expression * args) const override
  {
    auto dargs = get_values<double>(args, 2);
    return std::make_shared<Boolean>(dargs[0] < dargs[1]);
  }
};

class More : public Callable {
public:
  NO_COPY_OR_ASSIGNMENT(More)
  More() = default;
  virtual ~More() = default;

  exp_ptr operator()(const Expression * args) const override
  {
    auto dargs = get_values<double>(args, 2);
    return std::make_shared<Boolean>(dargs[0] > dargs[1]);
  }
};

class Same: public Callable {
public:
  NO_COPY_OR_ASSIGNMENT(Same)
  Same() = default;
  virtual ~Same() = default;

  exp_ptr operator()(const Expression * args) const override
  {
    auto dargs = get_values<double>(args, 2);
    return std::make_shared<Boolean>(dargs[0] == dargs[1]);
  }
};

class Car : public Callable {
public:
  NO_COPY_OR_ASSIGNMENT(Car)
  Car() = default;
  virtual ~Car() = default;

  exp_ptr operator()(const Expression * args) const override
  {
    if (args->children.empty()) {
      throw std::logic_error("internal error: 'car' performed on empty list");
    }
    return args->children.front();
  }
};

class Cdr : public Callable {
public:
  NO_COPY_OR_ASSIGNMENT(Cdr)
  Cdr() = default;
  virtual ~Cdr() = default;

  exp_ptr operator()(const Expression * args) const override
  {
    if (args->children.empty()) {
      throw std::logic_error("internal error: 'cdr' performed on empty list");
    }
    return std::make_shared<Expression>(next(args->children.begin()), args->children.end());
  }
};

inline exp_ptr eval(exp_ptr & exp, env_ptr env)
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
      env = std::make_shared<Environment>(func->parms.get(), exps.get(), func->env);
    }
    else {
      const auto builtin = std::dynamic_pointer_cast<Callable>(front);
      return (*builtin)(exps.get());
    }
  }
}

class ParseTree : public std::vector<ParseTree> {
public:
  explicit ParseTree(std::istream_iterator<std::string> & it)
  {
    if (*it == "(") {
      while (*(++it) != ")") {
        // TODO: handle mismatched parentheses
        this->push_back(ParseTree(it));
      }
    }
    this->token = *it;
  }

  std::string token;
};

inline exp_ptr parse(const ParseTree & parsetree)
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
  
  exp_vec exp_vec(parsetree.size());
  std::transform(parsetree.begin(), parsetree.end(), exp_vec.begin(), parse);

  const std::string token = parsetree[0].token;

  if (token == "quote") {
    return std::make_shared<Quote>(exp_vec);
  }
  if (token == "if") {
    return std::make_shared<If>(exp_vec);
  }
  if (token == "lambda") {
    return std::make_shared<Lambda>(exp_vec);
  }
  if (token == "define") {
    return std::make_shared<Define>(exp_vec);
  }
  
  if (token == "+" || token == "-" || token == "*" || token == "/") {
    if (parsetree.size() < 3) {
      throw std::invalid_argument("too few arguments to " + token);
    }
  } else if (token == "<" || token == ">" || token == "=") {
    if (parsetree.size() != 3) {
      throw std::invalid_argument("wrong number of arguments to " + token);
    }
  } else if (token == "list") {
    if (parsetree.size() < 2) {
      throw std::invalid_argument("too few arguments to list");
    }
  }

  return std::make_shared<Expression>(exp_vec.begin(), exp_vec.end());
}

static Environment global_env{
  { "+",   std::make_shared<Add>() },
  { "-",   std::make_shared<Sub>() },
  { "/",   std::make_shared<Div>() },
  { "*",   std::make_shared<Mul>() },
  { "<",   std::make_shared<Less>() },
  { ">",   std::make_shared<More>() },
  { "=",   std::make_shared<Same>() },
  { "car", std::make_shared<Car>() },
  { "cdr", std::make_shared<Cdr>() },
  { "pi",  std::make_shared<Number>(PI) },
};

class Scheme {
public:
  Scheme()
    : environment(std::make_shared<Environment>(global_env.begin(), global_env.end()))
  {}

  exp_ptr eval(std::string input) const
  {
    input = std::regex_replace(input, std::regex(R"([(])"), " ( ");
    input = std::regex_replace(input, std::regex(R"([)])"), " ) ");

    std::istringstream iss(input);
    std::istream_iterator<std::string> tokens(iss);
    const ParseTree parsetree(tokens);
    auto exp = parse(parsetree);
    return ::eval(exp, this->environment);
  }

  env_ptr environment;
};
