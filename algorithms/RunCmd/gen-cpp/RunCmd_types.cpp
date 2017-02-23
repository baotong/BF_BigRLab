/**
 * Autogenerated by Thrift Compiler (0.9.3)
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */
#include "RunCmd_types.h"

#include <algorithm>
#include <ostream>

#include <thrift/TToString.h>

namespace RunCmd {


CmdResult::~CmdResult() throw() {
}


void CmdResult::__set_output(const std::string& val) {
  this->output = val;
}

void CmdResult::__set_retval(const int32_t val) {
  this->retval = val;
}

void swap(CmdResult &a, CmdResult &b) {
  using ::std::swap;
  swap(a.output, b.output);
  swap(a.retval, b.retval);
  swap(a.__isset, b.__isset);
}

CmdResult::CmdResult(const CmdResult& other0) {
  output = other0.output;
  retval = other0.retval;
  __isset = other0.__isset;
}
CmdResult::CmdResult( CmdResult&& other1) {
  output = std::move(other1.output);
  retval = std::move(other1.retval);
  __isset = std::move(other1.__isset);
}
CmdResult& CmdResult::operator=(const CmdResult& other2) {
  output = other2.output;
  retval = other2.retval;
  __isset = other2.__isset;
  return *this;
}
CmdResult& CmdResult::operator=(CmdResult&& other3) {
  output = std::move(other3.output);
  retval = std::move(other3.retval);
  __isset = std::move(other3.__isset);
  return *this;
}
void CmdResult::printTo(std::ostream& out) const {
  using ::apache::thrift::to_string;
  out << "CmdResult(";
  out << "output=" << to_string(output);
  out << ", " << "retval=" << to_string(retval);
  out << ")";
}

} // namespace
