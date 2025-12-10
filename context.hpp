#ifndef CONTEXT_HPP
#define CONTEXT_HPP

#include "./utils/utils.hpp"
#include <string>
#include <unordered_map>
#include <vector>

struct RedisContextError {
  std::string message;
  int code = 0;

  RedisContextError(const std::string &msg, int c = 0) {
    message = msg;
    code = c;
  }
};

template <typename KeyType, typename ValueType> class RedisContext {
public:
  RedisContext() = default;
  using OkType = std::vector<KeyType>;
  using ErrType = RedisContextError;

  Result<OkType, ErrType> get_keys() const {
    using Ok = RedisContext::OkType;
    using Err = RedisContext::ErrType;

    if (map.empty()) {
      return Result<Ok, Err>::err(RedisContextError("No Keys Found", 404));
    }

    std::vector<std::string> vec;
    vec.reserve(map.size());

    for (const auto &pair : map) {
      vec.push_back(pair.first);
    }

    return Result<Ok, Err>::ok(vec);
  };

private:
  std::unordered_map<KeyType, ValueType> map;
};

#endif
