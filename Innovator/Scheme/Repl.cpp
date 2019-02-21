
#include <Innovator/Scheme/Scheme2.h>

#include <iostream>
#include <string>

int main(int, char **)
{
  Scheme scheme;
  std::cout << "Innovator Scheme REPL" << std::endl;

  while (true) {
    try {
      std::cout << "> ";
      std::string input;
      std::getline(std::cin, input);
      std::cout << to_string(scheme.eval(input)) << std::endl;
    }
    catch (std::exception & e) {
      std::cerr << e.what() << std::endl;
    }
  }
  return 1;
}
