#include <iostream>
#include <string>

int main() {

  std::string input = "";
  // Flush after every std::cout / std:cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  std::cout << "$ ";

  std::cin >> input;

  if (input.size() > 0) {
    std::cout << input << ": " << "command not found" << std::endl;
  }
}
