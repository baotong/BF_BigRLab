/**
 * Autogenerated by Thrift Compiler (0.9.3)
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */
#include "xgboost_svr_types.h"

#include <algorithm>
#include <ostream>

#include <thrift/TToString.h>

namespace XgBoostSvr {


InvalidRequest::~InvalidRequest() throw() {
}


void InvalidRequest::__set_reason(const std::string& val) {
  this->reason = val;
}

void swap(InvalidRequest &a, InvalidRequest &b) {
  using ::std::swap;
  swap(a.reason, b.reason);
  swap(a.__isset, b.__isset);
}

InvalidRequest::InvalidRequest(const InvalidRequest& other0) : TException() {
  reason = other0.reason;
  __isset = other0.__isset;
}
InvalidRequest::InvalidRequest( InvalidRequest&& other1) : TException() {
  reason = std::move(other1.reason);
  __isset = std::move(other1.__isset);
}
InvalidRequest& InvalidRequest::operator=(const InvalidRequest& other2) {
  reason = other2.reason;
  __isset = other2.__isset;
  return *this;
}
InvalidRequest& InvalidRequest::operator=(InvalidRequest&& other3) {
  reason = std::move(other3.reason);
  __isset = std::move(other3.__isset);
  return *this;
}
void InvalidRequest::printTo(std::ostream& out) const {
  using ::apache::thrift::to_string;
  out << "InvalidRequest(";
  out << "reason=" << to_string(reason);
  out << ")";
}

const char* InvalidRequest::what() const throw() {
  try {
    std::stringstream ss;
    ss << "TException - service has thrown: " << *this;
    this->thriftTExceptionMessageHolder_ = ss.str();
    return this->thriftTExceptionMessageHolder_.c_str();
  } catch (const std::exception&) {
    return "TException - service has thrown: InvalidRequest";
  }
}

} // namespace
