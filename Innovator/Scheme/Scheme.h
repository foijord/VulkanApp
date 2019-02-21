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

#define EXP_DECL(Class)                                 \
  Class() = delete;                                     \
  virtual ~Class() = default;                           \
  Class(Class&&) = delete;                              \
  Class(const Class&) = delete;                         \
  Class & operator=(Class&&) = delete;                  \
  Class & operator=(const Class&) = delete;             \

#define CALLABLE_DECL(Class)                            \
  Class() = default;                                    \
  virtual ~Class() = default;                           \
  Class(Class&&) = delete;                              \
  Class(const Class&) = delete;                         \
  Class & operator=(Class&&) = delete;                  \
  Class & operator=(const Class&) = delete;             \

class List;
class Expression;
class Environment;

typedef std::shared_ptr<List> lst_ptr;
typedef std::shared_ptr<Expression> exp_ptr;
typedef std::shared_ptr<Environment> env_ptr;

exp_ptr eval(exp_ptr & exp, env_ptr env);

typedef std::vector<exp_ptr> exp_vec;

class Expression : public std::enable_shared_from_this<Expression> {
public:
  CALLABLE_DECL(Expression)
  virtual std::string toString() const { return ""; }
  virtual exp_ptr eval(env_ptr) { return nullptr; }
};

class Callable : public Expression {
public:
  virtual exp_ptr operator()(const exp_vec & args) const = 0;
};

class List : public Expression {
public:
  CALLABLE_DECL(List)

  explicit List(exp_vec expressions)
    : expressions(expressions) {}

  explicit List(exp_vec::iterator begin, exp_vec::iterator end)
    : expressions(begin, end) {}

  exp_ptr car() 
  {
    return this->expressions.front();
  }

  lst_ptr cdr() 
  {
    return std::make_shared<List>(std::next(this->expressions.begin()), this->expressions.end());
  }

  virtual exp_ptr eval(env_ptr env);

  virtual std::string toString() const
  {
    std::string s("( ");
    for (auto child : this->expressions) {
      s += child->toString() + " ";
    }
    return s + ")";
  }

  exp_vec expressions;
};

class Symbol : public Expression {
public:
  EXP_DECL(Symbol)

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
  EXP_DECL(Environment)

  explicit Environment(std::initializer_list<value_type> il)
    : std::map<std::string, exp_ptr>(il)
  {}

  explicit Environment(iterator begin, iterator end)
    : std::map<std::string, exp_ptr>(begin, end)
  {}

