#pragma once

#include <expected>
#include <string>
#include <string_view>

namespace disruptor {

// Error types for std::expected/unexpected
enum class ErrorCode {
  InsufficientCapacity,
  InvalidArgument,
  RuntimeError,
};

struct Error {
  ErrorCode code;
  std::string message;

  Error(ErrorCode c, std::string_view msg) : code(c), message(msg) {}

  // Convenience constructors
  static Error insufficient_capacity() {
    return Error(ErrorCode::InsufficientCapacity, "InsufficientCapacityException");
  }

  static Error invalid_argument(std::string_view msg) {
    return Error(ErrorCode::InvalidArgument, msg);
  }

  static Error runtime_error(std::string_view msg) {
    return Error(ErrorCode::RuntimeError, msg);
  }
};

}  // namespace disruptor
