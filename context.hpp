#ifndef CONTEXT_HPP
#define CONTEXT_HPP

#include "./utils/utils.hpp"
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <deque>
#include <filesystem>
#include <fstream>
#include <ios>
#include <stdexcept>
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
  explicit Persistor() {
    const char *home = std::getenv("HOME");
    if (!home)
      throw std::runtime_error("HOME not set");

    root_path = std::filesystem::path(home);
    init();
  };
  ~Persistor() {
    // TODO: Optimize logger file for values that are "expired"; Update History
    // file
    historystream.close();
    logger.close();
  }

  Result<int, Err> change_dirpath(std::string new_dirpath) {
    using Ok = int;

    if (new_dirpath.empty()) {
      return Result<Ok, Err>::err(
          RedisContextError("new dirpath is empty", -1));
    }
    if (auto s = new_dirpath.front(); s == *"/") {
      return Result<Ok, Err>::err(
          RedisContextError("dirpath must be relative", -1));
    }

    dirpath = new_dirpath;
    history = dirpath + "/history";
    init();

    return Result<Ok, Err>::ok(200);
  }
  void change_dirname(std::string name) {
    dir_name = name;
    init();
  }

  std::string get_dirpath() const {
    return (root_path / dirpath / dir_name).string();
  }

  Result<int, Err> append_logger(std::string command) {
    using Ok = int;
    if (!logger.is_open()) {
      init();
      if (!logger.is_open())
        return Result<Ok, Err>::err(
            RedisContextError("Failed to open Logger", -1));
    }

    logger << command << '\n';
    logger.flush();
    return Result<Ok, Err>::ok(0);
  }

private:
  void init() {
    base_path = root_path / dirpath / dir_name;
    std::filesystem::create_directories(base_path);

    logger.open(base_path / logger_in_use, std::ios::app);
  }

private:
  std::filesystem::path base_path;
  std::filesystem::path root_path;

  std::string logger_in_use = "snapshot";
  std::string dirpath = "Desktop";
  std::string history = "history";
  std::string dir_name = "MYRADIS_PERSISTOR";

  std::ofstream historystream;
  std::ofstream logger;
};

