#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <filesystem>
#include <cstring>

namespace fs = std::filesystem;

int main() {
  // Flush after every std::cout / std:cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  fs::path curDir = fs::current_path();

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
        std::cin >> command;
        if (std::find(validCommands.begin(), validCommands.end(), command) != validCommands.end()) {
          std::cout << command << " is a shell builtin" << std::endl;
        } else {
          const char* path_env = std::getenv("PATH");
            for (int i = 0; i < strlen(path_env); i++) {
              std::string curDir = "";
              int slashIndex = 0;
              while (path_env[i] != ':') {
                curDir += path_env[i];
                if (path_env[i] == '/') {
                  slashIndex = i;
                }
                
                std::string file = curDir.substr(slashIndex);
                // Check if file == command 
                if (file == command && has_execute_permission(file)) {
                  std::cout << command << " is " << curDir << std::endl;
                } else {
                  continue;
                }

              }
              
              std::cout << path_env[i] << std::endl;
            }
          std::cout << command << ": " << "not found" << std::endl;
        }
      }
    }
  }
  

}

bool has_execute_permission(const std::string& filename) {
    std::error_code ec;
    fs::file_status s = fs::status(filename, ec);

    if (ec) {
        std::cerr << "Error getting file status: " << ec.message() << std::endl;
        return false;
    }

    fs::perms p = s.permissions();

    if (((p & fs::perms::owner_exec) != fs::perms::none) ||
        ((p & fs::perms::group_exec) != fs::perms::none) ||
        ((p & fs::perms::others_exec) != fs::perms::none)) {
        return true;
    }
    return false;
}
