/**
 * Autogenerated by Thrift Compiler (0.9.3)
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */
#include "topics_types.h"

#include <algorithm>
#include <ostream>

#include <thrift/TToString.h>

namespace Topics {


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


Result::~Result() throw() {
}


void Result::__set_topicId(const int64_t val) {
  this->topicId = val;
}

void Result::__set_possibility(const double val) {
  this->possibility = val;
}

void swap(Result &a, Result &b) {
  using ::std::swap;
  swap(a.topicId, b.topicId);
  swap(a.possibility, b.possibility);
  swap(a.__isset, b.__isset);
}

Result::Result(const Result& other4) {
  topicId = other4.topicId;
  possibility = other4.possibility;
  __isset = other4.__isset;
}
Result::Result( Result&& other5) {
  topicId = std::move(other5.topicId);
  possibility = std::move(other5.possibility);
  __isset = std::move(other5.__isset);
}
Result& Result::operator=(const Result& other6) {
  topicId = other6.topicId;
  possibility = other6.possibility;
  __isset = other6.__isset;
  return *this;
}
Result& Result::operator=(Result&& other7) {
  topicId = std::move(other7.topicId);
  possibility = std::move(other7.possibility);
  __isset = std::move(other7.__isset);
  return *this;
}
void Result::printTo(std::ostream& out) const {
  using ::apache::thrift::to_string;
  out << "Result(";
  out << "topicId=" << to_string(topicId);
  out << ", " << "possibility=" << to_string(possibility);
  out << ")";
}

} // namespace