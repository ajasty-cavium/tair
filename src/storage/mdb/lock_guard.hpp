/*
 * (C) 2007-2013 Alibaba Group Holding Limited
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *
 * Version: $Id: lock_guard.hpp 1006 2013-06-29 01:08:04Z ganyu.hfl $
 *
 * Authors:
 *   GanYu <ganyu.hfl@taobao.com>
 *
 */
#ifndef TAIR_LOCK_GUARD_HPP
#define TAIR_LOCK_GUARD_HPP
#include <pthread.h>

namespace tair {

class lock_guard {
public:
  explicit lock_guard(pthread_mutex_t *lock) {
    this->lock = lock;
    pthread_mutex_lock(lock);
  }
  ~lock_guard() {
    pthread_mutex_unlock(lock);
  }
private:
  pthread_mutex_t *lock;
};

}

#endif
