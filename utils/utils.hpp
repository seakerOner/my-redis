#pragma once
#include <utility>
#include <variant>

template <typename T, typename E> class Result {
public:
  using Ok = T;
  using Err = E;
  static Result ok(T value) { return Result(std::move(value)); }
  static Result err(E error) { return Result(std::move(error)); }

  bool is_ok() const { return std::holds_alternative<T>(data); }
  bool is_err() const { return std::holds_alternative<E>(data); }

  const T &unwrap() const & { return std::get<T>(data); }
  T &unwrap() & { return std::get<T>(data); }
  T unwrap() && { return std::move(std::get<T>(data)); }

  const E &unwrap_err() const & { return std::get<E>(data); }
  E &unwrap_err() &{ return std::get<E>(data); }
  E unwrap_err() && { return std::move(std::get<E>(data)); }

private:
  std::variant<T, E> data;

  explicit Result(T value) : data(std::move(value)) {}
  explicit Result(E error) : data(std::move(error)) {}
};
