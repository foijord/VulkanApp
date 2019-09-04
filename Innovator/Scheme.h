#pragma once

#include <any>
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

namespace scm {

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

  struct If {};
  struct Quote {};
  struct Define {};
  struct Lambda {};
  struct Begin{};

  struct Function {
    std::any parms, exp;
    env_ptr env;
  };

  template <typename T>
  std::vector<T> any_cast(const List & lst)
  {
    std::vector<T> args(lst.size());
    std::transform(lst.begin(), lst.end(), args.begin(),
      [](std::any exp) { return std::any_cast<T>(exp); });
    return args;
  }

  fun_ptr plus = [](const List & lst)
  {
    std::vector<Number> args = any_cast<Number>(lst);
    return std::accumulate(next(args.begin()), args.end(), args.front(), std::plus<Number>());
  };

  fun_ptr minus = [](const List & lst)
  {
    std::vector<Number> args = any_cast<Number>(lst);
    return std::accumulate(next(args.begin()), args.end(), args.front(), std::minus<Number>());
  };

  fun_ptr divides = [](const List & lst)
  {
    std::vector<Number> args = any_cast<Number>(lst);
    return std::accumulate(next(args.begin()), args.end(), args.front(), std::divides<Number>());
  };

  fun_ptr multiplies = [](const List & lst)
  {
    std::vector<Number> args = any_cast<Number>(lst);
    return std::accumulate(next(args.begin()), args.end(), args.front(), std::multiplies<Number>());
  };

  fun_ptr greater = [](const List & lst)
  {
    std::vector<Number> args = any_cast<Number>(lst);
    return Boolean(args[0] > args[1]);
  };

  fun_ptr less = [](const List & lst)
  {
    std::vector<Number> args = any_cast<Number>(lst);
    return Boolean(args[0] < args[1]);
  };

  fun_ptr equal = [](const List & lst)
  {
    std::vector<Number> args = any_cast<Number>(lst);
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
    Env(std::unordered_map<std::string, std::any> inner)
      : inner(inner)
    {}
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
      }
      else {
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
    env_ptr outer{ nullptr };
  };

