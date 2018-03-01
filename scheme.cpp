
#include <map>
#include <list>
#include <regex>
#include <vector>
#include <memory>
#include <numeric>
#include <iostream>
#include <algorithm>
#include <functional>

using namespace std;

class Expression;
class Environment;

shared_ptr<class Expression> eval(shared_ptr<Expression> exp,
                                  shared_ptr<Environment> env);

class Expression : public list<shared_ptr<Expression>> {
public:
  enum Type {
    SYMBOL,
    NUMBER,
    BOOLEAN,
    QUOTE,
    DEFINE,
    SET,
    IF,
    LIST,
    LAMBDA,
    FUNCTION
  };

  Expression(Type type = LIST);
  Expression(Expression::iterator begin, Expression::iterator end);
  
  virtual string toString();
  virtual shared_ptr<Expression> eval(shared_ptr<Environment> & env);

  Type type;
};

class Environment : public map<string, shared_ptr<Expression>> {
public:
  Environment();
  
  Environment(shared_ptr<Expression> & parms,
              shared_ptr<Expression> & args,
              const shared_ptr<Environment> & outer = make_shared<Environment>());
  
  Environment * getEnv(const string & sym);
  
  shared_ptr<Environment> outer;
};

class Symbol : public Expression {
public:
  Symbol(const string & token);
  
  virtual string toString();
  virtual shared_ptr<Expression> eval(shared_ptr<Environment> & env);
  
  string token;
};


Environment::Environment() {}
  
Environment::Environment(shared_ptr<Expression> & parms,
                         shared_ptr<Expression> & args,
                         const shared_ptr<Environment> & outer)
  : outer(outer)
{
  for (auto p = parms->begin(), a = args->begin(); p != parms->end() && a != args->end(); ++p, ++a) {
    auto parm = dynamic_pointer_cast<Symbol>(*p);
    (*this)[parm->token] = *a;
  }
}

  
Environment *
Environment::getEnv(const string & sym)
{
  if (this->find(sym) != this->end()) {
    return this;
  }
  if (!this->outer) {
    throw runtime_error("undefined symbol '" + sym + "'");
  }
  return this->outer->getEnv(sym);
}


Expression::Expression(Type type)
  : type(type)
{}
  
Expression::Expression(Expression::iterator begin, Expression::iterator end)
  : list<shared_ptr<Expression>>(begin, end)
{}
  
string
Expression::toString()
{
  string s = "(";
  for (auto it = this->begin(); it != this->end(); it++) {
    s += (*it)->toString() + " ";
  }
  return s += ")";
}
  
shared_ptr<Expression>
Expression::eval(shared_ptr<Environment> & env)
{ 
  auto exp = make_shared<Expression>();
  for (auto it = this->begin(); it != this->end(); ++it) {
    exp->push_back(::eval(*it, env));
  }
  return exp;
}
  
Symbol::Symbol(const string & token) 
  : Expression(SYMBOL), token(token)
{}
  
string
Symbol::toString()
{
  return this->token;
}
  
shared_ptr<Expression>
Symbol::eval(shared_ptr<Environment> & env)
{ 
  return (*env->getEnv(this->token))[this->token];
}

class Number : public Expression {
public:
  Number(double value) 
    : Expression(NUMBER), value(value)
  {}

  Number(const string & token) 
    : Expression(NUMBER), value(stod(token))
  {}
  
  virtual string toString()
  {
    return to_string(this->value);
  }

  double value;
};

class Boolean : public Expression {
public:
  Boolean(bool value) 
    : Expression(BOOLEAN), value(value)
  {}
  
  Boolean(const string & token)
    : Expression(BOOLEAN)
  {
    if (token == "#t") {
      this->value = true;
    }
    else if (token == "#f") {
      this->value = false;
    }
    else {
      throw invalid_argument("expression is not boolean");
    }
  }
  
  virtual string toString()
  {
    return this->value ? "#t" : "#f";
  }
  
  bool value;
};

class Quote : public Expression {
public:
  Quote(shared_ptr<Expression> exp)
    : Expression(QUOTE), exp(exp)
  {}
  
  virtual shared_ptr<Expression> eval(shared_ptr<Environment> & env)
  {
    return this->exp;
  }
  
  shared_ptr<Expression> exp;
};

class Define : public Expression {
public:
  Define(shared_ptr<Symbol> var, shared_ptr<Expression> exp) 
    : Expression(DEFINE), var(var), exp(exp)
  {}
  
  virtual shared_ptr<Expression> eval(shared_ptr<Environment> & env)
  {
    (*env)[this->var->token] = ::eval(this->exp, env);
    return make_shared<Expression>();
  }
  
