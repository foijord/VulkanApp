
#include <Innovator/Scheme/Scheme2.h>

#include <iostream>
#include <string>

int main(int, char **)
{
  Scheme scheme;
  std::cout << "Innovator Scheme REPL" << std::endl;

  try {
    scheme.eval("(define sum-to (lambda (n) (if (= n 0) 0 (+ n (sum-to (- n 1))))))");
    scheme.eval("(define sum2 (lambda (n acc) (if (= n 0) acc (sum2 (- n 1) (+ n acc)))))");
  }
  catch (std::exception & e) {
    std::cerr << e.what() << std::endl;
  }

  while (true) {
    try {
      std::cout << "> ";
      std::string input;
      std::getline(std::cin, input);
      std::cout << to_string(scheme.eval(input), scheme.env) << std::endl;
    }
    catch (std::exception & e) {
      std::cerr << e.what() << std::endl;
    }
  }
  return 1;
}
