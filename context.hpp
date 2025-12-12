#ifndef CONTEXT_HPP
#define CONTEXT_HPP

#include "./utils/utils.hpp"
#include <cstddef>
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
  using Err = RedisContextError;

  /* Get all keys */
  Result<std::vector<KeyType>, Err> get_keys() const {
    using Ok = std::vector<KeyType>;

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

  /* Set an entry */
  void set(const KeyType &key, const ValueType &value) { map[key] = value; }

  /* Set an entry only if it doesn't exist */
  Result<uint8_t, Err> setnx(const KeyType &key, const ValueType &value) {
    using Ok = uint8_t;

    if (auto res = map.insert(std::make_pair(key, value)); res.second == true) {
      return Result<Ok, Err>::ok(200);

    } else {
      return Result<Ok, Err>::err(
          RedisContextError("Duplicate Key Found", 409));
    };
  }

  Result<uint8_t, Err> update(const KeyType &key, const ValueType &value) {
    using Ok = uint8_t;

    if (auto p = map.find(key); p != map.end()) {
      map[key] = value;
      return Result<Ok, Err>::ok(200);
    } else {
      return Result<Ok, Err>::err(RedisContextError("Not Found", 404));
    }
  }

  /* Get an entry */
  Result<std::pair<KeyType, ValueType>, Err> get(const KeyType &key) const {
    using Ok = std::pair<KeyType, ValueType>;

    if (auto p = map.find(key); p != map.end()) {
      return Result<Ok, Err>::ok(std::make_pair(p->first, p->second));
    } else {
      return Result<Ok, Err>::err(RedisContextError("Not Found", 404));
    }
  }

  /* Returns true if the entry exists and false if it doesn't */
  bool exists(const KeyType &key) const {
    if (auto p = map.find(key); p != map.end()) {
      return true;
    } else {
      return false;
    }
  }

  /* Delete an entry */
  Result<uint8_t, Err> del(const KeyType &key) {
    using Ok = uint8_t;

    if (auto p = map.erase(key); p == 1) {
      return Result<Ok, Err>::ok(200);
    } else {
      return Result<Ok, Err>::err(RedisContextError("Not Found", 404));
    }
  }

  /* Flush all data */
  void clear() { map.clear(); }

  /* Size of */
  size_t size() const { return map.size(); }

private:
  std::unordered_map<KeyType, ValueType> map;
};

#endif
