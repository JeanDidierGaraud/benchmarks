#pragma once

#include <chrono>
#include <iostream>

namespace aset::asolve {
struct Timer {
  Timer(const std::string& name, int level = 0, bool verbose = true) : level_{level}, name_{name}, cumulated_{0.} {
    if (verbose)
      std::cout << "Starting " << name_ << " ";
    for (int i = 0; i <= level_; i++)
      std::cout << "..";
    std::cout << std::endl;

    start_ = std::chrono::high_resolution_clock::now();
  }

  ~Timer() {}

  auto stop() {
    auto now = std::chrono::high_resolution_clock::now();
    cumulated_ += now - start_;
    return cumulated_;
  }

  void restart() {
    start_ = std::chrono::high_resolution_clock::now();
  }

  auto count() { return cumulated_.count(); }

  void display() {
    for (int i = 0; i <= level_; i++)
      std::cout << "..";
    std::cout << " " << name_ << " done : " << count() << " ms " << std::endl;
  }

  void stop_and_display() {
    stop();
    display();
  }

 private:
  int level_;
  std::string name_;
  std::chrono::time_point<std::chrono::high_resolution_clock> start_;
  std::chrono::duration<double, std::milli> cumulated_;
};
} // namespace aset::asolve