  Environment(const exp_vec & parms, const exp_vec & args, env_ptr outer)
    : outer(std::move(outer))
  {
    for (auto p = parms.begin(), a = args.begin(); p != parms.end() && a != args.end(); ++p, ++a) {
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
List::eval(env_ptr env)
{ 
  auto list = std::make_shared<List>(this->expressions);
  std::transform(list->expressions.begin(), list->expressions.end(), list->expressions.begin(), 
    [&](exp_ptr exp) { return ::eval(exp, env); });
  return list;
}

inline exp_ptr
Symbol::eval(env_ptr env)
{ 
  return (*env->get_env(this->token))[this->token];
}

class Number : public Expression {
public:
  EXP_DECL(Number)

  explicit Number(const double value) 
    : value(value) {}

  explicit Number(const std::string & token)
    : Number(stod(token)) {}

  exp_ptr eval(env_ptr) override
  {
    return shared_from_this();
  }

  std::string toString() const override
  {
    return std::to_string(this->value);
  }

  double value;
};

class Boolean : public Expression {
public:
  EXP_DECL(Boolean)

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

  exp_ptr eval(env_ptr) override
  {
    return shared_from_this();
  }

  std::string toString() const override
  {
    return this->value ? "#t" : "#f";
  }
  
  bool value;
};

class String : public Expression {
public:
  EXP_DECL(String)

  explicit String(const std::string & token)
  {
    if (token.front() != '"' || token.back() != '"') {
      throw std::invalid_argument("invalid string literal: " + token);
    }
    this->value = token.substr(1, token.size() - 2);
  }

  exp_ptr eval(env_ptr) override
  {
    return shared_from_this();
  }

  std::string toString() const override
  {
    return this->value;
  }

  std::string value;
};


class Quote : public Expression {
public:
  EXP_DECL(Quote)

  explicit Quote(exp_ptr exp)
    : exp(exp) {}
  
  exp_ptr eval(env_ptr) override
  {
    return this->exp;
  }
  
  exp_ptr exp;
};

class Define : public Expression {
public:
  EXP_DECL(Define)

  explicit Define(std::shared_ptr<Symbol> var, exp_ptr exp) : 
    var(std::move(var)),
    exp(std::move(exp))
  {}
  
  exp_ptr eval(env_ptr env) override
  {
    auto exp = ::eval(this->exp, env);
    (*env)[this->var->token] = exp;
    return exp;
  }
  
  std::shared_ptr<Symbol> var;
  exp_ptr exp;
};

class If : public Expression {
public:
  EXP_DECL(If)

  explicit If(const std::vector<exp_ptr> & args) :
    test(args[1]),
    conseq(args[2]),
    alt(args[3]) 
  {}
  
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
  EXP_DECL(Function)

  Function(exp_vec parms, lst_ptr body, env_ptr env) : 
    parms(std::move(parms)), 
    body(std::move(body)), 
    env(std::move(env))
  {}
  
  exp_vec parms;
  lst_ptr body;
  env_ptr env;
};

class Lambda : public Expression {
public:
  EXP_DECL(Lambda)

  explicit Lambda(exp_vec parms, lst_ptr body) :
    parms(std::move(parms)),
    body(std::move(body))
  {}
  
  exp_ptr eval(env_ptr env) override
  {
    return std::make_shared<Function>(this->parms, this->body, env);
  }
  
  exp_vec parms;
  lst_ptr body;
};

static void
check_num_args(const exp_vec & args, size_t num)
{
  if (args.size() != num) {
    throw std::invalid_argument("invalid number of arguments");
  }
}

static void
check_num_args(const exp_vec & args)
{
  if (args.empty()) {
    throw std::invalid_argument("invalid number of arguments");
  }
}

template <typename T>
static std::vector<std::shared_ptr<T>> get_args(const exp_vec & args)
{
  std::vector<std::shared_ptr<T>> args_t(args.size());
  std::transform(args.begin(), args.end(), args_t.begin(), [](auto arg) {
    const auto arg_t = std::dynamic_pointer_cast<T>(arg);
    if (!arg_t) {
      throw std::invalid_argument("invalid argument type");
    }
    return arg_t;
  });
  return args_t;
}

template <typename T>
static std::vector<T> get_values(const exp_vec & args)
{
  auto numbers = get_args<Number>(args);
  std::vector<T> values(numbers.size());

  std::transform(numbers.begin(), numbers.end(), values.begin(), [](auto number) {
    return static_cast<T>(number->value);
  });
  return values;
}

template <typename T>
static std::vector<T> get_values(const exp_vec & args, size_t expected_size)
{
  auto argvec = get_values<T>(args);
  if (argvec.size() != expected_size) {
    throw std::logic_error("internal error: wrong number of arguments!");
  }
  return argvec;
}

template <typename T>
static std::shared_ptr<T> get_arg(const exp_vec & args)
{
  for (auto arg : args) {
    auto arg_t = std::dynamic_pointer_cast<T>(arg);
    if (arg_t) {
      return arg_t;
    }
  }
  throw std::invalid_argument("missing argument");
}

class ListFunction : public Callable {
public:
  CALLABLE_DECL(ListFunction)
  exp_ptr operator()(const exp_vec & args) const override
  {
    return std::make_shared<List>(args);
  }
};

class Length : public Callable {
public:
  CALLABLE_DECL(Length)
  exp_ptr operator()(const exp_vec & args) const override
  {
    check_num_args(args, 1);
    const auto list = std::dynamic_pointer_cast<List>(args.front());
    return std::make_shared<Number>(static_cast<double>(list->expressions.size()));
  }
};

template <typename T>
class Operator : public Callable {
public:
  CALLABLE_DECL(Operator)
  exp_ptr operator()(const exp_vec & args) const override
  {
    auto dargs = get_values<double>(args);
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
  CALLABLE_DECL(Less)
  exp_ptr operator()(const exp_vec & args) const override
  {
    auto dargs = get_values<double>(args, 2);
    return std::make_shared<Boolean>(dargs[0] < dargs[1]);
  }
};

class More : public Callable {
public:
  CALLABLE_DECL(More)
  exp_ptr operator()(const exp_vec & args) const override
  {
    auto dargs = get_values<double>(args, 2);
    return std::make_shared<Boolean>(dargs[0] > dargs[1]);
  }
};

class Same: public Callable {
public:
  CALLABLE_DECL(Same)
  exp_ptr operator()(const exp_vec & args) const override
  {
    auto dargs = get_values<double>(args, 2);
    return std::make_shared<Boolean>(dargs[0] == dargs[1]);
  }
};

class Car : public Callable {
public:
  CALLABLE_DECL(Car)
  exp_ptr operator()(const exp_vec & args) const override
  {
    check_num_args(args, 1);
    const auto list = std::dynamic_pointer_cast<List>(args.front());
    return list->car();
  }
};

class Cdr : public Callable {
public:
  CALLABLE_DECL(Cdr)
  exp_ptr operator()(const exp_vec & args) const override
  {
    check_num_args(args, 1);
    const auto list = std::dynamic_pointer_cast<List>(args.front());
    return list->cdr();
  }
};

inline exp_ptr eval(exp_ptr & exp, env_ptr env)
{
  while (true) {
    const auto e = exp.get();
    const auto & type = typeid(*e);

    if (type == typeid(If)) {
      exp = exp->eval(env);
      continue;
    }
    if (type != typeid(List)) {
      return exp->eval(env);
    }
    auto list = std::dynamic_pointer_cast<List>(exp->eval(env));

    auto front = list->car();
    auto rest = list->cdr()->expressions;

    if (typeid(*front.get()) == typeid(Function)) {
      auto func = std::dynamic_pointer_cast<Function>(front);
      exp = func->body;
      env = std::make_shared<Environment>(func->parms, rest, func->env);
    }
    else {
      auto car = std::dynamic_pointer_cast<Callable>(front);
      return (*car)(rest);
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
    if (exp_vec.size() != 2) {
      throw std::invalid_argument("wrong number of arguments to quote");
    }
    return std::make_shared<Quote>(exp_vec[1]);
  }
  if (token == "if") {
    if (exp_vec.size() != 4) {
      throw std::invalid_argument("wrong number of arguments to if");
    }
    return std::make_shared<If>(exp_vec);
  }
  if (token == "lambda") {
    if (exp_vec.size() != 3) {
      throw std::invalid_argument("wrong number of arguments to lambda");
    }
    auto args = std::dynamic_pointer_cast<List>(exp_vec[1]);
    if (!args) {
      throw std::invalid_argument("first parameter to lambda must be a list of arguments");
    }
    auto body = std::dynamic_pointer_cast<List>(exp_vec[2]);
    if (!body) {
      throw std::invalid_argument("second parameter to lambda must be an expression body");
    }
    return std::make_shared<Lambda>(args->expressions, body);
  }
  if (token == "define") {
    if (exp_vec.size() != 3) {
      throw std::invalid_argument("wrong number of arguments to define");
    }
    auto var = std::dynamic_pointer_cast<Symbol>(exp_vec[1]);
    if (!var) {
      throw std::invalid_argument("variable name must be a symbol");
    }
    return std::make_shared<Define>(var, exp_vec[2]);
  }
  if (token == "+" || token == "-" || token == "*" || token == "/") {
    if (parsetree.size() < 3) {
      throw std::invalid_argument("too few arguments to " + token);
    }
  } else if (token == "<" || token == ">" || token == "=") {
    if (parsetree.size() != 3) {
      throw std::invalid_argument("wrong number of arguments to " + token);
    }
  }

  return std::make_shared<List>(exp_vec);
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
  { "list", std::make_shared<ListFunction>() },
  { "length", std::make_shared<Length>() },
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
