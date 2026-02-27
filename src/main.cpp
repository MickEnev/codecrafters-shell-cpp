#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <filesystem>
#include <cstring>
#include <sstream>
#include <unistd.h> 
#include <cstring>
#include <sys/wait.h>

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
      std::string input;
      std::getline(std::cin, input);

      std::stringstream iss(input);
      std::vector<std::string> args;
      std::string temp;

      while (iss >> temp) {
          args.push_back(temp);
      }
      std::vector<char*> argv;

      for (auto& s : args) {
          argv.push_back(const_cast<char*>(s.c_str()));
      }
      argv.push_back(nullptr);
      // custom stuffs
      const char* path_env = std::getenv("PATH");
          // parse each directory 
          std::vector<std::string> parts;
          std::stringstream ss(path_env);
          std::string token;

          while (std::getline(ss, token, ':')) {
              parts.push_back(token);
          }

          std::string command = argv[0];

          for (const auto& p : parts) {
            std::string file = p + "/" + command;
            if (fs::exists(file) && access(file.c_str(), X_OK) == 0) {
                pid_t pid = fork();
                if (pid == 0) {
                    execv(file.c_str(), argv.data());
                    exit(1);
                } else if (pid > 0) {
                    int status;
                    waitpid(pid, &status, 0);
                }
            } else {
              std::cout << input << ": " << "command not found" << std::endl;
              break;
            }
          }
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
        bool found = false;
        if (std::find(validCommands.begin(), validCommands.end(), command) != validCommands.end()) {
          std::cout << command << " is a shell builtin" << std::endl;
        } else {
          const char* path_env = std::getenv("PATH");
          // parse each directory 
          std::vector<std::string> parts;
          std::stringstream ss(path_env);
          std::string token;

          while (std::getline(ss, token, ':')) {
              parts.push_back(token);
          }

          for (const auto& p : parts) {
            std::string file = p + "/" + command;
            if (fs::exists(file) && access(file.c_str(), X_OK) == 0) {
                std::cout << command << " is " << file << std::endl;
                found = true;
                break;
            }
          }
          if (!found) {
            std::cout << command << ": " << "not found" << std::endl;
          }
        }
      }
    }
  }
  

}