  shared_ptr<Symbol> var;
  shared_ptr<Expression> exp;
};

class If : public Expression {
public:
  If(shared_ptr<Expression> test, shared_ptr<Expression> conseq, shared_ptr<Expression> alt) 
    : Expression(IF), test(test), conseq(conseq), alt(alt)
  {}
  
  virtual shared_ptr<Expression> eval(shared_ptr<Environment> & env)
  {
    return (static_pointer_cast<Boolean>(::eval(this->test, env))->value) ? this->conseq : this->alt;
  }
  
  shared_ptr<Expression> test, conseq, alt;
};

class Function : public Expression {
public:
  Function(shared_ptr<Expression> parms, shared_ptr<Expression> body, shared_ptr<Environment> env)
    : Expression(FUNCTION), parms(parms), body(body), env(env)
  {}
  
  shared_ptr<Environment> env;
  shared_ptr<Expression> parms, body;
};

class Lambda : public Expression {
public:
  Lambda(shared_ptr<Expression> parms, shared_ptr<Expression> body) 
    : Expression(LAMBDA), parms(parms), body(body)
  {}
  
  virtual shared_ptr<Expression> eval(shared_ptr<Environment> & env)
  {
    return make_shared<Function>(this->parms, this->body, env);
  }
  
  shared_ptr<Expression> parms, body;
};

class Callable : public Expression {
public:
  virtual shared_ptr<Expression> operator()(const shared_ptr<Expression> & args) = 0;
  
  vector<double> getDoubleArgs(const shared_ptr<Expression> & args)
  {
    vector<double> dargs(args->size());
    transform(args->begin(), args->end(), dargs.begin(), [](auto & arg) {
        return static_pointer_cast<Number>(arg)->value;
    });
    return dargs;
  }
};

template <typename T>
class Operator : public Callable {
public:
  virtual shared_ptr<Expression> operator()(const shared_ptr<Expression> & args)
  {
    vector<double> dargs = this->getDoubleArgs(args);
    return make_shared<Number>(accumulate(next(dargs.begin()), dargs.end(), dargs[0], op));
  }
  
  T op;
};

typedef Operator<plus<double>> Add;
typedef Operator<minus<double>> Sub;
typedef Operator<divides<double>> Div;
typedef Operator<multiplies<double>> Mul;

class Less : public Callable {
public:
  virtual shared_ptr<Expression> operator()(const shared_ptr<Expression> & args)
  {
    vector<double> dargs = this->getDoubleArgs(args);
    return make_shared<Boolean>(dargs[0] < dargs[1]);
  }
};

class More : public Callable {
public:
  virtual shared_ptr<Expression> operator()(const shared_ptr<Expression> & args)
  {
    vector<double> dargs = this->getDoubleArgs(args);
    return make_shared<Boolean>(dargs[0] > dargs[1]);
  }
};

class Same: public Callable {
public:
  virtual shared_ptr<Expression> operator()(const shared_ptr<Expression> & args)
  {
    vector<double> dargs = this->getDoubleArgs(args);
    return make_shared<Boolean>(dargs[0] == dargs[1]);
  }
};

class Car : public Callable {
  virtual shared_ptr<Expression> operator()(const shared_ptr<Expression> & args)
  {
    return args->front();
  }
};

class Cdr : public Callable {
  virtual shared_ptr<Expression> operator()(const shared_ptr<Expression> & args)
  {
    return make_shared<Expression>(next(args->begin()), args->end());
  }
};

class ParseTree : public vector<ParseTree> {
public:
  ParseTree(sregex_token_iterator & it)
  {
    if (it->str() == "(") {
      while (*(++it) != ")") {
        this->push_back(ParseTree(it));
      }
    }
    this->token = *it;
  }
  string token;
};

shared_ptr<Expression> eval(shared_ptr<Expression> exp, shared_ptr<Environment> env)
{
  while (true) {
    switch (exp->type) {
      case Expression::NUMBER:
      case Expression::BOOLEAN:
        return exp;

      case Expression::SET:
      case Expression::QUOTE:
      case Expression::SYMBOL:
      case Expression::DEFINE:
      case Expression::LAMBDA:
        return exp->eval(env);

      case Expression::IF:
        exp = exp->eval(env);
        continue;

      default: {
        auto exps = exp->eval(env);
        auto front = exps->front();
        exps->pop_front();
        
        if (front->type == Expression::FUNCTION) {
          auto func = dynamic_pointer_cast<Function>(front);
          exp = func->body;
          env = make_shared<Environment>(func->parms, exps, func->env);
        }
        else {
          auto builtin = dynamic_pointer_cast<Callable>(front);
          return (*builtin)(exps);
        }
      }
    }
  }
}

