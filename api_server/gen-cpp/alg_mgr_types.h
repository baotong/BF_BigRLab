/**
 * Autogenerated by Thrift Compiler (0.9.3)
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */
#ifndef alg_mgr_TYPES_H
#define alg_mgr_TYPES_H

#include <iosfwd>

#include <thrift/Thrift.h>
#include <thrift/TApplicationException.h>
#include <thrift/protocol/TProtocol.h>
#include <thrift/transport/TTransport.h>

#include <thrift/cxxfunctional.h>


namespace BigRLab {

enum ErrCodeType {
  SUCCESS = 0,
  ALREADY_EXIST = 1,
  SERVER_UNREACHABLE = 2,
  NO_SERVICE = 3,
  INTERNAL_FAIL = 4
};

extern const std::map<int, const char*> _ErrCodeType_VALUES_TO_NAMES;

class InvalidRequest;

class AlgSvrInfo;

typedef struct _InvalidRequest__isset {
  _InvalidRequest__isset() : desc(false), errCode(false) {}
  bool desc :1;
  bool errCode :1;
} _InvalidRequest__isset;

class InvalidRequest : public ::apache::thrift::TException {
 public:

  InvalidRequest(const InvalidRequest&);
  InvalidRequest(InvalidRequest&&);
  InvalidRequest& operator=(const InvalidRequest&);
  InvalidRequest& operator=(InvalidRequest&&);
  InvalidRequest() : desc(), errCode((ErrCodeType)0) {
  }

  virtual ~InvalidRequest() throw();
  std::string desc;
  ErrCodeType errCode;

  _InvalidRequest__isset __isset;

  void __set_desc(const std::string& val);

  void __set_errCode(const ErrCodeType val);

  template <class Protocol_>
  uint32_t read(Protocol_* iprot);
  template <class Protocol_>
  uint32_t write(Protocol_* oprot) const;

  virtual void printTo(std::ostream& out) const;
  mutable std::string thriftTExceptionMessageHolder_;
  const char* what() const throw();
};

void swap(InvalidRequest &a, InvalidRequest &b);

inline std::ostream& operator<<(std::ostream& out, const InvalidRequest& obj)
{
  obj.printTo(out);
  return out;
}

typedef struct _AlgSvrInfo__isset {
  _AlgSvrInfo__isset() : addr(false), port(false), maxConcurrency(false), serviceName(false) {}
  bool addr :1;
  bool port :1;
  bool maxConcurrency :1;
  bool serviceName :1;
} _AlgSvrInfo__isset;

class AlgSvrInfo {
 public:

  AlgSvrInfo(const AlgSvrInfo&);
  AlgSvrInfo(AlgSvrInfo&&);
  AlgSvrInfo& operator=(const AlgSvrInfo&);
  AlgSvrInfo& operator=(AlgSvrInfo&&);
  AlgSvrInfo() : addr(), port(0), maxConcurrency(0), serviceName() {
  }

  virtual ~AlgSvrInfo() throw();
  std::string addr;
  int16_t port;
  int32_t maxConcurrency;
  std::string serviceName;

  _AlgSvrInfo__isset __isset;

  void __set_addr(const std::string& val);

  void __set_port(const int16_t val);

  void __set_maxConcurrency(const int32_t val);

  void __set_serviceName(const std::string& val);

  template <class Protocol_>
  uint32_t read(Protocol_* iprot);
  template <class Protocol_>
  uint32_t write(Protocol_* oprot) const;

  virtual void printTo(std::ostream& out) const;
};

void swap(AlgSvrInfo &a, AlgSvrInfo &b);

inline std::ostream& operator<<(std::ostream& out, const AlgSvrInfo& obj)
{
  obj.printTo(out);
  return out;
}

} // namespace

#include "alg_mgr_types.tcc"

#endif
