#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

int main() {
  // Flush after every std::cout / std:cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  std::vector<std::string> validCommands = {"exit", "echo", "type"};


  while (true) {
    std::string input = "";
    std::cout << "$ ";
  
    std::cin >> input;
  
    if (std::find(validCommands.begin(), validCommands.end(), input) == validCommands.end()) {
      std::cout << input << ": " << "command not found" << std::endl;
    } else {
      if (input == "exit") {
        break;
      }
      if (input == "echo") {
        std::string echoSentence;
        std::getline(std::cin, echoSentence);
        std::cout << echoSentence.substr(1) << std::endl;
      }
      if (input == "type") {
        std::string command;
        std::getline(std::cin, command);
        if (std::find(validCommands.begin(), validCommands.end(), command.substr(1)) == validCommands.end()) {
          std::cout << command.substr(1) << " is a shell builtin" << std::endl;
        } else {
          std::cout << command.substr(1) << ": " << "command not found" << std::endl;
        }
      }
    }
  }

}
