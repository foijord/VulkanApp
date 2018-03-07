#pragma once

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

class Expression : public std::list<std::shared_ptr<Expression>> {
public:
  Expression() = default;
  Expression(const Expression&) = delete;
  Expression(const Expression&&) = delete;
  Expression & operator=(const Expression&) = delete;
  Expression & operator=(const Expression&&) = delete;

  virtual ~Expression() = default;

  explicit Expression(const const_iterator & begin, const const_iterator & end)
    : std::list<std::shared_ptr<Expression>>(begin, end) {}

  virtual std::shared_ptr<Expression> eval(Environment & env);

  virtual std::string string()
  {
    std::string s = "(";
    for (auto & it : *this) {
      s += it->string() + ' ';
    }
    s.back() = ')';
    return s;
  }
};

class Symbol : public Expression {
public:
  explicit Symbol(std::string token)
    : token(std::move(token))
  {}

  std::shared_ptr<Expression> eval(Environment & env) override;

  std::string string() override
  {
    return this->token;
  }

  std::string token;
};

class Environment : public std::map<std::string, std::shared_ptr<Expression>> {
public:
  Environment() : outer(nullptr) {}
  Environment(const Expression * parms,
              const Expression * args,
              Environment * outer)
    : outer(outer)
  {
    for (auto p = parms->begin(), a = args->begin(); p != parms->end() && a != args->end(); ++p, ++a) {
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
  auto exp = std::make_shared<Expression>();
  for (auto & it : *this) {
    exp->push_back(::eval(it, env));
  }
  return exp;
}

inline std::shared_ptr<Expression>
Symbol::eval(Environment & env)
{ 
  return (*env.get_env(this->token))[this->token];
}

class Number : public Expression {
public:
  explicit Number(const double value) 
    : value(value) 
  {}

  explicit Number(const std::string & token)
    : Number(stod(token)) {}
  
  std::string string() override
  {
    return std::to_string(this->value);
  }

  double value;
};

class Boolean : public Expression {
public:
  explicit Boolean(const bool value) 
    : value(value) 
  {}
  
  explicit Boolean(const std::string & token)
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
  
  std::string string() override
  {
    return this->value ? "#t" : "#f";
  }
  
  bool value;
};

class String : public Expression {
public:
  explicit String(const std::string & token)
  {
    if (token.front() != '"' || token.back() != '"') {
      throw std::invalid_argument("invalid string literal: " + token);
    }
    this->value = token.substr(1, token.size() - 2);
  }

  std::string string() override
  {
    return this->value;
  }

  std::string value;
};


class Quote : public Expression {
public:
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
  explicit Define(std::shared_ptr<Symbol> var, 
                  std::shared_ptr<Expression> exp)
    : var(std::move(var)), exp(std::move(exp))
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
  If(std::shared_ptr<Expression> test,
     std::shared_ptr<Expression> conseq,
     std::shared_ptr<Expression> alt)
    : test(std::move(test)), conseq(std::move(conseq)), alt(std::move(alt))
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
    std::vector<double> dargs(args->size());
    std::transform(args->begin(), args->end(), dargs.begin(), [](auto & arg) {
      return std::static_pointer_cast<Number>(arg)->value;
    });
    return dargs;
  }

  static void 
  check_num_args(const Expression * args, size_t num)
  {
    if (args->size() != num) {
      throw std::invalid_argument("invalid number of arguments");
    }
  }

  static const String * 
  get_string(const Expression * args, size_t n)
  {
    const auto it = std::next(args->begin(), n);
    const auto string = dynamic_cast<const String*>(&**it);
    if (!string) {
      throw std::invalid_argument("parameter must be a string");
    }
    return string;
  }

  static const Number * 
  get_number(const Expression * args, size_t n)
  {
    const auto it = std::next(args->begin(), n);
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
  std::shared_ptr<Expression> operator()(const Expression * args) const override
  {
    auto dargs = get_argvec(args);
    return std::make_shared<Boolean>(dargs[0] < dargs[1]);
  }
};

class More : public Callable {
public:
  std::shared_ptr<Expression> operator()(const Expression * args) const override
  {
    auto dargs = get_argvec(args);
    return std::make_shared<Boolean>(dargs[0] > dargs[1]);
  }
};

class Same: public Callable {
public:
  std::shared_ptr<Expression> operator()(const Expression * args) const override
  {
    auto dargs = get_argvec(args);
    return std::make_shared<Boolean>(dargs[0] == dargs[1]);
  }
};

class Car : public Callable {
public:
  std::shared_ptr<Expression> operator()(const Expression * args) const override
  {
    return args->front();
  }
};

class Cdr : public Callable {
public:
  std::shared_ptr<Expression> operator()(const Expression * args) const override
  {
    return std::make_shared<Expression>(next(args->begin()), args->end());
  }
};

inline std::shared_ptr<Expression> eval(std::shared_ptr<Expression> & exp, Environment env)
{
  while (true) {
    const auto & type = typeid(*exp.get());
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
    const auto front = exps->front();
    exps->pop_front();

    if (typeid(*front.get()) == typeid(Function)) {
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
    list->push_back(parse(pt));
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
