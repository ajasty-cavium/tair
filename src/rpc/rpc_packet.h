#ifndef TAIR_RPC_PACKET_H_
#define TAIR_RPC_PACKET_H_

#include <easy_io.h>
#include <google/protobuf/message.h>
#include "reference_count.h"
#include "tair_message.h"

namespace tair
{
namespace rpc 
{

class RpcPacket : public ReferenceCount
{
 public:
  struct Header
  {
    const static int32_t MAGIC 	      = 0x74526D43;
    const static int32_t LENGTH = 24; 

    const static int32_t TAIR_MAGIC 	= 0x6D426454;
    const static int32_t TAIR_LENGTH  = 16;

    int32_t magic_code;
    int32_t body_len;     // not include header length
    int32_t channel_seq;  // channel seq for rpc
    int32_t index;        // for dispatch to thread

    int16_t style;        // body style
    int16_t packet_id;   // command id
    int8_t  version;       
    int8_t  caller;       
    int16_t reserved1;
    int32_t header_len;  // not encode
  };

  RpcPacket();
  ~RpcPacket();

  void PreEncode();

  easy_buf_t* EncodeTo(easy_pool_t *pool); 

  bool DecodeFrom(int32_t magic, easy_buf_t *buf); 

  Header& mutable_header() { return header_; }

  const Header& header() const { return header_; }

  static int      HandlePacketCleanup(easy_request_t *r, void *p);
  static void*    HandlePacketDecode(easy_message_t *m);
  static int      HandlePacketEncode(easy_request_t *r, void *p);
  static uint64_t HandlePacketID(easy_connection_t *c, void *packet); 
 
  inline const ::google::protobuf::Message* body() const { return body_; }
  inline ::google::protobuf::Message* mutable_body() { return body_; }

  inline const TairMessage* tair_body() const { return tair_body_; }

  void set_body(google::protobuf::Message *body);

  void set_body(TairMessage *body);

 private:
  inline void EncodeHeader(easy_buf_t *buf);
  inline void DecodeHeader(easy_buf_t *buf);

  inline void EncodeTairHeader(easy_buf_t *buf);
  inline void DecodeTairHeader(easy_buf_t *buf);

 private:
  Header header_;
  ::google::protobuf::Message*  body_;
  TairMessage*                  tair_body_;
  char*                         encoded_buffer_; 
};


}
}

#endif

