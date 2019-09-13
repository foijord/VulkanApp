
#include <Innovator/Scheme.h>

#include <string>
#include <chrono>
#include <iostream>

using namespace scm;

int main(int, char **)
{
  std::cout << "Innovator Scheme REPL" << std::endl;
  env_ptr env = scm::global_env();

  while (true) {
    try {
      std::cout << "> ";
      std::string input;
      std::getline(std::cin, input);
      auto start = std::chrono::high_resolution_clock::now();
      std::any exp = scm::read(input);
      auto result = scm::eval(exp, env);
      auto finish = std::chrono::high_resolution_clock::now();
      std::chrono::duration<double> elapsed = finish - start;
      scm::print(result);
      std::cout << std::endl;
      std::cout << "Elapsed time: " << elapsed.count() << " seconds" << std::endl;
    }
    catch (std::exception & e) {
      std::cerr << e.what() << std::endl;
    }
  }
  return 1;
}
