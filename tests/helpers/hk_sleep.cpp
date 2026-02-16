#include <cstdlib>
#include <iostream>
#include <string>
#include <unistd.h>

int main(int argc, char* argv[]) {
  int seconds = 60; // default
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--seconds" && i + 1 < argc) {
      seconds = std::atoi(argv[++i]);
    }
  }

  sleep(seconds);
  return 0;
}