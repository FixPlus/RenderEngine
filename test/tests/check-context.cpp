#include "TestContext.h"
#include <chrono>
#include <iostream>
#include <thread>

int main() {
  using namespace std::chrono_literals;
  try {
    re::test::BasicContextCreator cc;
    auto context = cc.create(true, &std::cout);
    std::cout << std::endl;
    auto sleep = 1000ms;
    std::cout << "Sleeping for " << sleep << "..." << std::endl;
    std::this_thread::sleep_for(sleep);
  } catch (std::runtime_error &e) {
    std::cout << std::endl << "[ERROR] - " << e.what() << std::endl;
    std::cout << "Test FAIL" << std::endl;
    return -1;
  }
  std::cout << std::endl << "Test OK" << std::endl;
  return 0;
}