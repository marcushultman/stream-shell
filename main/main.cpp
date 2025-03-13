#include <iostream>
#include <string>

int main(int argc, char **argv) {
  std::cout << "> " << std::flush;
  for (std::string buffer;;) {
    std::cin >> buffer;
    std::cout << buffer << std::endl << "> " << std::flush;
  }
  return 0;
}