  env_ptr global_env()
  {
    return std::make_shared<Env>(
      std::unordered_map<std::string, std::any>{
        { Symbol("pi"), Number(3.14159265358979323846) },
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
    });
  }

void print(std::any exp) 
{
  if (exp.type() == typeid(Number)) {
    std::cout << std::any_cast<Number>(exp);
  } 
  else if (exp.type() == typeid(Symbol)) {
    std::cout << std::any_cast<Symbol>(exp);
  }
  else if (exp.type() == typeid(String)) {
    std::cout << std::any_cast<String>(exp);
  }
  else if (exp.type() == typeid(Boolean)) {
    std::string s = std::any_cast<Boolean>(exp) ? "#t" : "#f";
    std::cout << s;
  }
  else if (exp.type() == typeid(Begin)) {
    std::cout << "begin";
  }
  else if (exp.type() == typeid(Define)) {
    std::cout << "define";
  }
  else if (exp.type() == typeid(Lambda)) {
    std::cout << "lambda";
  }
  else if (exp.type() == typeid(If)) {
    std::cout << "if";
  }
  else if (exp.type() == typeid(Quote)) {
    std::cout << "quote";
  }
  else if (exp.type() == typeid(fun_ptr)) {
    std::cout << "function";
  }
  else if (exp.type() == typeid(lst_ptr)) {
    auto & list = *std::any_cast<lst_ptr>(exp);

    std::cout << "(";
    for (auto s : list) {
      print(s);
      std::cout << " ";
    }
    std::cout << ")";
  } else {
    std::cout << "()";
  }
}

std::any eval(std::any & exp, env_ptr & env)
{
  while (true) {
    if (exp.type() == typeid(lst_ptr)) {
      auto & list = *std::any_cast<lst_ptr>(exp);
      
      if (list[0].type() == typeid(Define)) {
        auto symbol = std::any_cast<Symbol>(list[1]);
        std::any value = eval(list[2], env);
        env->inner[symbol] = value;
        return value;
      }
      else if (list[0].type() == typeid(Lambda)) {
        return Function{ list[1], list[2], env };
      }
      else if (list[0].type() == typeid(If)) {
        exp = std::any_cast<Boolean>(eval(list[1], env)) ? list[2]: list[3];
        continue;
      }
      else if (list[0].type() == typeid(Quote)) {
        return list[1];
      }
      else if (list[0].type() == typeid(Begin)) {
        for (size_t i = 1; i < list.size(); i++) {
          eval(list[i], env);
        }
        exp = list[list.size() - 1];
        continue;
      }

      auto car = eval(list[0], env);
      auto cdr = List(list.size() - 1);

      std::transform(next(list.begin()), list.end(), cdr.begin(), [&](std::any exp) { 
        return eval(exp, env);
      });

      if (car.type() == typeid(Function)) {
        auto f = std::any_cast<Function>(car);
        exp = f.exp;
        env = std::make_shared<Env>(f.parms, cdr, f.env);
      } else {
        auto f = std::any_cast<fun_ptr>(car);
        return f(cdr);
      }
    }
    else {
      if (exp.type() == typeid(Number) ||
          exp.type() == typeid(Boolean)) {
        return exp;      
      }
      else if (exp.type() == typeid(String)) {
        auto string = std::any_cast<String>(exp);
        return string.substr(1, string.size() - 2);
      }
      else if (exp.type() == typeid(Symbol)) {
        auto symbol = std::any_cast<Symbol>(exp);
        return env->get(symbol);
      }
    }
  }
}

std::any ast(std::istream_iterator<std::string> & it)
{
  if (*it == "(") {
    List list;
    while (*(++it) != ")") {
      list.push_back(ast(it));
    }
    return list;
  }
  else {
    return *it;
  }
}

inline std::any parse(std::any exp)
{
  if (exp.type() == typeid(List)) {
    auto list = std::any_cast<List>(exp);
    std::transform(list.begin(), list.end(), list.begin(), parse);

    if (list[0].type() == typeid(Symbol)) {
      auto token = std::any_cast<Symbol>(list[0]);

      if (token == Symbol("quote")) {
        if (list.size() != 2) {
          throw std::invalid_argument("wrong number of arguments to quote");
        }
        list[0] = Quote{};
      }
      if (token == Symbol("if")) {
        if (list.size() != 4) {
          throw std::invalid_argument("wrong number of arguments to if");
        }
        list[0] = If{};
      }
      if (token == Symbol("lambda")) {
        if (list.size() != 3) {
          throw std::invalid_argument("wrong Number of arguments to lambda");
        }
        list[0] = Lambda{};
      }
      if (token == Symbol("begin")) {
        if (list.size() < 2) {
          throw std::invalid_argument("wrong Number of arguments to begin");
        }
        list[0] = Begin{};
      }
      if (token == Symbol("define")) {
        if (list.size() != 3) {
          throw std::invalid_argument("wrong number of arguments to Define");
        }
        if (list[1].type() != typeid(Symbol)) {
          throw std::invalid_argument("first argument to Define must be a Symbol");
        }
        list[0] = Define{};
      }
    }
    return std::make_shared<List>(list);
  }

  auto token = std::any_cast<std::string>(exp);
  if (token == "#t") {
    return Boolean(true);
  }
  if (token == "#f") {
    return Boolean(false);
  }
  if (token.front() == '"' && token.back() == '"') {
    return String(token);
  }
  std::regex match_number(R"(^[+-]?([0-9]*[.])?[0-9]+$)");
  if (std::regex_match(token, match_number)) {
    return Number(std::stod(token));
  }
  return Symbol(token);
}

std::any read(std::string input)
{
  input = std::regex_replace(input, std::regex(R"([(])"), " ( ");
  input = std::regex_replace(input, std::regex(R"([)])"), " ) ");

  std::istringstream iss(input);
  std::istream_iterator<std::string> tokens(iss);

  return parse(ast(tokens));
};
} // namespace scm