template <typename KeyType, typename ValueType> class RedisContext {
public:
  RedisContext() = default;
  ~RedisContext() {}
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
  Result<int, Err> set(const KeyType &key, const ValueType &value) {
    using Ok = int;
    map_[key] = value;

    if (usePersistor) {
      std::string cmd = "SET " + key + " " + value;
      auto res = persistor_.append_logger(cmd);
      if (res.is_err()) {
        RedisContextError err = res.unwrap_err();
        return Result<Ok, Err>::err(RedisContextError(err));
      }
    }

    return Result<Ok, Err>::ok(200);
  }

  /* Set an entry only if it doesn't exist */
  Result<uint8_t, Err> setnx(const KeyType &key, const ValueType &value) {
    using Ok = uint8_t;

    if (auto res = map_.insert(std::make_pair(key, value));
        res.second == true) {
      if (usePersistor) {
        std::string cmd = "SETNX " + key + " " + value;
        auto res = persistor_.append_logger(cmd);
        if (res.is_err()) {
          RedisContextError err = res.unwrap_err();
          return Result<Ok, Err>::err(RedisContextError(err));
        }
      }
      return Result<Ok, Err>::ok(200);

    } else {
      return Result<Ok, Err>::err(
          RedisContextError("Duplicate Key Found", 409));
    };
  }

  Result<int, Err> lpush(const KeyType &key, const ValueType &value) {
    using Ok = int;
    list_[key].push_front(value);

    if (usePersistor) {
      std::string cmd = "LPUSH " + key + " " + value;
      auto res = persistor_.append_logger(cmd);
      if (res.is_err()) {
        RedisContextError err = res.unwrap_err();
        return Result<Ok, Err>::err(RedisContextError(err));
      }
    }

    return Result<Ok, Err>::ok(200);
  }

  Result<int, Err> rpush(const KeyType &key, const ValueType &value) {
    using Ok = int;
    list_[key].push_back(value);

    if (usePersistor) {
      std::string cmd = "RPUSH " + key + " " + value;
      auto res = persistor_.append_logger(cmd);
      if (res.is_err()) {
        RedisContextError err = res.unwrap_err();
        return Result<Ok, Err>::err(RedisContextError(err));
      }
    }

    return Result<Ok, Err>::ok(200);
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

    if (usePersistor) {
      std::string cmd = "LPOP " + key + " " + val;
      auto res = persistor_.append_logger(cmd);
      if (res.is_err()) {
        RedisContextError err = res.unwrap_err();
        return Result<Ok, Err>::err(RedisContextError(err));
      }
    }

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

    if (usePersistor) {
      std::string cmd = "RPOP " + key + " " + val;
      auto res = persistor_.append_logger(cmd);
      if (res.is_err()) {
        RedisContextError err = res.unwrap_err();
        return Result<Ok, Err>::err(RedisContextError(err));
      }
    }
    return Result<Ok, Err>::ok(val);
  }

  Result<uint8_t, Err> update(const KeyType &key, const ValueType &value) {
    using Ok = uint8_t;

    if (auto p = map_.find(key); p != map_.end()) {
      map_[key] = value;

      if (usePersistor) {
        std::string cmd = "UPDATE " + key + " " + value;
        auto res = persistor_.append_logger(cmd);
        if (res.is_err()) {
          RedisContextError err = res.unwrap_err();
          return Result<Ok, Err>::err(RedisContextError(err));
        }
      }

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
      } else if constexpr (std::is_same_v<Value_t, std::string>) {
        long val = std::stol(p->second);
        val += 1;
        map_[key] = std::to_string(val);
      } else {
        return Result<Ok, Err>::err(
            RedisContextError("Value type isn't support to increment", -1));
      }

      if (usePersistor) {
        std::string cmd = "INCR " + key;
        auto res = persistor_.append_logger(cmd);
        if (res.is_err()) {
          RedisContextError err = res.unwrap_err();
          return Result<Ok, Err>::err(RedisContextError(err));
        }
      }

      return Result<Ok, Err>::ok(map_[key]);

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
      } else if constexpr (std::is_same_v<Value_t, std::string>) {
        long val = std::stol(p->second);
        val -= 1;
        map_[key] = std::to_string(val);
      } else {
        return Result<Ok, Err>::err(
            RedisContextError("Value type isn't support to increment", -1));
      }

      if (usePersistor) {
        std::string cmd = "DECR " + key;
        auto res = persistor_.append_logger(cmd);
        if (res.is_err()) {
          RedisContextError err = res.unwrap_err();
          return Result<Ok, Err>::err(RedisContextError(err));
        }
      }

      return Result<Ok, Err>::ok(map_[key]);
    } else {
      return Result<Ok, Err>::err(RedisContextError("Not Found", 404));
    }
  }

  /* Delete an entry */
  Result<uint8_t, Err> del(const KeyType &key) {
    using Ok = uint8_t;

    if (auto p = map_.erase(key); p == 1) {
      if (usePersistor) {
        std::string cmd = "DEL " + key;
        auto res = persistor_.append_logger(cmd);
        if (res.is_err()) {
          RedisContextError err = res.unwrap_err();
          return Result<Ok, Err>::err(RedisContextError(err));
        }
      }

      return Result<Ok, Err>::ok(200);
    } else {
      return Result<Ok, Err>::err(RedisContextError("Not Found", 404));
    }
  }

  /* Flush all data */
  void clear_all() {
    map_.clear();
    list_.clear();

    if (usePersistor) {
      std::string cmd = "CLEARALL";
      auto res = persistor_.append_logger(cmd);
      if (res.is_err()) {
        RedisContextError err = res.unwrap_err();
        // return Result<Ok, Err>::err(RedisContextError(err));
      }
    }
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

  int isusing_persistor() {
    if (usePersistor) {
      return true;
    } else {
      return false;
    }
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
