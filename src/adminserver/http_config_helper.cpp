#include "http_config_helper.h"
#include <sstream>
#include <iostream>

namespace tair
{
namespace admin
{

const std::string HttpConfigHelper::kTidName    = "tid";
const std::string HttpConfigHelper::kCidName    = "cid";
const std::string HttpConfigHelper::kKeyName    = "key";
const std::string HttpConfigHelper::kValName    = "val";
const std::string HttpConfigHelper::kUUIDName   = "uuid";
const std::string HttpConfigHelper::kMasterName = "master";
const std::string HttpConfigHelper::kSlaveName  = "slave";
const std::string HttpConfigHelper::kGroupName  = "group";

const size_t HttpConfigHelper::kParameterLen = 1024;

void HttpConfigHelper::JsonFormatEnd(std::stringstream &ss,
                                     const std::string &method,
                                     const Status &status)
{
  ss << "\"Method\":\"" << method << "\",";
  ss << "\"StatusMessage\":\"" << status.message() << "\",";
  ss << "\"StatusString\":\"" << status.str()  << "\",";
  ss << "\"StatusCode\":" << status.code();
  ss << "}";
}

void HttpConfigHelper::JsonFormatInt(std::stringstream &ss,
                                     const std::map<std::string, int64_t> &params)
{
  std::map<std::string, int64_t>::const_iterator iter = params.begin();
  for (; iter != params.end(); ++iter)
  {
    ss << "\"" << iter->first << "\":";
    ss << "" << iter->second << ",";
  }
}

void HttpConfigHelper::JsonFormatString(std::stringstream &ss,
                                        const std::map<std::string, std::string> &params)
{
  std::map<std::string, std::string>::const_iterator iter = params.begin();
  for (; iter != params.end(); ++iter)
  {
    ss << "\"" << iter->first << "\":";
    ss << "\"" << iter->second << "\",";
  }
}

void HttpConfigHelper::JsonFormatBegin(std::stringstream &ss)
{
  ss << "{";
}

void HttpConfigHelper::OKRequest(struct mg_connection *conn, 
                                 const std::string &content)
{
  mg_printf(conn, "HTTP/1.0 200 OK\r\n"
      "Content-Length: %d\r\n"
      "Content-Type: application/json\r\n\r\n"
      "%s",
      content.length(), content.c_str());
}


void HttpConfigHelper::OKRequest(struct mg_connection *conn, 
                                 const std::string &method,
                                 const std::map<std::string, std::string> &params, 
                                 const std::map<std::string, int64_t> &int_params,
                                 const Status &status)
{
  std::stringstream ss;
  JsonFormatBegin(ss);
  JsonFormatString(ss, params);
  JsonFormatInt(ss, int_params);
  JsonFormatEnd(ss, method, status);

  std::string json = ss.str();
  mg_printf(conn, "HTTP/1.0 200 OK\r\n"
      "Content-Length: %d\r\n"
      "Content-Type: application/json\r\n\r\n"
      "%s",
      json.length(), json.c_str());
}

void HttpConfigHelper::ServiceUnavailable(struct mg_connection *conn)
{
  std::stringstream ss;
  ss << "{\"Message\":\"" << "not ready for service" << "\"";
  ss << ",\"Status\":\"" << Status::NotReady().code() << "\"";
  ss << "}";
  std::string json = ss.str();
  mg_printf(conn, "HTTP/1.0 400 Bad Request\r\n"
      "Content-Length: %d\r\n"
      "Content-Type: application/json\r\n\r\n"
      "%s",
      json.length(), json.c_str());
}

void HttpConfigHelper::BadRequest(struct mg_connection *conn, const Status &status)
{
  std::stringstream ss;
  ss << "{\"Message\":\"" << status.message() << "\"";
  ss << ",\"Status\":\"" << status.code() << "\"";
  ss << "}";
  std::string json = ss.str();
  mg_printf(conn, "HTTP/1.0 400 Bad Request\r\n"
      "Content-Length: %d\r\n"
      "Content-Type: application/json\r\n\r\n"
      "%s",
      json.length(), json.c_str());
}

Status HttpConfigHelper::GetParameterValue(struct mg_connection *conn, std::map<std::string, std::string> &params)
{
  char param_buffer[HttpConfigHelper::kParameterLen];  
  int  param_length = 0;
  Status status = Status::OK(); 

  std::map<std::string, std::string>::iterator iter = params.begin();
  for(; iter != params.end(); ++iter)
  {
    param_length = mg_get_var(conn, iter->first.c_str(), param_buffer, sizeof(param_buffer));
    if (param_length >= 0)
    {
      iter->second = std::string(param_buffer, param_length);
    }
    else if (param_length == -1)
    {
      status = Status::NotFound();
      status.mutable_message() << "parameter value of " << iter->first << " miss";
      break;
    }
    else if (param_length == -2)
    {
      status = Status::IllegalArgument();
      status.mutable_message() << "parameter value of " << iter->first << " too large";
      break;
    }
    else 
    {
      status = Status::IllegalArgument();
      status.mutable_message() << "parameter value of " << iter->first << " decode error";
      break;
    }
  }
  return status;
}

}
} 
