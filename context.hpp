#ifndef CONTEXT_HPP
#define CONTEXT_HPP

#include "./utils/utils.hpp"
#include <cstdint>
#include <string>
#include <unordered_map>
#include <utility>
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
  using ErrType = RedisContextError;

  /* get all keys */
  Result<std::vector<KeyType>, ErrType> get_keys() const {
    using Ok = std::vector<KeyType>;
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

  /* set an entry */
  Result<uint8_t, ErrType> set(KeyType key, ValueType value) {
    using Ok = uint8_t;
    using Err = RedisContext::ErrType;

    if (map.find(key) == map.end()) {
      map.insert(std::make_pair(key, value));
      return Result<Ok, Err>::ok(200);
    } else {
      return Result<Ok, Err>::err(
          RedisContextError("Duplicate Key Found", 409));
    }
  }

  /* get an entry */
  Result<std::pair<KeyType, ValueType>, ErrType> get(KeyType key) {
    using Ok = std::pair<KeyType, ValueType>;
    using Err = RedisContext::ErrType;

    if (auto p = map.find(key); p != map.end()) {
      return Result<Ok, Err>::ok(std::make_pair(p->first, p->second));
    } else {
      return Result<Ok, Err>::err(RedisContextError("Not Found", 404));
    }
  }

  /* delete an entry */
  Result<uint8_t, ErrType> del(KeyType key) {
    using Ok = uint8_t;
    using Err = RedisContext::ErrType;

    if (auto p = map.erase(key); p == 1) {
      return Result<Ok, Err>::ok(200);
    } else {
      return Result<Ok, Err>::err(RedisContextError("Not Found", 404));
    }
  }

private:
  std::unordered_map<KeyType, ValueType> map;
};

#endif
