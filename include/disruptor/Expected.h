#pragma once
// Lightweight std::expected<T, E> polyfill for C++20 compilers that lack C++23.
// Provides the subset used by this project: value construction, unexpected,
// has_value, value, error, operator*, operator bool.

#include <cassert>
#include <type_traits>
#include <utility>

namespace disruptor {

template <typename E>
class Unexpected {
public:
  explicit Unexpected(const E& e) : error_(e) {}
  explicit Unexpected(E&& e) : error_(std::move(e)) {}

  const E& error() const& { return error_; }
  E& error() & { return error_; }
  E&& error() && { return std::move(error_); }

private:
  E error_;
};

// Deduction guide
template <typename E>
Unexpected(E) -> Unexpected<E>;

template <typename T, typename E>
class Expected {
public:
  // Value constructors
  Expected(const T& val) : hasValue_(true) { new (&storage_.value) T(val); }
  Expected(T&& val) : hasValue_(true) { new (&storage_.value) T(std::move(val)); }

  // Unexpected constructor
  Expected(const Unexpected<E>& u) : hasValue_(false) { new (&storage_.error) E(u.error()); }
  Expected(Unexpected<E>&& u) : hasValue_(false) { new (&storage_.error) E(std::move(u).error()); }

  // Copy/move
  Expected(const Expected& o) : hasValue_(o.hasValue_) {
    if (hasValue_)
      new (&storage_.value) T(o.storage_.value);
    else
      new (&storage_.error) E(o.storage_.error);
  }

  Expected(Expected&& o) noexcept : hasValue_(o.hasValue_) {
    if (hasValue_)
      new (&storage_.value) T(std::move(o.storage_.value));
    else
      new (&storage_.error) E(std::move(o.storage_.error));
  }

  Expected& operator=(const Expected& o) {
    if (this != &o) {
      destroy();
      hasValue_ = o.hasValue_;
      if (hasValue_)
        new (&storage_.value) T(o.storage_.value);
      else
        new (&storage_.error) E(o.storage_.error);
    }
    return *this;
  }

  Expected& operator=(Expected&& o) noexcept {
    if (this != &o) {
      destroy();
      hasValue_ = o.hasValue_;
      if (hasValue_)
        new (&storage_.value) T(std::move(o.storage_.value));
      else
        new (&storage_.error) E(std::move(o.storage_.error));
    }
    return *this;
  }

  ~Expected() { destroy(); }

  bool has_value() const { return hasValue_; }
  explicit operator bool() const { return hasValue_; }

  T& value() & {
    assert(hasValue_);
    return storage_.value;
  }

  const T& value() const& {
    assert(hasValue_);
    return storage_.value;
  }

  T&& value() && {
    assert(hasValue_);
    return std::move(storage_.value);
  }

  E& error() & {
    assert(!hasValue_);
    return storage_.error;
  }

  const E& error() const& {
    assert(!hasValue_);
    return storage_.error;
  }

  T& operator*() & { return value(); }
  const T& operator*() const& { return value(); }
  T&& operator*() && { return std::move(value()); }

private:
  union Storage {
    T value;
    E error;
    Storage() {}
    ~Storage() {}
  } storage_;
  bool hasValue_;

  void destroy() {
    if (hasValue_)
      storage_.value.~T();
    else
      storage_.error.~E();
  }
};

}  // namespace disruptor
