#pragma once
// 1:1 port of com.lmax.disruptor.ExceptionHandlers
// Source: reference/disruptor/src/main/java/com/lmax/disruptor/ExceptionHandlers.java

#include "ExceptionHandler.h"
#include "FatalExceptionHandler.h"

#include <memory>

namespace disruptor {

class ExceptionHandlers final {
public:
  ExceptionHandlers() = delete;

  // Java: public static ExceptionHandler<Object> defaultHandler()
  template <typename T>
  static std::shared_ptr<ExceptionHandler<T>> defaultHandler() {
    // Use a "leaked" pointer pattern to avoid static destruction order issues.
    // The handler is intentionally never deleted to ensure it outlives all threads.
    static std::shared_ptr<ExceptionHandler<T>>* handler = 
        new std::shared_ptr<ExceptionHandler<T>>(std::make_shared<FatalExceptionHandler<T>>());
    return *handler;
  }
};

} // namespace disruptor
