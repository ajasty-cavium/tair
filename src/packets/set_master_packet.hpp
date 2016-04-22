/*
 * (C) 2007-2010 Alibaba Group Holding Limited
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * this packet is for set one configserver as the master
 *
 * Version: $Id: set_master_packet.hpp 2640 2014-06-20 03:50:30Z mingmin.xmm@alibaba-inc.com $
 *
 * Authors:
 *   ruohai <ruohai@taobao.com>
 *     - initial release
 *
 */
#ifndef TAIR_PACKET_SET_MASTER_PACKET_H
#define TAIR_PACKET_SET_MASTER_PACKET_H
#include "base_packet.hpp"
#include "ping_packet.hpp"
namespace tair {
   class request_set_master : public request_ping {
   public:
      request_set_master() : request_ping()
      {
         setPCode(TAIR_REQ_SETMASTER_PACKET);
      }

      virtual base_packet::Type get_type() {
        return base_packet::REQ_SPECIAL;
      }
   private:
      request_set_master(const request_set_master&);
   };
}
#endif
