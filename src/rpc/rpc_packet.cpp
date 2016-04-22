#include "rpc_packet.h"
#include <tbsys.h>
#include <byteswap.h>
#include <google/protobuf/message.h>
#include "rpc_command_factory.h" 

namespace tair
{
namespace rpc 
{

template<class T>inline void ReadType(easy_buf_t *buf, T *t);
inline void ReadBigEndian16(easy_buf_t *buf, int16_t *n);
inline void ReadBigEndian32(easy_buf_t *buf, int32_t *n);
inline bool ReadMessage(easy_buf_t *buf, ::google::protobuf::Message* message, int32_t len);
inline bool ReadMessage(easy_buf_t *buf, TairMessage* message, int32_t len);

template<class T>inline void WriteType(easy_buf_t *buf, const T t);
inline void WriteBigEndian32(easy_buf_t *buf, int32_t n);
inline void WriteBigEndian16(easy_buf_t *buf, int16_t n);

inline void WriteMessage(easy_buf_t *buf, TairMessage* message, int32_t len);
inline void WriteMessage(easy_buf_t *buf, ::google::protobuf::Message *message, int32_t len);

RpcPacket::RpcPacket()
    : body_(NULL), encoded_buffer_(NULL)
{
  bzero(&header_, sizeof(header_));
}

void RpcPacket::set_body(TairMessage *body) 
{ 
  this->tair_body_ 	 = body;
  this->header_.body_len = body->ByteSize();
}

void RpcPacket::set_body(::google::protobuf::Message *body) 
{ 
  this->body_ = body;
  this->header_.body_len = body->ByteSize();
}

RpcPacket::~RpcPacket() 
{
  assert(refcnt_ == 0);
  if (body_ != NULL)
    delete body_;
  if (encoded_buffer_ != NULL)
    delete[] encoded_buffer_;
  if (tair_body_ != NULL)
    delete tair_body_;
}

void RpcPacket::PreEncode()
{
  if(header_.magic_code == Header::MAGIC)
  {
    this->encoded_buffer_ = new char[header_.header_len + header_.body_len];
    easy_buf_t ebuf;
    ebuf.pos = encoded_buffer_; 
    EncodeHeader(&ebuf);
    WriteMessage(&ebuf, this->body_, header_.body_len);
  }
  else if(header_.magic_code == Header::TAIR_MAGIC)
  {
    this->encoded_buffer_ = new char[header_.header_len + header_.body_len];
    easy_buf_t ebuf;
    ebuf.pos = encoded_buffer_; 
    EncodeTairHeader(&ebuf);
    WriteMessage(&ebuf, this->tair_body_, header_.body_len);
  }
}

easy_buf_t* RpcPacket::EncodeTo(easy_pool_t *pool)
{
  easy_buf_t *buf = NULL;
  int32_t total_len = header_.header_len + header_.body_len;
  if (encoded_buffer_ != NULL)
  {
    buf = easy_buf_pack(pool, encoded_buffer_, total_len);    
  }
  else if(header_.magic_code == Header::MAGIC)
  {
    buf = easy_buf_create(pool, total_len);    
    if (buf == NULL) 
       return NULL;
    buf->last = buf->end; 
    char *mark_pos = buf->pos;
    EncodeHeader(buf);
    WriteMessage(buf, this->body_, header_.body_len);
    buf->pos = mark_pos;
  }
  else if(header_.magic_code == Header::TAIR_MAGIC)
  {
    buf = easy_buf_create(pool, total_len);    
    if (buf == NULL) 
       return NULL;
    buf->last = buf->end; 
    char *mark_pos = buf->pos;
    EncodeTairHeader(buf);
    WriteMessage(buf, this->tair_body_, header_.body_len);
    buf->pos = mark_pos;
  }
  else
  {
    // unreachable
  }
  return buf;
}

bool RpcPacket::DecodeFrom(int32_t magic, easy_buf_t *buf)
{
  assert(this->body_ == NULL);
  if (magic == RpcPacket::Header::MAGIC)
  {
    DecodeHeader(buf);
    this->body_ = RpcCommandFactory::NewPacketMessage(header_.packet_id);
    if (this->body_ != NULL)
    {
      if (ReadMessage(buf, this->body_, header_.body_len) == false)
      {
        delete this->body_;
        this->body_ = NULL;
      }
    }
  }
  else if (magic == RpcPacket::Header::TAIR_MAGIC)
  {
    DecodeTairHeader(buf);
    this->tair_body_ = RpcCommandFactory::NewPacketTairMessage(header_.packet_id);
    if (this->tair_body_ != NULL)
    {
      if (ReadMessage(buf, this->tair_body_, header_.body_len) == false)
      {
        delete this->tair_body_;
        this->tair_body_ = NULL;
      }
    }
  }
  return this->body_ != NULL || this->tair_body_ != NULL;
}

uint64_t RpcPacket::HandlePacketID(easy_connection_t *c, void *p)
{
  RpcPacket *packet = reinterpret_cast<RpcPacket *>(p);
  return packet->mutable_header().channel_seq;
}

int RpcPacket::HandlePacketEncode(easy_request_t *r, void *p)
{
  // 1. cast to MemcachedStatPacket
  RpcPacket *packet = reinterpret_cast<RpcPacket *>(p);
  // 2. encode to easy_buf 
  easy_buf_t *buf = packet->EncodeTo(r->ms->pool);
  if (buf == NULL)
  {
    TBSYS_LOG(ERROR, "encode error");
    return EASY_ERROR;
  }
  // 3. easy_request_addbuf(r, buf); 
  easy_request_addbuf(r, buf);
  return EASY_OK;
}

void* RpcPacket::HandlePacketDecode(easy_message_t *m)
{
  easy_buf_t *buf = m->input;
  int32_t buffer_len = buf->last - buf->pos;
  if (buffer_len < RpcPacket::Header::TAIR_LENGTH && buffer_len < RpcPacket::Header::LENGTH)
  {
    return NULL;
  }
  int32_t magic        = 0;
  int32_t body_len     = 0;
  int32_t total_len    = 0;

  char *mark_pos = buf->pos;
  ReadBigEndian32(buf, &magic);
  if (magic == RpcPacket::Header::MAGIC)
  {
    ReadBigEndian32(buf, &body_len);
    total_len = RpcPacket::Header::LENGTH + body_len;
  }
  else if (magic == RpcPacket::Header::TAIR_MAGIC)
  {
    int32_t temp = 0;
    ReadBigEndian32(buf, &temp); //Skip channel seq
    ReadBigEndian32(buf, &temp); //Skip packet id
    ReadBigEndian32(buf, &body_len);
    total_len = RpcPacket::Header::TAIR_LENGTH + body_len;
  }
  m->input->pos = mark_pos;

  RpcPacket *packet = NULL;
  if (body_len >= 1024 * 1024 * 100)
  {
    m->status = EASY_ERROR;
  }
  else if (buffer_len < total_len) 
  {
    m->next_read_len = total_len - buffer_len;
  }
  else
  {
    packet = new RpcPacket();
    packet->set_ref(1);
    if (packet->DecodeFrom(magic, buf) == false)
    {
      packet->set_ref(0);
      delete packet;
      packet = NULL;
      m->status = EASY_ERROR;
    }
  }
  return packet;
}

void RpcPacket::EncodeTairHeader(easy_buf_t *buf)
{
  WriteBigEndian32(buf, header_.magic_code);    //4
  WriteBigEndian32(buf, header_.channel_seq);   //4
  WriteBigEndian32(buf, header_.packet_id);     //4
  WriteBigEndian32(buf, header_.body_len);      //4
}

void RpcPacket::EncodeHeader(easy_buf_t *buf)
{
  WriteBigEndian32(buf, header_.magic_code);   //4
  WriteBigEndian32(buf, header_.body_len);     //4
  WriteBigEndian32(buf, header_.channel_seq);  //4
  WriteBigEndian32(buf, header_.index);        //4
  WriteBigEndian16(buf, header_.style);        //2
  WriteBigEndian16(buf, header_.packet_id);    //2
  WriteType(buf, header_.version);             //1
  WriteType(buf, header_.caller);              //1
  WriteBigEndian16(buf, header_.reserved1);    //2
}

void RpcPacket::DecodeTairHeader(easy_buf_t *buf)
{
  ReadBigEndian32(buf, &(header_.magic_code));
  ReadBigEndian32(buf, &(header_.channel_seq));
  int32_t packet_id = 0;
  ReadBigEndian32(buf, &packet_id);
  header_.packet_id = (int16_t)(packet_id);
  ReadBigEndian32(buf, &(header_.body_len));
  header_.header_len = RpcPacket::Header::TAIR_LENGTH;
}

void RpcPacket::DecodeHeader(easy_buf_t *buf)
{
  ReadBigEndian32(buf, &(header_.magic_code));
  ReadBigEndian32(buf, &(header_.body_len));
  ReadBigEndian32(buf, &(header_.channel_seq));
  ReadBigEndian32(buf, &(header_.index));
  ReadBigEndian16(buf, &(header_.style));
  ReadBigEndian16(buf, &(header_.packet_id));
  ReadType(buf, &(header_.version));
  ReadType(buf, &(header_.caller));
  ReadBigEndian16(buf, &(header_.reserved1));
  header_.header_len = RpcPacket::Header::LENGTH;
}

int RpcPacket::HandlePacketCleanup(easy_request_t *r, void *p)
{
  if (r != NULL) 
  {
    if (r->ipacket != NULL && r->ipacket != p)
    {
      reinterpret_cast<RpcPacket *>(r->ipacket)->UnRef();
      r->ipacket = NULL; 
    }
    if (r->opacket != NULL && r->opacket != p)
    {
      reinterpret_cast<RpcPacket *>(r->opacket)->UnRef();
      r->opacket = NULL; 
    }
  }
  if (p != NULL)
  {
    reinterpret_cast<RpcPacket *>(p)->UnRef();
  }
  return EASY_OK;
}

template<class T>
void WriteType(easy_buf_t *buf, const T t)
{
  memcpy(buf->pos, reinterpret_cast<const char *>(&t), sizeof(t));
  buf->pos += sizeof(t);
}

template<class T>
void ReadType(easy_buf_t *buf, T *t)
{
  *t = *reinterpret_cast<T *>(buf->pos);
  buf->pos += sizeof(T);                                                  
}

void ReadBigEndian16(easy_buf_t *buf, int16_t *n)
{
  ReadType(buf, n);
  *n = bswap_16(*n);
}

void ReadBigEndian32(easy_buf_t *buf, int32_t *n)
{
  ReadType(buf, n);
  *n = bswap_32(*n);
}

void WriteBigEndian32(easy_buf_t *buf, int32_t n)
{
  int32_t t = bswap_32(n);
  WriteType(buf, t);
}

void WriteBigEndian16(easy_buf_t *buf, int16_t n)
{
  int16_t t = bswap_16(n);
  WriteType(buf, t);
}

void WriteMessage(easy_buf_t *buf, TairMessage* message, int32_t len)
{
  message->SerializeToArray(buf->pos, len);
  buf->pos += len;
}

void WriteMessage(easy_buf_t *buf, ::google::protobuf::Message* message, int32_t len)
{
  message->SerializeToArray(buf->pos, len);
  buf->pos += len;
}

bool ReadMessage(easy_buf_t *buf, TairMessage* message, int32_t len)
{
  if (message->ParseFromArray(buf->pos, len) == true)
  {
    buf->pos += len;
    return true;
  }
  return false;
}

bool ReadMessage(easy_buf_t *buf, ::google::protobuf::Message* message, int32_t len)
{
  if (message->ParseFromArray(buf->pos, len) == true)
  {
    buf->pos += len;
    return true;
  }
  return false; 
}

}
}

