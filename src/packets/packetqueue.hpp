/*
 * (C) 2007-2010 Taobao Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * packet queue, duplicated from tbnet
 *
 * Version: $Id$
 *
 * Authors:
 *   duolong <duolong@taobao.com>
 *
 */

#ifndef TAIR_PACKET_QUEUE_HPP
#define TAIR_PACKET_QUEUE_HPP
#include "packet.hpp"

namespace tair {
  class PacketQueue {
  public:
    PacketQueue();
    ~PacketQueue();
    Packet *pop();
    void clear();
    void push(Packet *packet);
    int size();
    bool empty();
    void move_to(PacketQueue *dest_queue);
    Packet *get_packet_list();
    Packet *head() {
      return _head;
    }
    Packet* tail() {
      return _tail;
    }
  protected:
    Packet *_head;
    Packet *_tail;
    int _size;
  };
}
#endif

