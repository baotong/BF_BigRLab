/**
 * Autogenerated by Thrift Compiler (0.9.3)
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */
#include "alg_mgr_types.h"

#include <algorithm>
#include <ostream>

#include <thrift/TToString.h>

namespace BigRLab {

int _kErrCodeTypeValues[] = {
  SUCCESS,
  ALREADY_EXIST,
  NO_SERVER
};
const char* _kErrCodeTypeNames[] = {
  "SUCCESS",
  "ALREADY_EXIST",
  "NO_SERVER"
};
const std::map<int, const char*> _ErrCodeType_VALUES_TO_NAMES(::apache::thrift::TEnumIterator(3, _kErrCodeTypeValues, _kErrCodeTypeNames), ::apache::thrift::TEnumIterator(-1, NULL, NULL));


InvalidRequest::~InvalidRequest() throw() {
}


void InvalidRequest::__set_desc(const std::string& val) {
  this->desc = val;
}

void InvalidRequest::__set_errno(const ErrCodeType val) {
  this->errno = val;
}

void swap(InvalidRequest &a, InvalidRequest &b) {
  using ::std::swap;
  swap(a.desc, b.desc);
  swap(a.errno, b.errno);
  swap(a.__isset, b.__isset);
}

InvalidRequest::InvalidRequest(const InvalidRequest& other1) : TException() {
  desc = other1.desc;
  errno = other1.errno;
  __isset = other1.__isset;
}
InvalidRequest::InvalidRequest( InvalidRequest&& other2) : TException() {
  desc = std::move(other2.desc);
  errno = std::move(other2.errno);
  __isset = std::move(other2.__isset);
}
InvalidRequest& InvalidRequest::operator=(const InvalidRequest& other3) {
  desc = other3.desc;
  errno = other3.errno;
  __isset = other3.__isset;
  return *this;
}
InvalidRequest& InvalidRequest::operator=(InvalidRequest&& other4) {
  desc = std::move(other4.desc);
  errno = std::move(other4.errno);
  __isset = std::move(other4.__isset);
  return *this;
}
void InvalidRequest::printTo(std::ostream& out) const {
  using ::apache::thrift::to_string;
  out << "InvalidRequest(";
  out << "desc=" << to_string(desc);
  out << ", " << "errno=" << to_string(errno);
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


AlgSvrInfo::~AlgSvrInfo() throw() {
}


void AlgSvrInfo::__set_addr(const std::string& val) {
  this->addr = val;
}

void AlgSvrInfo::__set_port(const int16_t val) {
  this->port = val;
}

void AlgSvrInfo::__set_nWorkThread(const int32_t val) {
  this->nWorkThread = val;
}

void swap(AlgSvrInfo &a, AlgSvrInfo &b) {
  using ::std::swap;
  swap(a.addr, b.addr);
  swap(a.port, b.port);
  swap(a.nWorkThread, b.nWorkThread);
  swap(a.__isset, b.__isset);
}

AlgSvrInfo::AlgSvrInfo(const AlgSvrInfo& other5) {
  addr = other5.addr;
  port = other5.port;
  nWorkThread = other5.nWorkThread;
  __isset = other5.__isset;
}
AlgSvrInfo::AlgSvrInfo( AlgSvrInfo&& other6) {
  addr = std::move(other6.addr);
  port = std::move(other6.port);
  nWorkThread = std::move(other6.nWorkThread);
  __isset = std::move(other6.__isset);
}
AlgSvrInfo& AlgSvrInfo::operator=(const AlgSvrInfo& other7) {
  addr = other7.addr;
  port = other7.port;
  nWorkThread = other7.nWorkThread;
  __isset = other7.__isset;
  return *this;
}
AlgSvrInfo& AlgSvrInfo::operator=(AlgSvrInfo&& other8) {
  addr = std::move(other8.addr);
  port = std::move(other8.port);
  nWorkThread = std::move(other8.nWorkThread);
  __isset = std::move(other8.__isset);
  return *this;
}
void AlgSvrInfo::printTo(std::ostream& out) const {
  using ::apache::thrift::to_string;
  out << "AlgSvrInfo(";
  out << "addr=" << to_string(addr);
  out << ", " << "port=" << to_string(port);
  out << ", " << "nWorkThread=" << to_string(nWorkThread);
  out << ")";
}

} // namespace
