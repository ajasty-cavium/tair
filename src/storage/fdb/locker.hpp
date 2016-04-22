/*
 * (C) 2007-2010 Alibaba Group Holding Limited
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * lock array support
 *
 * Version: $Id: locker.hpp 1279 2013-01-04 02:50:08Z dutor $
 *
 * Authors:
 *   ruohai <ruohai@taobao.com>
 *     - initial release
 *
 */
#ifndef TAIR_KDB_LOCKER_HPP
#define TAIR_KDB_LOCKER_HPP

#include <pthread.h>
#include "log.hpp"

namespace tair {
  namespace storage {
    namespace fdb {
      class locker {
      public:
        explicit locker(int bucket_number);
         ~locker();

        bool lock(int index, bool is_write = false);
        bool unlock(int index);

      private:
        void init();

      private:
          pthread_rwlock_t * b_locks;
        int bucket_number;
      };
    }
  }
}

#endif
