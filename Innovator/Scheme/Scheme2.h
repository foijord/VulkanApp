#pragma once

#include <any>
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
#include <unordered_map>

typedef std::vector<std::any> List;
typedef std::shared_ptr<List> lst_ptr;
typedef std::function<std::any(const List & args)> fun_ptr;

typedef bool Boolean;
typedef double Number;
typedef std::string String;

struct Symbol : public std::string {
  explicit Symbol(const String & s) : std::string(s) {}
};

class Env;
typedef std::shared_ptr<Env> env_ptr;

struct Quote {
  std::any exp;
};

struct Define {
  Symbol sym;
  std::any exp;
};

struct Lambda {
  std::any parm, body;
};

struct Function {
  std::any parms, exp;
  env_ptr env;
};

struct If {
  std::any test, conseq, alt;
};

template <typename T>
std::vector<T> cast(const List & lst)
{
  std::vector<T> args(lst.size());
  std::transform(lst.begin(), lst.end(), args.begin(),
    [](std::any exp) { return std::any_cast<T>(exp); });
  return args;
}

fun_ptr plus = [](const List & lst)
{
  std::vector<Number> args = cast<Number>(lst);
  return std::accumulate(next(args.begin()), args.end(), args.front(), std::plus<Number>());
};

fun_ptr minus = [](const List & lst)
{
  std::vector<Number> args = cast<Number>(lst);
  return std::accumulate(next(args.begin()), args.end(), args.front(), std::minus<Number>());
};

fun_ptr divides = [](const List & lst)
{
  std::vector<Number> args = cast<Number>(lst);
  return std::accumulate(next(args.begin()), args.end(), args.front(), std::divides<Number>());
};

fun_ptr multiplies = [](const List & lst)
{
  std::vector<Number> args = cast<Number>(lst);
  return std::accumulate(next(args.begin()), args.end(), args.front(), std::multiplies<Number>());
};

fun_ptr greater = [](const List & lst)
{
  std::vector<Number> args = cast<Number>(lst);
  return Boolean(args[0] > args[1]);
};

fun_ptr less = [](const List & lst)
{
  std::vector<Number> args = cast<Number>(lst);
  return Boolean(args[0] < args[1]);
};

fun_ptr equal = [](const List & lst)
{
  std::vector<Number> args = cast<Number>(lst);
  return Boolean(args[0] == args[1]);
};

fun_ptr car = [](const List & lst)
{
  auto l = std::any_cast<lst_ptr>(lst.front());
  return l->front();
};

fun_ptr cdr = [](const List & lst)
{
  auto l = std::any_cast<lst_ptr>(lst.front());
  return std::make_shared<List>(next(l->begin()), l->end());
};

fun_ptr list = [](const List & lst)
{
  return std::make_shared<List>(lst.begin(), lst.end());
};

fun_ptr length = [](const List & lst)
{
  auto l = std::any_cast<lst_ptr>(lst.front());
  return static_cast<Number>(l->size());
};

class Env {
public:
  Env() = default;
  ~Env() = default;

  explicit Env(const std::any & parm, const List & args, env_ptr outer)
    : outer(std::move(outer))
  {
    if (parm.type() == typeid(lst_ptr)) {
      auto parms = std::any_cast<lst_ptr>(parm);
      for (size_t i = 0; i < parms->size(); i++) {
        auto sym = std::any_cast<Symbol>((*parms)[i]);
        this->inner[sym] = args[i];
      }
    } else {
      auto sym = std::any_cast<Symbol>(parm);
      this->inner[sym] = args;
    }
  }

  std::any get(Symbol sym)
  {
    if (this->inner.find(sym) != this->inner.end()) {
      return this->inner.at(sym);
    }
    if (this->outer) {
      return this->outer->get(sym);
    }
    throw std::runtime_error("undefined symbol: " + sym);
  }
    
  std::unordered_map<std::string, std::any> inner;
  env_ptr outer { nullptr };
};

std::string to_string(std::any exp, env_ptr env) 
{
  if (exp.type() == typeid(Number)) {
    return std::to_string(std::any_cast<Number>(exp));
  }
  if (exp.type() == typeid(Symbol)) {
    auto symbol = std::any_cast<Symbol>(exp);
    auto value = env->get(symbol);
    return symbol + ": " + to_string(value, env);
  }
  if (exp.type() == typeid(Boolean)) {
    return std::any_cast<Boolean>(exp) ? "#t" : "#f";
  }
  if (exp.type() == typeid(lst_ptr)) {
    auto list = std::any_cast<lst_ptr>(exp);

    std::string result("(");
    for (auto s : *list) {
      result += to_string(s, env) + " ";
    }
    result.pop_back();
    return result + ")";
  }
  return "()";
}

