#include "./utils/utils.hpp"
#include "context.hpp"
#include <iostream>

int main() {
  RedisContext<std::string, std::string> redisCtx;
  std::cout << "Hello, Redis\n";

  auto res = redisCtx.set("name_id1", "alex");
  if (res.is_err()) {
    res.cout_err();
  }
  auto res2 = redisCtx.set("name_id2", "joao");
  if (res2.is_err()) {
    res2.cout_err();
  }
  auto res3 = redisCtx.set("name_id2", "joao");
  if (res3.is_err()) {
    res3.cout_err();
  }

  auto keys = redisCtx.get_keys();

  if (keys.is_ok()) {
    std::cout << "Keys:" << '\n';
    for (const auto &key : keys.unwrap()) {
      std::cout << key << '\n';
    }
  } else {
    keys.cout_err();
  }

  auto user_r = redisCtx.get("name_id1");
  if (user_r.is_err()) {
    user_r.cout_err();
  }

  auto user = user_r.unwrap();
  std::cout << '{' << user.first << ',' << user.second << "}\n";

  return 0;
}
