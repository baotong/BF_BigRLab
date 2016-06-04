/**
 * Autogenerated by Thrift Compiler (0.9.3)
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */
#include "wordseg_types.h"

#include <algorithm>
#include <ostream>

#include <thrift/TToString.h>

namespace WordSeg {


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


ResultItem::~ResultItem() throw() {
}


void ResultItem::__set_item(const std::string& val) {
  this->item = val;
}

void ResultItem::__set_tag(const std::string& val) {
  this->tag = val;
}

void swap(ResultItem &a, ResultItem &b) {
  using ::std::swap;
  swap(a.item, b.item);
  swap(a.tag, b.tag);
  swap(a.__isset, b.__isset);
}

ResultItem::ResultItem(const ResultItem& other4) {
  item = other4.item;
  tag = other4.tag;
  __isset = other4.__isset;
}
ResultItem::ResultItem( ResultItem&& other5) {
  item = std::move(other5.item);
  tag = std::move(other5.tag);
  __isset = std::move(other5.__isset);
}
ResultItem& ResultItem::operator=(const ResultItem& other6) {
  item = other6.item;
  tag = other6.tag;
  __isset = other6.__isset;
  return *this;
}
ResultItem& ResultItem::operator=(ResultItem&& other7) {
  item = std::move(other7.item);
  tag = std::move(other7.tag);
  __isset = std::move(other7.__isset);
  return *this;
}
void ResultItem::printTo(std::ostream& out) const {
  using ::apache::thrift::to_string;
  out << "ResultItem(";
  out << "item=" << to_string(item);
  out << ", " << "tag=" << to_string(tag);
  out << ")";
}

} // namespace