shared_ptr<Expression> parse(const ParseTree & parsetree)
{
  if (parsetree.empty()) {
    try {
      return make_shared<Number>(parsetree.token);
    }
    catch (invalid_argument) {
      try {
        return make_shared<Boolean>(parsetree.token);
      }
      catch (invalid_argument) {
        return make_shared<Symbol>(parsetree.token);
      }
    }
  }
  
  if (parsetree[0].token == "quote") {
    if (parsetree.size() != 2) {
      throw runtime_error("invalid num args to quote");
    }
    return make_shared<Quote>(parse(parsetree[1]));
  }
  
  if (parsetree[0].token == "if") {
    if (parsetree.size() != 4) {
      throw runtime_error("invalid num args to if");
    }
    return make_shared<If>(parse(parsetree[1]), parse(parsetree[2]), parse(parsetree[3]));
  }
  
  if (parsetree[0].token == "lambda") {
    if (parsetree.size() != 3) {
      throw runtime_error("invalid num arguments for lambda");
    }
    return make_shared<Lambda>(parse(parsetree[1]), parse(parsetree[2]));
  }
  
  if (parsetree[0].token == "define") {
    if (parsetree.size() != 3) {
      throw runtime_error("invalid num arguments for define");
    }
    auto var = dynamic_pointer_cast<Symbol>(parse(parsetree[1]));
    if (!var) {
      throw runtime_error("variable must be symbol");
    }
    auto exp = parse(parsetree[2]);
    return make_shared<Define>(var, exp);
  }
  
  if (parsetree[0].token == "list") {
    if (parsetree.size() < 2) {
      throw runtime_error("too few arguments to function list");
    }
  }
  
  if (parsetree[0].token == "<") {
    if (parsetree.size() != 3) {
      throw runtime_error("wrong number of arguments to <");
    }
  }
  
  if (parsetree[0].token == ">") {
    if (parsetree.size() != 3) {
      throw runtime_error("wrong number of arguments to >");
    }
  }
  
  if (parsetree[0].token == "=") {
    if (parsetree.size() != 3) {
      throw runtime_error("wrong number of arguments to =");
    }
  }

  auto list = make_shared<Expression>();
  for (const ParseTree & pt : parsetree) {
    list->push_back(parse(pt));
  }
  
  return list;
}

int main() {
  regex tokenizer("[()]|[a-z]+|[0-9]+|[+*-/<>=]");

  auto global_env = make_shared<Environment>();
  (*global_env)["+"] = make_shared<Add>();
  (*global_env)["-"] = make_shared<Sub>();
  (*global_env)["/"] = make_shared<Div>();
  (*global_env)["*"] = make_shared<Mul>();
  (*global_env)["<"] = make_shared<Less>();
  (*global_env)[">"] = make_shared<More>();
  (*global_env)["="] = make_shared<Same>();
  (*global_env)["car"] = make_shared<Car>();
  (*global_env)["cdr"] = make_shared<Cdr>();
  (*global_env)["pi"] = make_shared<Number>(3.14159265358979323846);

  while (true) {
    cout << "scm> ";
    string input;
    {
      input = string("(define twice (lambda (x) (* 2 x)))");
      sregex_token_iterator tokens(input.begin(), input.end(), tokenizer);
      ParseTree parsetree(tokens);
      auto exp = parse(parsetree);
      eval(exp, global_env);
    }
    {
      input = string("(define repeat (lambda (f) (lambda (x) (f (f x)))))");
      sregex_token_iterator tokens(input.begin(), input.end(), tokenizer);
      ParseTree parsetree(tokens);
      auto exp = parse(parsetree);
      eval(exp, global_env);
    }
    {
      input = string("(define sum(lambda(n acc) (if (= n 0) acc(sum(-n 1) (+n acc)))))");
      sregex_token_iterator tokens(input.begin(), input.end(), tokenizer);
      ParseTree parsetree(tokens);
      auto exp = parse(parsetree);
      eval(exp, global_env);
    }
    {
      input = string("(sum 1000 0)");
      sregex_token_iterator tokens(input.begin(), input.end(), tokenizer);
      ParseTree parsetree(tokens);
      auto exp = parse(parsetree);
      eval(exp, global_env);
    }
    getline(cin, input);
    sregex_token_iterator tokens(input.begin(), input.end(), tokenizer);
    try {
      ParseTree parsetree(tokens);
      auto exp(parse(parsetree));
      cout << "=> " << eval(exp, global_env)->toString() << endl;
    }
    catch (runtime_error & e) {
      cout << e.what() << endl;
    }
  }
  return 0;
}
