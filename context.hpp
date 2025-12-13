#ifndef CONTEXT_HPP
#define CONTEXT_HPP

#include "./utils/utils.hpp"
#include <cstddef>
#include <cstdint>
#include <deque>
#include <string>
#include <type_traits>
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

struct Persistor {
  using Err = RedisContextError;

public:
  explicit Persistor() = default;
  ~Persistor() {
    historystream.close();
    logger.close();
    // TODO: Optimize logger file for values that are "expired"
  };

  void change_dirpath(std::string new_dirpath) {
    dirpath = new_dirpath;
    history = dirpath + "/history";
  }
  void change_dirname(std::string new_dirname) { dir_name = new_dirname; }

  std::string get_dirpath() const {
    return std::string(dirpath) + std::string(dir_name);
  }
  Result<int, Err> append_logger(std::string command) {
    using Ok = int;
    if (logger.fail()) {
      // TODO: create fallback to create a new logger file and see the name we
      // need to create based on `history` file
			
      // logger(dirpath + dir_name + logger_in_use);
      return Result<Ok, Err>::err(
          RedisContextError("Logger is bad", -1));
    }
    // TODO: Write to file the command
    return Result<Ok, Err>::ok(0);
  }

private:
  std::string logger_in_use = "snapshot";
  std::string dirpath = "~/Desktop/";
  std::string history = "/history";
  std::string dir_name = "MYRADIS_PERSISTOR";
  // std::ofstream historystream{};
  // std::ofstream logger{};
};
template <typename KeyType, typename ValueType> struct EntryList;
template <typename KeyType, typename ValueType> struct List;

