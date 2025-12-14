# MY-REDIS

- A toy for fun implementation of Redis (Remote Dictionary Server) in C++.
- This project is purely educational and not intended for production use.

## Overview

### MY-REDIS is an in-memory key-value store supporting:

- String keys and values
- Lists (deque-style push/pop)
- Basic operations: `SET`, `GET`, `DEL`, `UPDATE`, `INCR`, `DECR`
- Conditional set (`SETNX`)
- Persistence logging to disk (optional)

It is implemented using modern C++ features like std::unordered_map, std::deque, and a Result<T, E> type for error handling.

## Features

- **Key-value operations:** store, retrieve, update, delete values.
- **List operations:** push/pop elements from front or back.
- **Persistence:** optionally log commands to a local file for replay.
- **Safe error handling:** operations return a Result type with error codes/messages

## API Example

```cpp
#include "context.hpp"
#include <iostream>

int main() {
    RedisContext<std::string, std::string> redis;
    redis.set_persistor(true);

    redis.set("name1", "Alice");
    redis.set("name2", "Bob");

    auto keys = redis.get_keys();
    if (keys.is_ok()) {
        for (auto &key : keys.unwrap())
            std::cout << key << '\n';
    }

    auto user = redis.get("name1");
    if (user.is_ok())
        std::cout << user.unwrap().second << '\n'; // prints "Alice"

    redis.del("name1"); // deletes key
}
```

## List Operations Example

```cpp
redis.lpush("mylist", "first");
redis.rpush("mylist", "second");

auto front = redis.lpop("mylist"); // "first"
auto back = redis.rpop("mylist");  // "second"
```

## Persistence

```cpp
redis.set_persistor(true);
redis.set("key", "value"); // automatically logs to a snapshot file
```

## Error Handling

- All operations return a `Result<T, RedisContextError>`:

```cpp
auto res = redis.get("missing_key");
if (res.is_err()) {
    res.cout_err();
}
```

# Build

```bash
make run
```

# Notes

- Educational for fun project.
- Persistence is simple logging, not a full durable database yet.
- No networking or Redis protocol support yet.
