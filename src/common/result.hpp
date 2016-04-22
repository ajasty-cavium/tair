#ifndef COMMON_RESULT_HPP_
#define COMMON_RESULT_HPP_

namespace tair {
  namespace common {
    template<class T>
    class Result
    {
      public:
        Result(T value, int code) {
          value_ = value;
          code_  = code;
        }
      public:
        T   value_;
        int code_;
    };
  }
}

#endif
