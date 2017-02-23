/**
 * Autogenerated by Thrift Compiler (0.9.3)
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */
#ifndef RunCmd_TYPES_H
#define RunCmd_TYPES_H

#include <iosfwd>

#include <thrift/Thrift.h>
#include <thrift/TApplicationException.h>
#include <thrift/protocol/TProtocol.h>
#include <thrift/transport/TTransport.h>

#include <thrift/cxxfunctional.h>
#include "AlgCommon_types.h"


namespace RunCmd {

class CmdResult;

typedef struct _CmdResult__isset {
  _CmdResult__isset() : output(false), retval(false) {}
  bool output :1;
  bool retval :1;
} _CmdResult__isset;

class CmdResult {
 public:

  CmdResult(const CmdResult&);
  CmdResult(CmdResult&&);
  CmdResult& operator=(const CmdResult&);
  CmdResult& operator=(CmdResult&&);
  CmdResult() : output(), retval(0) {
  }

  virtual ~CmdResult() throw();
  std::string output;
  int32_t retval;

  _CmdResult__isset __isset;

  void __set_output(const std::string& val);

  void __set_retval(const int32_t val);

  template <class Protocol_>
  uint32_t read(Protocol_* iprot);
  template <class Protocol_>
  uint32_t write(Protocol_* oprot) const;

  virtual void printTo(std::ostream& out) const;
};

void swap(CmdResult &a, CmdResult &b);

inline std::ostream& operator<<(std::ostream& out, const CmdResult& obj)
{
  obj.printTo(out);
  return out;
}

} // namespace

#include "RunCmd_types.tcc"

#endif
