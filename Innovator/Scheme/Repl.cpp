
#include <Innovator/Scheme/Scheme.h>

#include <iostream>
#include <string>

int main(int, char **)
{
  Scheme scheme;
  std::cout << "Innovator Scheme REPL" << std::endl;

  while (true) {
    try {
      std::cout << "> ";
      std::string expression;
      std::getline(std::cin, expression);
      std::cout << scheme.eval(expression)->toString() << std::endl;
    }
    catch (std::exception & e) {
      std::cerr << e.what() << std::endl;
    }
  }
  return 1;
}
