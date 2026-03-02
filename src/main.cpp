#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <filesystem>
#include <cstring>
#include <sstream>
#include <unistd.h> 
#include <sys/wait.h>
#include <fstream>
#include <fcntl.h>
#include <ncurses.h>
#include <readline/readline.h>
#include <readline/history.h>

namespace fs = std::filesystem;

std::vector<std::string> VALID_COMMANDS = {"exit", "echo", "type", "pwd", "cd"};

struct Command {
  std::vector<std::string> args;
  bool redirectOutput = false;
  bool overwriteOutput = true;
  bool redirectError = false;
  std::string file;
  std::string errorFile;
};

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

Command parseArgs(const std::string& line) {
  enum class ParseState {
      NORMAL,
      IN_SINGLE_QUOTE,
      IN_DOUBLE_QUOTE,
      ESCAPE
  };

  ParseState state = ParseState::NORMAL;
  ParseState prevState = ParseState::NORMAL;

  Command cmd;
  bool fileOutput = false;
  bool fileError = false;
  bool overrideOutput = true;

  std::vector<std::string> args;
  std::string current;

  for (size_t i = 0; i < line.size(); i++) {
    char c = line[i];
      if (state == ParseState::NORMAL) {
          if (c == '\'') {
              state = ParseState::IN_SINGLE_QUOTE;
              continue;
          }
          if (c == '"') {
            state = ParseState::IN_DOUBLE_QUOTE;
            continue;
          }
          if (c == '\\') {
            state = ParseState::ESCAPE;
            continue; 
          }
          if (std::isspace(c)) {
              if (!current.empty()) {
                if (fileOutput) {
                  cmd.file = current;
                  cmd.redirectOutput = true;
                  fileOutput = false;
                } else {
                  cmd.args.push_back(current);
                }
                current.clear();
              }
              continue;
          }
          if (c == '>' && state == ParseState::NORMAL) {
            bool append = false;

            if (i + 1 < line.size() && line[i + 1] == '>') {
              append = true;
              i++;
            }

            if (current == "2") {
                fileError = true;  
              } else {
                fileOutput = true;
              }

            if (append) {
              overrideOutput = false;
            }
              
            if (!current.empty()) {
                if (current != "1" && current != "2") {
                  cmd.args.push_back(current);  
                }
                current.clear();
            }
            
            continue;
        }
          current.push_back(c);
      } else if (state == ParseState::IN_SINGLE_QUOTE) {
          if (c == '\'') {
              prevState = state;
              state = ParseState::NORMAL;
              continue;
          }
          current.push_back(c);
      } else if (state == ParseState::IN_DOUBLE_QUOTE) {
          if (c == '"') {
            prevState = state;
            state = ParseState::NORMAL;
            continue;
          }
          if (c == '\\') {
            prevState = state;
            state = ParseState::ESCAPE;
            continue;
          }
          current.push_back(c);
      } else if (state == ParseState::ESCAPE) {
        current.push_back(c);
        state = prevState;
        continue;
      }
  }
  if (!current.empty()) {
      cmd.overwriteOutput = overrideOutput;
    if (fileOutput) {
      cmd.file = current;
      cmd.redirectOutput = true;
    } else if (fileError) {
        cmd.errorFile = current;
        cmd.redirectError = true;
    } else {
      cmd.args.push_back(current);
    }
  }
  return cmd;
}

void echo(const std::vector<std::string>& args) {
  for (size_t i = 1; i < args.size(); i++) {
        std::cout << args[i];
        if (i + 1 < args.size()) std::cout << " ";
    }
    std::cout << std::endl;
}

