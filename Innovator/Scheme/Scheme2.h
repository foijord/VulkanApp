#pragma once

#include <any>
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

typedef std::vector<std::any> List;
typedef std::shared_ptr<List> lst_ptr;
typedef std::function<std::any(lst_ptr)> fun_ptr;

typedef double Number;
typedef std::string Symbol;

class Env;
typedef std::shared_ptr<Env> env_ptr;

class Env {
public:
  explicit Env(const std::map<Symbol, std::any> & inner)
    : inner(inner)
  {}

  explicit Env(lst_ptr parms, lst_ptr args, env_ptr outer)
    : outer(std::move(outer))
  {
    for (size_t i = 0; i < parms->size(); i++) {
      auto sym = std::any_cast<Symbol>((*parms)[i]);
      this->inner[sym] = (*args)[i];
    }
  }

  std::any eval(Symbol sym)
  {
    if (this->inner.find(sym) != this->inner.end()) {
      return this->inner.at(sym);
    }
    if (this->outer) {
      return this->outer->eval(sym);
    }
    throw std::runtime_error("undefined symbol: " + sym);
  }
    
  std::map<Symbol, std::any> inner;
  env_ptr outer { nullptr };
};

struct Lambda {
  lst_ptr parms;
  lst_ptr body;
};

struct Define {
  Symbol sym;
  std::any exp;
};

std::any eval(std::any exp, env_ptr env)
{
  if (exp.type() == typeid(Number)) {
    return exp;
  }
  if (exp.type() == typeid(Symbol)) {
    return env->eval(std::any_cast<Symbol>(exp));
  }
  if (exp.type() == typeid(Lambda)) {
    return fun_ptr([=](lst_ptr args) {
      auto lambda = std::any_cast<Lambda>(exp);
      return eval(lambda.body, std::make_shared<Env>(lambda.parms, args, env));
    });
  }
  if (exp.type() == typeid(Define)) {
    auto define = std::any_cast<Define>(exp);
    env->inner[define.sym] = eval(define.exp, env);
  } 
  else {
    auto list = std::any_cast<lst_ptr>(exp);

    std::transform(list->begin(), list->end(), list->begin(), [&](std::any exp) { 
      return eval(exp, env); 
    });

    auto fun = std::any_cast<fun_ptr>(list->front());
    list->erase(list->begin());
    return fun(list);
  }
}

class ParseTree : public std::vector<ParseTree> {
public:
  explicit ParseTree(std::istream_iterator<std::string> & it)
  {
    if (*it == "(") {
      while (*(++it) != ")") {
        this->push_back(ParseTree(it));
      }
    }
    this->token = *it;
  }

  std::string token;
};

std::string to_string(std::any exp) 
{
  if (exp.type() == typeid(Number)) {
    return std::to_string(std::any_cast<Number>(exp));
  }
  return "not a Number";
}

inline std::any parse(const ParseTree & parsetree)
{
  if (parsetree.empty()) {
    std::regex match_number(R"(^[+-]?([0-9]*[.])?[0-9]+$)");
    if (std::regex_match(parsetree.token, match_number)) {
      return Number(std::stod(parsetree.token));
    }
    return Symbol(parsetree.token);
  }
  
  List exp(parsetree.size());
  std::transform(parsetree.begin(), parsetree.end(), exp.begin(), parse);

  const std::string token = parsetree[0].token;

  if (token == "lambda") {
    if (exp.size() != 3) {
      throw std::invalid_argument("wrong Number of arguments to lambda");
    }
    if (exp[1].type() != typeid(lst_ptr)) {
      throw std::invalid_argument("first parameter to lambda must be a List of arguments");
    }
    if (exp[2].type() != typeid(lst_ptr)) {
      throw std::invalid_argument("second parameter to lambda must be an expression body");
    }
    return Lambda { std::any_cast<lst_ptr>(exp[1]), std::any_cast<lst_ptr>(exp[2]) };
  }

  if (token == "define") {
    if (exp.size() != 3) {
      throw std::invalid_argument("wrong number of arguments to Define");
    }
    if (exp[1].type() != typeid(Symbol)) {
      throw std::invalid_argument("first argument to Define must be a Symbol");
    }
    return Define{ std::any_cast<Symbol>(exp[1]), exp[2] };
  }

  return std::make_shared<List>(exp);
}

fun_ptr plus = [](lst_ptr args)
{
  std::vector<Number> dargs(args->size());
  std::transform(args->begin(), args->end(), dargs.begin(), 
    [](std::any arg) { return std::any_cast<Number>(arg); });

  return std::accumulate(next(dargs.begin()), dargs.end(), dargs.front(), std::plus<Number>());
};

static std::map<Symbol, std::any> global {
  { "pi",  Number(3.14159265358979323846) },
  { "+", plus }
};

class Scheme {
public:
  Scheme() :
    env(std::make_shared<Env>(global))
  {}

  std::any eval(std::string input) const
  {
    input = std::regex_replace(input, std::regex(R"([(])"), " ( ");
    input = std::regex_replace(input, std::regex(R"([)])"), " ) ");

    std::istringstream iss(input);
    std::istream_iterator<std::string> tokens(iss);
    const ParseTree parsetree(tokens);
    auto exp = parse(parsetree);

    return ::eval(exp, this->env);
  }

  env_ptr env;
};
