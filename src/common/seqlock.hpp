/*
 * (C) 2014- Alibaba Group Holding Limited
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This is a implementation of Sequential Lock.
 *
 * Typical pattern of using this lock:
 * Readers:
 *   do {
 *     int seq = seqlock.read_begin();
 *     // read operations
 *   } while (seqlock.read_retry(seq));
 * Writers:
 *   seqlock.write_begin();
 *   // write operations
 *   seqlock.write_end();
 *
 * For those scenes having single writer or having their own lock already,
 * the seqcount_t applies.
 *
 * Version: $Id: seqlock.hpp 2248 2014-04-14 10:56:22Z ganyu.hfl@taobao.com $
 *
 * Authors:
 *   dutor <ganyu.hfl@taobao.com>
 *     - initial release
 *
 */
#ifndef TAIR_SEQLOCK_HPP
#define TAIR_SEQLOCK_HPP

#if defined(__i386__) || defined(__x86_64__)
#include <pthread.h>

namespace tair
{

class seqlock_t
{
public:
  seqlock_t(int pshared = PTHREAD_PROCESS_PRIVATE) {
    pthread_spin_init(&lock_, pshared);
    seq_ = 0;
  }
  
  uint32_t read_begin() {
    return seq_;
  }

  bool read_retry(uint32_t seq) {
    return (seq & 1) | (seq_ ^ seq);
  }

  void write_begin() {
    pthread_spin_lock(&lock_);
    ++seq_;
    asm volatile("":::"memory");
  }

  void write_end() {
    asm volatile("":::"memory");
    ++seq_;
    pthread_spin_lock(&lock_);
  }

private:
  pthread_spinlock_t lock_;
  uint32_t seq_;
};

class seqcount_t
{
public:
  seqcount_t() {
    seq_ = 0;
  }

  uint32_t read_begin() {
    return seq_;
  }

  bool read_retry(uint32_t seq) {
    return (seq & 1) | (seq_ ^ seq);
  }

  void write_begin() {
    ++seq_;
    asm volatile("":::"memory");
  }

  void write_end() {
    asm volatile("":::"memory");
    ++seq_;
  }

private:
  uint32_t seq_;
};

}
#else // defined(__i386__) || defined(__x86_64__)
#error "tair::seqlock_t does not support this architecture!"
#endif // defined(__i386__) || defined(__x86_64__)

#endif
