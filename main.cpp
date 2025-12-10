#include "./utils/utils.hpp"
#include "context.hpp"
#include <iostream>

int main() {
  RedisContext<std::string, std::string> redisCtx;
  std::cout << "Hello, Redis\n";

  auto keys = redisCtx.get_keys();

  if (keys.is_ok()) {
    for (const auto &key : keys.unwrap()) {
      std::cout << key << '\n';
    }
  } else {
    std::cout << "ERROR CODE: " << keys.unwrap_err().code << '\n'
              << "ERROR MESSAGE: " << keys.unwrap_err().message << '\n';
  }

  return 0;
}