void checkCustomCommand(Command cmd) {
  bool found = false;

  // Convert args -> argv (execv format)
  std::vector<char*> argv;
  for (const auto& s : cmd.args) {
      argv.push_back(const_cast<char*>(s.c_str()));
  }
  argv.push_back(nullptr);

  std::string command = cmd.args[0];

  // parse each directory 
  std::vector<std::string> parts = getPathDirs();

  for (const auto& p : parts) {
    std::string file = p + "/" + command;
    if (fs::exists(file) && access(file.c_str(), X_OK) == 0) {
        pid_t pid = fork();
        if (pid == 0) {
          if (cmd.redirectOutput) {
            int fd;
            if (cmd.overwriteOutput) {
              fd = open(cmd.file.c_str(),
                            O_WRONLY | O_CREAT | O_TRUNC,
                            0644);
            } else {
              fd = open(cmd.file.c_str(),
                            O_WRONLY | O_CREAT | O_APPEND,
                            0644);
            }

              if (fd < 0) {
                  perror("open");
                  exit(1);
              }

              dup2(fd, STDOUT_FILENO);
              close(fd);
          }
          if (cmd.redirectError) {
            int fd;
            if (cmd.overwriteOutput) {
              fd = open(cmd.errorFile.c_str(),
                            O_WRONLY | O_CREAT | O_TRUNC,
                            0644);
            } else {
              fd = open(cmd.errorFile.c_str(),
                            O_WRONLY | O_CREAT | O_APPEND,
                            0644);
            }

            if (fd >= 0) {
                dup2(fd, STDERR_FILENO);
                close(fd);
            }
          }
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

void changeDirectory(const std::string& newDir) {
  fs::path new_dir = newDir;
  if (newDir == "~") {
    const char* path_env = std::getenv("HOME");
    new_dir = path_env;
  }
  try {
    fs::current_path(new_dir);
  } catch (fs::filesystem_error& e) {
    std::cerr << "cd: " << new_dir.c_str() << ": No such file or directory " << std::endl;
  }
}

bool builtin(std::string& command) {
  
  return std::find(VALID_COMMANDS.begin(), VALID_COMMANDS.end(), command) != VALID_COMMANDS.end();
}

void runBuiltin(Command cmd) {
  std::string command = cmd.args[0];
  if (command == "exit") {
    exit(0);
  }
  if (command == "echo") {
    echo(cmd.args);
  }
  if (command == "type") {
    if (cmd.args.size() < 2) {
      std::cout << "type: missing argument\n";
      return;
    }   
    checkType(cmd.args[1], VALID_COMMANDS);
  }
  if (command == "pwd") {
    std::cout << fs::current_path().c_str() << std::endl;
  }
  if (command == "cd") {
    changeDirectory(cmd.args[1]);
  }
}

void runBuiltinWithRedirect(const Command& cmd) {
    int saved_stdout = dup(STDOUT_FILENO);
    int saved_stderr = dup(STDERR_FILENO);

    // redirect stdout
    if (cmd.redirectOutput) {
      int fd;
      if (cmd.overwriteOutput) {
        fd = open(cmd.file.c_str(),
                      O_WRONLY | O_CREAT | O_TRUNC,
                      0644);
      } else {
        fd = open(cmd.file.c_str(),
                      O_WRONLY | O_CREAT | O_APPEND,
                      0644);
      }

        if (fd < 0) {
            perror("open");
        } else {
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }
    }

    // redirect stderr
    if (cmd.redirectError) {
      int fd;
      if (cmd.overwriteOutput) {
        fd = open(cmd.errorFile.c_str(),
                      O_WRONLY | O_CREAT | O_TRUNC,
                      0644);
      } else {
        fd = open(cmd.errorFile.c_str(),
                      O_WRONLY | O_CREAT | O_APPEND,
                      0644);
      }

        if (fd >= 0) {
            dup2(fd, STDERR_FILENO);
            close(fd);
        }
    }

    runBuiltin(cmd);

    // restore
    dup2(saved_stdout, STDOUT_FILENO);
    dup2(saved_stderr, STDERR_FILENO);

    close(saved_stdout);
    close(saved_stderr);
}

char* command_generator(const char* text, int state) {
    static size_t index;
    static size_t len;

    if (!state) {
        index = 0;
        len = strlen(text);
    }

    while (index < VALID_COMMANDS.size()) {
        const std::string& name = VALID_COMMANDS[index++];
        if (name.compare(0, len, text) == 0) {
            return strdup(name.c_str());
        }
    }
    index = 0;
    static std::vector<std::string> path = getPathDirs();
    while (index < path.size()) {
      static const std::string& name = path[index++];
      for (const auto& entry : fs::directory_iterator(name)) {
        static std::string fileName = entry.path().filename();
        std::cout << fileName << std::endl;
        if (fileName.compare(0, len, text) == 0) {
            return strdup(fileName.c_str());
        }
      }
    }

    return nullptr;
}

char** command_completion(const char* text, int start, int end) {
    rl_attempted_completion_over = 1;
    return rl_completion_matches(text, command_generator);
}

int main() {
  // Flush after every std::cout / std:cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  fs::path curDir = fs::current_path();
  rl_attempted_completion_function = command_completion;

  while (true) {
    char* input = readline("$ ");

    if (!input) break;

    std::string line = input;
    free(input);

    Command cmd  = parseArgs(line);
    
    if (cmd.args.empty()) continue;

    std::string command = cmd.args[0];

    if (!builtin(command)) {
      checkCustomCommand(cmd);
    } else {
      if (cmd.redirectOutput || cmd.redirectError) {
        runBuiltinWithRedirect(cmd);
      } else {
        runBuiltin(cmd);
      }
    }
  }
}

