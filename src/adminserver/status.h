#ifndef ADMIN_STATUS_H_
#define ADMIN_STATUS_H_

#include <string>
#include <sstream>

namespace tair
{
namespace admin
{

class Status
{
 public:
  Status(const Status& rhs)
  {
    this->code_ = rhs.code_;
    this->mesg_ << rhs.mesg_.str();
    this->str_  = rhs.str_;
  }
  
  Status& operator=(const Status& rhs)
  {
    this->code_ = rhs.code_;
    this->mesg_.clear();
    this->mesg_ << rhs.mesg_.str();
    this->str_  = rhs.str_;
    return *this;
  }

  std::stringstream& mutable_message() { return mesg_; }

  std::string message() const { return mesg_.str(); }

  int code() const { return code_; }

  bool ok() { return code_ == kOk; }

  bool IsNotFound() { return code_ == kNotFound; }

  bool IsIllegalArgument() { return code_ == kIllegalArgument; }

  static Status OK() { return Status(kOk, "OK"); }

  static Status NotFound() { return Status(kNotFound, "NotFound"); }

  static Status IllegalArgument() { return Status(kIllegalArgument, "IllegalArgument"); }

  static Status Busy() { return Status(kBusy, "Busy"); }

  static Status NotReady() { return Status(kNotReady, "NotReady"); }

  static Status MysqlError() { return Status(kMysqlError, "MysqlError"); }

  static Status TairError() { return Status(kTairError, "TairError"); }

  static Status NotChanged() { return Status(kNotChanged, "NotChanged"); }

  const char *const str() const { return this->str_; }
 private:
  enum Code
  {
    kOk,
    kNotFound,
    kIllegalArgument,
    kNotChanged,
    kBusy,
    kMysqlError,
    kTairError,
    kNotReady
  };

  Status(Code code, const char *str) : code_(code), str_(str) {}

  std::stringstream mesg_;
  Code              code_;
  const char        *str_;
};



}
}

#endif
