#ifndef TAIR_MESSAGE_H_
#define TAIR_MESSAGE_H_

#include <string>
#include <byteswap.h>

namespace tair
{
namespace rpc
{

class TairMessage
{
 public:
  virtual ~TairMessage() {}

  virtual TairMessage* New() = 0;

  virtual bool ParseFromArray(char *buf, int32_t len) = 0;

  virtual void SerializeToArray(char *buf, int32_t len) = 0;

  virtual int32_t ByteSize() const = 0;

  virtual const std::string& GetTypeName() const = 0;

 protected:
  class MessageBuffer
  {
   public:
    MessageBuffer(char *buf, int32_t len)
        : buf_(buf), end_(buf + len)
    {
    }

    template<class T>
    bool WriteType(const T t)
    {
      int32_t tlen = sizeof(t);
      if (tlen > end_ - buf_)
        return false;
      memcpy(buf_, reinterpret_cast<const char *>(&t), tlen);
      buf_ += tlen;
      return true;
    }

    template<class T>
    bool ReadType(T &t)
    {
      int32_t tlen = sizeof(t);
      if (tlen > end_ - buf_)
        return false;
      t = *reinterpret_cast<T *>(buf_);
      buf_ += tlen;
      return true;
    }

    bool WriteLittleString(const std::string &str)
    {
      int16_t len = str.length();
      if (len + 2 > end_ - buf_)
      {
        return false;
      }
      WriteType(bswap_16(len));
      memcpy(buf_, str.c_str(), str.length());
      buf_ += len;
      return true;
    }

    bool ReadString(std::string &str)
    {
      int32_t len = 0;
      if (ReadType(len) == false)
        return false;
      len = bswap_32(len);
      if (len < 0 || len > end_ - buf_)
      {
        return false;
      }
      str.assign(buf_, len);
      buf_ += len;
      return true;
    }

    bool ReadLittleString(std::string &str)
    {
      int16_t len = 0;
      if (ReadType(len) == false)
        return false;
      len = bswap_16(len);
      if (len < 0 || len > end_ - buf_)
      {
        return false;
      }
      str.assign(buf_, len);
      buf_ += len;
      return true;
    }

    bool SkipBytes(int32_t len)
    {
      if (len > end_ - buf_)
      {
        return false;
      }
      buf_ += len;
      return true;
    }

   private:
    char    *buf_;
    char    *end_;
  };

  template<class T>
  void WriteType(void *buf, const T t)
  {
    memcpy(buf, reinterpret_cast<const char *>(&t), sizeof(t));
  }

  template<class T>
  void ReadType(char *buf, T &t)
  {
    t = *reinterpret_cast<T *>(buf);
  }
};

}
}

#endif
