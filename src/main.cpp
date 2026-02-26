#include <iostream>
#include <string>

int main() {
  // Flush after every std::cout / std:cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;


  while (true) {
    std::string input = "";
    std::cout << "$ ";
  
    std::cin >> input;
  
    if (input != "exit") {
      std::cout << input << ": " << "command not found" << std::endl;
    } else {
      break;
    }
  }

}
