#include "./utils/utils.hpp"
#include "context.hpp"
#include <iostream>
#include <string>

int main() {
  RedisContext<std::string, std::string> redisCtx;
  std::cout << "Hello, Redis\n";

  redisCtx.set("name_id1", "alex");
  redisCtx.set("name_id2", "joao");
  redisCtx.set("name_id2", "joao");

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
  std::cout << '{' << '"' + user.first + '"' << ',' << '"' + user.second + '"'
            << "}\n";

  auto del_res = redisCtx.del("name_id1");
  if (del_res.is_ok()) {
    std::cout << "Removed! \n";
    std::cout << std::to_string(del_res.unwrap()) << "\n";
  } else {
    del_res.cout_err();
  }

  std::cout << "Getting the removed user! \n";

  auto user_rr = redisCtx.get("name_id1");
  if (user_rr.is_err()) {
    user_rr.cout_err();
  }

  return 0;
}
