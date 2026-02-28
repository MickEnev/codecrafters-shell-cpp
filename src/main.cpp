#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <filesystem>
#include <cstring>
#include <sstream>
#include <unistd.h> 
#include <sys/wait.h>

namespace fs = std::filesystem;

std::vector<std::string> getPathDirs() {
  std::vector<std::string> parts;

  const char* path_env = std::getenv("PATH");
  if (!path_env) return parts;

  std::stringstream ss(path_env);
  std::string token;

  while (std::getline(ss, token, ':')) {
      parts.push_back(token);
  }
  return parts;
}

std::vector<std::string> parseArgs(const std::string& line) {
  std::stringstream iss(line);
  std::vector<std::string> args;
  std::string temp;

  while (iss >> temp) {
      args.push_back(temp);
  }
  return args;
}

void echo(const std::vector<std::string>& args) {
  for (size_t i = 1; i < args.size(); i++) {
        std::cout << args[i];
        if (i + 1 < args.size()) std::cout << " ";
    }
    std::cout << std::endl;
}

void checkCustomCommand(const std::vector<std::string>& args) {
  bool found = false;
  // Convert args -> argv (execv format)
  std::vector<char*> argv;
  for (const auto& s : args) {
      argv.push_back(const_cast<char*>(s.c_str()));
  }
  argv.push_back(nullptr);

  std::string command = args[0];

  // parse each directory 
  std::vector<std::string> parts = getPathDirs();

  for (const auto& p : parts) {
    std::string file = p + "/" + command;
    if (fs::exists(file) && access(file.c_str(), X_OK) == 0) {
        pid_t pid = fork();
        if (pid == 0) {
          execv(file.c_str(), argv.data());
          exit(1);
        } else if (pid > 0) {
            found = true;
            int status;
            waitpid(pid, &status, 0);
            break;
        }
    }
  }
  if (!found) {
    std::cout << command << ": " << "command not found" << std::endl;
  }
}

void checkType(const std::string& command, const std::vector<std::string>& validCommands) {
  bool found = false;
  if (std::find(validCommands.begin(), validCommands.end(), command) != validCommands.end()) {
    std::cout << command << " is a shell builtin" << std::endl;
  } else {
    // parse each directory 
    std::vector<std::string> parts = getPathDirs();

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

bool builtin(std::string& command) {
  std::vector<std::string> validCommands = {"exit", "echo", "type"};
  return std::find(validCommands.begin(), validCommands.end(), command) != validCommands.end();
}

void runBuiltin(const std::vector<std::string>& args) {
  std::string command = args[0];
  std::vector<std::string> validCommands = {"exit", "echo", "type"};
  if (command == "exit") {
    exit(0);
  }
  if (command == "echo") {
    echo(args);
  }
  if (command == "type") {
    if (args.size() < 2) {
      std::cout << "type: missing argument\n";
      return;
    }   
    checkType(args[1], validCommands);
  }
}

void external(const std::vector<std::string>& args) {

}

int main() {
  // Flush after every std::cout / std:cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  fs::path curDir = fs::current_path();

  

  while (true) {
    std::cout << "$ ";
  
    std::string line;
    std::getline(std::cin, line);

    auto args = parseArgs(line);
    if (args.empty()) continue;

    std::string command = args[0];
    
    if (!builtin(command)) {
      checkCustomCommand(args);
    } else {
      runBuiltin(args);
    }
  }
}