template <typename KeyType, typename ValueType> class RedisContext {
public:
  RedisContext() = default;
  using Err = RedisContextError;

  /* Get all keys */
  Result<std::vector<KeyType>, Err> get_keys() const {
    using Ok = std::vector<KeyType>;

    if (map_.empty()) {
      return Result<Ok, Err>::err(RedisContextError("No Keys Found", 404));
    }

    std::vector<KeyType> vec;
    vec.reserve(map_.size());

    for (const auto &pair : map_) {
      vec.push_back(pair.first);
    }

    return Result<Ok, Err>::ok(vec);
  };

  /* Get all list keys */
  Result<std::vector<KeyType>, Err> get_lkeys() const {
    using Ok = std::vector<KeyType>;

    if (list_.empty()) {
      return Result<Ok, Err>::err(RedisContextError("No Keys Found", 404));
    }

    std::vector<KeyType> vec;
    vec.reserve(list_.size());
    for (const auto &entry : list_) {
      vec.push_back(entry.first);
    }

    return Result<Ok, Err>::ok(vec);
  }

  /* Set an entry */
  void set(const KeyType &key, const ValueType &value) { map_[key] = value; }

  /* Set an entry only if it doesn't exist */
  Result<uint8_t, Err> setnx(const KeyType &key, const ValueType &value) {
    using Ok = uint8_t;

    if (auto res = map_.insert(std::make_pair(key, value));
        res.second == true) {
      return Result<Ok, Err>::ok(200);

    } else {
      return Result<Ok, Err>::err(
          RedisContextError("Duplicate Key Found", 409));
    };
  }

  void lpush(const KeyType &key, const ValueType &value) {
    list_[key].push_front(value);
  }

  void rpush(const KeyType &key, const ValueType &value) {
    list_[key].push_back(value);
  }

  Result<ValueType, Err> lpop(const KeyType &key) {
    using Ok = ValueType;

    if (list_.empty()) {
      return Result<Ok, Err>::err(RedisContextError("Empty list", -1));
    }
    if (auto l = list_.find(key); l == list_.end()) {
      return Result<Ok, Err>::err(RedisContextError("List Not Found", -1));
    }

    ValueType val = list_[key].front();
    list_[key].pop_front();
    return Result<Ok, Err>::ok(val);
  }

  Result<ValueType, Err> rpop(const KeyType &key) {
    using Ok = ValueType;

    if (list_.empty()) {
      return Result<Ok, Err>::err(RedisContextError("Empty list", -1));
    }
    if (auto l = list_.find(key); l == list_.end()) {
      return Result<Ok, Err>::err(RedisContextError("List Not Found", -1));
    }

    ValueType val = list_[key].back();
    list_[key].pop_back();
    return Result<Ok, Err>::ok(val);
  }

  Result<uint8_t, Err> update(const KeyType &key, const ValueType &value) {
    using Ok = uint8_t;

    if (auto p = map_.find(key); p != map_.end()) {
      map_[key] = value;
      return Result<Ok, Err>::ok(200);
    } else {
      return Result<Ok, Err>::err(RedisContextError("Not Found", 404));
    }
  }

  /* Get an entry */
  Result<std::pair<KeyType, ValueType>, Err> get(const KeyType &key) const {
    using Ok = std::pair<KeyType, ValueType>;

    if (auto p = map_.find(key); p != map_.end()) {
      return Result<Ok, Err>::ok(std::make_pair(p->first, p->second));
    } else {
      return Result<Ok, Err>::err(RedisContextError("Not Found", 404));
    }
  }

  /* Returns true if the entry exists and false if it doesn't */
  bool exists(const KeyType &key) const {
    if (auto p = map_.find(key); p != map_.end()) {
      return true;
    } else {
      return false;
    }
  }

  bool lexists(const KeyType &key) const {
    if (auto l = list_.find(key); l != list_.end()) {
      return true;
    } else {
      return false;
    }
  }

  Result<ValueType, Err> incr(const KeyType &key) {
    using Ok = ValueType;

    if (auto p = map_.find(key); p != map_.end()) {
      using Value_t = std::remove_reference_t<decltype(p->second)>;

      if constexpr (std::is_integral_v<Value_t>) {
        map_[key] += 1;
        return Result<Ok, Err>::ok(map_[key]);
      } else if constexpr (std::is_same_v<Value_t, std::string>) {
        long val = std::stol(p->second);
        val += 1;
        map_[key] = std::to_string(val);
        return Result<Ok, Err>::ok(map_[key]);
      } else {
        return Result<Ok, Err>::err(
            RedisContextError("Value type isn't support to increment", -1));
      }
    } else {
      return Result<Ok, Err>::err(RedisContextError("Not Found", 404));
    }
  }

  Result<ValueType, Err> decr(const KeyType &key) {
    using Ok = ValueType;

    if (auto p = map_.find(key); p != map_.end()) {
      using Value_t = std::remove_reference_t<decltype(p->second)>;

      if constexpr (std::is_integral_v<Value_t>) {
        map_[key] -= 1;
        return Result<Ok, Err>::ok(map_[key]);
      } else if constexpr (std::is_same_v<Value_t, std::string>) {
        long val = std::stol(p->second);
        val -= 1;
        map_[key] = std::to_string(val);
        return Result<Ok, Err>::ok(map_[key]);
      } else {
        return Result<Ok, Err>::err(
            RedisContextError("Value type isn't support to increment", -1));
      }
    } else {
      return Result<Ok, Err>::err(RedisContextError("Not Found", 404));
    }
  }

  /* Delete an entry */
  Result<uint8_t, Err> del(const KeyType &key) {
    using Ok = uint8_t;

    if (auto p = map_.erase(key); p == 1) {
      return Result<Ok, Err>::ok(200);
    } else {
      return Result<Ok, Err>::err(RedisContextError("Not Found", 404));
    }
  }

  /* Flush all data */
  void clear_all() {
    map_.clear();
    list_.clear();
  }
  /* Flush data */
  void clear() { map_.clear(); }
  /* Flush deque data */
  void clear_list() { list_.clear(); }

  /* Number of entries */
  size_t size() const { return map_.size(); }
  /* Number of entries in deque */
  size_t size_list() const { return list_.size(); }

  /* Activate persistance (false by default) */
  int set_persistor(bool b) {
    usePersistor = b;
    return 200;
  }

  /* Change the path where the directory for the persistor will be made */
  int change_persistor_dirpath(std::string new_dirpath) {
    persistor_.change_dirpath(new_dirpath);
    return 200;
  }

  /* Change the name of the directory used by the persistor */
  int change_persistor_dirname(std::string new_dirname) {
    persistor_.change_dirname(new_dirname);
    return 200;
  }
  std::string get_persistor_path() const { return persistor_.get_dirpath(); }

private:
  std::unordered_map<KeyType, ValueType> map_;
  std::unordered_map<KeyType, std::deque<ValueType>> list_;
  Persistor persistor_;
  bool usePersistor = false;
};

#endif