std::any eval(std::any & exp, env_ptr & env)
{
  while (true) {
    if (exp.type() == typeid(Number)) {
      return exp;      
    }
    else if (exp.type() == typeid(Boolean)) {
      return exp;
    } 
    else if (exp.type() == typeid(Symbol)) {
      auto symbol = std::any_cast<Symbol>(exp);
      return env->get(symbol);
    }
    else if (exp.type() == typeid(Quote)) {
      auto quote = std::any_cast<Quote>(exp);
      return quote.exp;
    }
    else if (exp.type() == typeid(Define)) {
      auto define = std::any_cast<Define>(exp);
      env->inner[define.sym] = eval(define.exp, env);
      return nullptr;
    } 
    else if (exp.type() == typeid(Lambda)) {
      auto lambda = std::any_cast<Lambda>(exp);
      return Function{ lambda.parm, lambda.body, env };
    }
    else if (exp.type() == typeid(If)) {
      auto iff = std::any_cast<If>(exp);
      exp = std::any_cast<Boolean>(eval(iff.test, env)) ? iff.conseq : iff.alt;
    }
    else {
      const auto & list = std::any_cast<lst_ptr>(exp);
      const auto & fun = eval(list->front(), env);

      auto args = List(list->size() - 1);
      std::transform(next(list->begin()), list->end(), args.begin(), [&](std::any exp) { 
        return eval(exp, env);
      });

      if (fun.type() == typeid(Function)) {
        auto f = std::any_cast<Function>(fun);
        exp = f.exp;
        env = std::make_shared<Env>(f.parms, args, f.env);
      } else {
        auto f = std::any_cast<fun_ptr>(fun);
        return f(args);
      }
    }
  }
}

class expression : public std::vector<expression> {
public:
  explicit expression(std::istream_iterator<std::string> & it)
  {
    if (*it == "(") {
      this->token = *(++it);
      while (*(++it) != ")") {
        this->push_back(expression(it));
      }
    } else {
      this->token = *it;
    }
  }

  std::string token;
};

inline std::any atom(const std::string & token)
{
  if (token == "#t") {
    return Boolean(true);
  }
  if (token == "#f") {
    return Boolean(false);
  }
  std::regex match_number(R"(^[+-]?([0-9]*[.])?[0-9]+$)");
  if (std::regex_match(token, match_number)) {
    return Number(std::stod(token));
  }
  return Symbol(token);
}

inline std::any parse(const expression & exp)
{
  if (exp.empty()) {
    return atom(exp.token);
  }
  
  List args(exp.size());
  std::transform(exp.begin(), exp.end(), args.begin(), parse);

  if (exp.token == "quote") {
    if (args.size() != 1) {
      throw std::invalid_argument("wrong number of arguments to quote");
    }
    return Quote{ args[0] };
  }

  if (exp.token == "if") {
    if (args.size() != 3) {
      throw std::invalid_argument("wrong number of arguments to if");
    }
    return If{ args[0], args[1], args[2] };
  }

  if (exp.token == "lambda") {
    if (args.size() != 2) {
      throw std::invalid_argument("wrong Number of arguments to lambda");
    }
    return Lambda{ args[0], args[1] };
  }

  if (exp.token == "define") {
    if (args.size() != 2) {
      throw std::invalid_argument("wrong number of arguments to Define");
    }
    if (args[0].type() != typeid(Symbol)) {
      throw std::invalid_argument("first argument to Define must be a Symbol");
    }
    return Define{ std::any_cast<Symbol>(args[0]), args[1] };
  }

  List list{ Symbol(exp.token) };
  list.insert(list.end(), args.begin(), args.end());
  return std::make_shared<List>(list);
}

class Scheme {
public:
  Scheme()
    : env(std::make_shared<Env>())
  {
    env->inner = {
      { Symbol("pi"),  Number(3.14159265358979323846) },
      { Symbol("+"), plus },
      { Symbol("-"), minus },
      { Symbol("/"), divides },
      { Symbol("*"), multiplies },
      { Symbol(">"), greater },
      { Symbol("<"), less },
      { Symbol("="), equal },
      { Symbol("car"), car },
      { Symbol("cdr"), cdr },
      { Symbol("list"), list },
      { Symbol("length"), length }
    };
  }

  std::any eval(std::string input) 
  {
    input = std::regex_replace(input, std::regex(R"([(])"), " ( ");
    input = std::regex_replace(input, std::regex(R"([)])"), " ) ");

    std::istringstream iss(input);
    std::istream_iterator<std::string> tokens(iss);

    const expression exp(tokens);
    auto list = parse(exp);

    return ::eval(list, this->env);
  }

  env_ptr env;
};
