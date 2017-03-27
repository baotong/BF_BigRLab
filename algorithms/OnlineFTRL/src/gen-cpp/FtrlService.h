/**
 * Autogenerated by Thrift Compiler (0.9.3)
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */
#ifndef FtrlService_H
#define FtrlService_H

#include <thrift/TDispatchProcessor.h>
#include <thrift/async/TConcurrentClientSyncInfo.h>
#include "ftrl_types.h"

namespace FTRL {

#ifdef _WIN32
  #pragma warning( push )
  #pragma warning (disable : 4250 ) //inheriting methods via dominance 
#endif

class FtrlServiceIf {
 public:
  virtual ~FtrlServiceIf() {}
  virtual double lrPredict(const std::string& id, const std::string& data) = 0;
  virtual bool setValue(const std::string& id, const double value) = 0;
  virtual void handleRequest(std::string& _return, const std::string& request) = 0;
};

class FtrlServiceIfFactory {
 public:
  typedef FtrlServiceIf Handler;

  virtual ~FtrlServiceIfFactory() {}

  virtual FtrlServiceIf* getHandler(const ::apache::thrift::TConnectionInfo& connInfo) = 0;
  virtual void releaseHandler(FtrlServiceIf* /* handler */) = 0;
};

class FtrlServiceIfSingletonFactory : virtual public FtrlServiceIfFactory {
 public:
  FtrlServiceIfSingletonFactory(const boost::shared_ptr<FtrlServiceIf>& iface) : iface_(iface) {}
  virtual ~FtrlServiceIfSingletonFactory() {}

  virtual FtrlServiceIf* getHandler(const ::apache::thrift::TConnectionInfo&) {
    return iface_.get();
  }
  virtual void releaseHandler(FtrlServiceIf* /* handler */) {}

 protected:
  boost::shared_ptr<FtrlServiceIf> iface_;
};

class FtrlServiceNull : virtual public FtrlServiceIf {
 public:
  virtual ~FtrlServiceNull() {}
  double lrPredict(const std::string& /* id */, const std::string& /* data */) {
    double _return = (double)0;
    return _return;
  }
  bool setValue(const std::string& /* id */, const double /* value */) {
    bool _return = false;
    return _return;
  }
  void handleRequest(std::string& /* _return */, const std::string& /* request */) {
    return;
  }
};

typedef struct _FtrlService_lrPredict_args__isset {
  _FtrlService_lrPredict_args__isset() : id(false), data(false) {}
  bool id :1;
  bool data :1;
} _FtrlService_lrPredict_args__isset;

class FtrlService_lrPredict_args {
 public:

  FtrlService_lrPredict_args(const FtrlService_lrPredict_args&);
  FtrlService_lrPredict_args(FtrlService_lrPredict_args&&);
  FtrlService_lrPredict_args& operator=(const FtrlService_lrPredict_args&);
  FtrlService_lrPredict_args& operator=(FtrlService_lrPredict_args&&);
  FtrlService_lrPredict_args() : id(), data() {
  }

  virtual ~FtrlService_lrPredict_args() throw();
  std::string id;
  std::string data;

  _FtrlService_lrPredict_args__isset __isset;

  void __set_id(const std::string& val);

  void __set_data(const std::string& val);

  template <class Protocol_>
  uint32_t read(Protocol_* iprot);
  template <class Protocol_>
  uint32_t write(Protocol_* oprot) const;

};


class FtrlService_lrPredict_pargs {
 public:


  virtual ~FtrlService_lrPredict_pargs() throw();
  const std::string* id;
  const std::string* data;

  template <class Protocol_>
  uint32_t write(Protocol_* oprot) const;

};

typedef struct _FtrlService_lrPredict_result__isset {
  _FtrlService_lrPredict_result__isset() : success(false), err(false) {}
  bool success :1;
  bool err :1;
} _FtrlService_lrPredict_result__isset;

class FtrlService_lrPredict_result {
 public:

  FtrlService_lrPredict_result(const FtrlService_lrPredict_result&);
  FtrlService_lrPredict_result(FtrlService_lrPredict_result&&);
  FtrlService_lrPredict_result& operator=(const FtrlService_lrPredict_result&);
  FtrlService_lrPredict_result& operator=(FtrlService_lrPredict_result&&);
  FtrlService_lrPredict_result() : success(0) {
  }

  virtual ~FtrlService_lrPredict_result() throw();
  double success;
   ::AlgCommon::InvalidRequest err;

  _FtrlService_lrPredict_result__isset __isset;

  void __set_success(const double val);

  void __set_err(const  ::AlgCommon::InvalidRequest& val);

  template <class Protocol_>
  uint32_t read(Protocol_* iprot);
  template <class Protocol_>
  uint32_t write(Protocol_* oprot) const;

};

typedef struct _FtrlService_lrPredict_presult__isset {
  _FtrlService_lrPredict_presult__isset() : success(false), err(false) {}
  bool success :1;
  bool err :1;
} _FtrlService_lrPredict_presult__isset;

class FtrlService_lrPredict_presult {
 public:


  virtual ~FtrlService_lrPredict_presult() throw();
  double* success;
   ::AlgCommon::InvalidRequest err;

  _FtrlService_lrPredict_presult__isset __isset;

  template <class Protocol_>
  uint32_t read(Protocol_* iprot);

};

typedef struct _FtrlService_setValue_args__isset {
  _FtrlService_setValue_args__isset() : id(false), value(false) {}
  bool id :1;
  bool value :1;
} _FtrlService_setValue_args__isset;

class FtrlService_setValue_args {
 public:

  FtrlService_setValue_args(const FtrlService_setValue_args&);
  FtrlService_setValue_args(FtrlService_setValue_args&&);
  FtrlService_setValue_args& operator=(const FtrlService_setValue_args&);
  FtrlService_setValue_args& operator=(FtrlService_setValue_args&&);
  FtrlService_setValue_args() : id(), value(0) {
  }

  virtual ~FtrlService_setValue_args() throw();
  std::string id;
  double value;

  _FtrlService_setValue_args__isset __isset;

  void __set_id(const std::string& val);

  void __set_value(const double val);

  template <class Protocol_>
  uint32_t read(Protocol_* iprot);
  template <class Protocol_>
  uint32_t write(Protocol_* oprot) const;

};


class FtrlService_setValue_pargs {
 public:


  virtual ~FtrlService_setValue_pargs() throw();
  const std::string* id;
  const double* value;

  template <class Protocol_>
  uint32_t write(Protocol_* oprot) const;

};

typedef struct _FtrlService_setValue_result__isset {
  _FtrlService_setValue_result__isset() : success(false), err(false) {}
  bool success :1;
  bool err :1;
} _FtrlService_setValue_result__isset;

class FtrlService_setValue_result {
 public:

  FtrlService_setValue_result(const FtrlService_setValue_result&);
  FtrlService_setValue_result(FtrlService_setValue_result&&);
  FtrlService_setValue_result& operator=(const FtrlService_setValue_result&);
  FtrlService_setValue_result& operator=(FtrlService_setValue_result&&);
  FtrlService_setValue_result() : success(0) {
  }

  virtual ~FtrlService_setValue_result() throw();
  bool success;
   ::AlgCommon::InvalidRequest err;

  _FtrlService_setValue_result__isset __isset;

  void __set_success(const bool val);

  void __set_err(const  ::AlgCommon::InvalidRequest& val);

  template <class Protocol_>
  uint32_t read(Protocol_* iprot);
  template <class Protocol_>
  uint32_t write(Protocol_* oprot) const;

};

typedef struct _FtrlService_setValue_presult__isset {
  _FtrlService_setValue_presult__isset() : success(false), err(false) {}
  bool success :1;
  bool err :1;
} _FtrlService_setValue_presult__isset;

class FtrlService_setValue_presult {
 public:


  virtual ~FtrlService_setValue_presult() throw();
  bool* success;
   ::AlgCommon::InvalidRequest err;

  _FtrlService_setValue_presult__isset __isset;

  template <class Protocol_>
  uint32_t read(Protocol_* iprot);

};

typedef struct _FtrlService_handleRequest_args__isset {
  _FtrlService_handleRequest_args__isset() : request(false) {}
  bool request :1;
} _FtrlService_handleRequest_args__isset;

class FtrlService_handleRequest_args {
 public:

  FtrlService_handleRequest_args(const FtrlService_handleRequest_args&);
  FtrlService_handleRequest_args(FtrlService_handleRequest_args&&);
  FtrlService_handleRequest_args& operator=(const FtrlService_handleRequest_args&);
  FtrlService_handleRequest_args& operator=(FtrlService_handleRequest_args&&);
  FtrlService_handleRequest_args() : request() {
  }

  virtual ~FtrlService_handleRequest_args() throw();
  std::string request;

  _FtrlService_handleRequest_args__isset __isset;

  void __set_request(const std::string& val);

  template <class Protocol_>
  uint32_t read(Protocol_* iprot);
  template <class Protocol_>
  uint32_t write(Protocol_* oprot) const;

};


class FtrlService_handleRequest_pargs {
 public:


  virtual ~FtrlService_handleRequest_pargs() throw();
  const std::string* request;

  template <class Protocol_>
  uint32_t write(Protocol_* oprot) const;

};

typedef struct _FtrlService_handleRequest_result__isset {
  _FtrlService_handleRequest_result__isset() : success(false), err(false) {}
  bool success :1;
  bool err :1;
} _FtrlService_handleRequest_result__isset;

class FtrlService_handleRequest_result {
 public:

  FtrlService_handleRequest_result(const FtrlService_handleRequest_result&);
  FtrlService_handleRequest_result(FtrlService_handleRequest_result&&);
  FtrlService_handleRequest_result& operator=(const FtrlService_handleRequest_result&);
  FtrlService_handleRequest_result& operator=(FtrlService_handleRequest_result&&);
  FtrlService_handleRequest_result() : success() {
  }

  virtual ~FtrlService_handleRequest_result() throw();
  std::string success;
   ::AlgCommon::InvalidRequest err;

  _FtrlService_handleRequest_result__isset __isset;

  void __set_success(const std::string& val);

  void __set_err(const  ::AlgCommon::InvalidRequest& val);

  template <class Protocol_>
  uint32_t read(Protocol_* iprot);
  template <class Protocol_>
  uint32_t write(Protocol_* oprot) const;

};

typedef struct _FtrlService_handleRequest_presult__isset {
  _FtrlService_handleRequest_presult__isset() : success(false), err(false) {}
  bool success :1;
  bool err :1;
} _FtrlService_handleRequest_presult__isset;

class FtrlService_handleRequest_presult {
 public:


  virtual ~FtrlService_handleRequest_presult() throw();
  std::string* success;
   ::AlgCommon::InvalidRequest err;

  _FtrlService_handleRequest_presult__isset __isset;

  template <class Protocol_>
  uint32_t read(Protocol_* iprot);

};

template <class Protocol_>
class FtrlServiceClientT : virtual public FtrlServiceIf {
 public:
  FtrlServiceClientT(boost::shared_ptr< Protocol_> prot) {
    setProtocolT(prot);
  }
  FtrlServiceClientT(boost::shared_ptr< Protocol_> iprot, boost::shared_ptr< Protocol_> oprot) {
    setProtocolT(iprot,oprot);
  }
 private:
  void setProtocolT(boost::shared_ptr< Protocol_> prot) {
  setProtocolT(prot,prot);
  }
  void setProtocolT(boost::shared_ptr< Protocol_> iprot, boost::shared_ptr< Protocol_> oprot) {
    piprot_=iprot;
    poprot_=oprot;
    iprot_ = iprot.get();
    oprot_ = oprot.get();
  }
 public:
  boost::shared_ptr< ::apache::thrift::protocol::TProtocol> getInputProtocol() {
    return this->piprot_;
  }
  boost::shared_ptr< ::apache::thrift::protocol::TProtocol> getOutputProtocol() {
    return this->poprot_;
  }
  double lrPredict(const std::string& id, const std::string& data);
  void send_lrPredict(const std::string& id, const std::string& data);
  double recv_lrPredict();
  bool setValue(const std::string& id, const double value);
  void send_setValue(const std::string& id, const double value);
  bool recv_setValue();
  void handleRequest(std::string& _return, const std::string& request);
  void send_handleRequest(const std::string& request);
  void recv_handleRequest(std::string& _return);
 protected:
  boost::shared_ptr< Protocol_> piprot_;
  boost::shared_ptr< Protocol_> poprot_;
  Protocol_* iprot_;
  Protocol_* oprot_;
};

typedef FtrlServiceClientT< ::apache::thrift::protocol::TProtocol> FtrlServiceClient;

template <class Protocol_>
class FtrlServiceProcessorT : public ::apache::thrift::TDispatchProcessorT<Protocol_> {
 protected:
  boost::shared_ptr<FtrlServiceIf> iface_;
  virtual bool dispatchCall(::apache::thrift::protocol::TProtocol* iprot, ::apache::thrift::protocol::TProtocol* oprot, const std::string& fname, int32_t seqid, void* callContext);
  virtual bool dispatchCallTemplated(Protocol_* iprot, Protocol_* oprot, const std::string& fname, int32_t seqid, void* callContext);
 private:
  typedef  void (FtrlServiceProcessorT::*ProcessFunction)(int32_t, ::apache::thrift::protocol::TProtocol*, ::apache::thrift::protocol::TProtocol*, void*);
  typedef void (FtrlServiceProcessorT::*SpecializedProcessFunction)(int32_t, Protocol_*, Protocol_*, void*);
  struct ProcessFunctions {
    ProcessFunction generic;
    SpecializedProcessFunction specialized;
    ProcessFunctions(ProcessFunction g, SpecializedProcessFunction s) :
      generic(g),
      specialized(s) {}
    ProcessFunctions() : generic(NULL), specialized(NULL) {}
  };
  typedef std::map<std::string, ProcessFunctions> ProcessMap;
  ProcessMap processMap_;
  void process_lrPredict(int32_t seqid, ::apache::thrift::protocol::TProtocol* iprot, ::apache::thrift::protocol::TProtocol* oprot, void* callContext);
  void process_lrPredict(int32_t seqid, Protocol_* iprot, Protocol_* oprot, void* callContext);
  void process_setValue(int32_t seqid, ::apache::thrift::protocol::TProtocol* iprot, ::apache::thrift::protocol::TProtocol* oprot, void* callContext);
  void process_setValue(int32_t seqid, Protocol_* iprot, Protocol_* oprot, void* callContext);
  void process_handleRequest(int32_t seqid, ::apache::thrift::protocol::TProtocol* iprot, ::apache::thrift::protocol::TProtocol* oprot, void* callContext);
  void process_handleRequest(int32_t seqid, Protocol_* iprot, Protocol_* oprot, void* callContext);
 public:
  FtrlServiceProcessorT(boost::shared_ptr<FtrlServiceIf> iface) :
    iface_(iface) {
    processMap_["lrPredict"] = ProcessFunctions(
      &FtrlServiceProcessorT::process_lrPredict,
      &FtrlServiceProcessorT::process_lrPredict);
    processMap_["setValue"] = ProcessFunctions(
      &FtrlServiceProcessorT::process_setValue,
      &FtrlServiceProcessorT::process_setValue);
    processMap_["handleRequest"] = ProcessFunctions(
      &FtrlServiceProcessorT::process_handleRequest,
      &FtrlServiceProcessorT::process_handleRequest);
  }

  virtual ~FtrlServiceProcessorT() {}
};

typedef FtrlServiceProcessorT< ::apache::thrift::protocol::TDummyProtocol > FtrlServiceProcessor;

template <class Protocol_>
class FtrlServiceProcessorFactoryT : public ::apache::thrift::TProcessorFactory {
 public:
  FtrlServiceProcessorFactoryT(const ::boost::shared_ptr< FtrlServiceIfFactory >& handlerFactory) :
      handlerFactory_(handlerFactory) {}

  ::boost::shared_ptr< ::apache::thrift::TProcessor > getProcessor(const ::apache::thrift::TConnectionInfo& connInfo);

 protected:
  ::boost::shared_ptr< FtrlServiceIfFactory > handlerFactory_;
};

typedef FtrlServiceProcessorFactoryT< ::apache::thrift::protocol::TDummyProtocol > FtrlServiceProcessorFactory;

class FtrlServiceMultiface : virtual public FtrlServiceIf {
 public:
  FtrlServiceMultiface(std::vector<boost::shared_ptr<FtrlServiceIf> >& ifaces) : ifaces_(ifaces) {
  }
  virtual ~FtrlServiceMultiface() {}
 protected:
  std::vector<boost::shared_ptr<FtrlServiceIf> > ifaces_;
  FtrlServiceMultiface() {}
  void add(boost::shared_ptr<FtrlServiceIf> iface) {
    ifaces_.push_back(iface);
  }
 public:
  double lrPredict(const std::string& id, const std::string& data) {
    size_t sz = ifaces_.size();
    size_t i = 0;
    for (; i < (sz - 1); ++i) {
      ifaces_[i]->lrPredict(id, data);
    }
    return ifaces_[i]->lrPredict(id, data);
  }

  bool setValue(const std::string& id, const double value) {
    size_t sz = ifaces_.size();
    size_t i = 0;
    for (; i < (sz - 1); ++i) {
      ifaces_[i]->setValue(id, value);
    }
    return ifaces_[i]->setValue(id, value);
  }

  void handleRequest(std::string& _return, const std::string& request) {
    size_t sz = ifaces_.size();
    size_t i = 0;
    for (; i < (sz - 1); ++i) {
      ifaces_[i]->handleRequest(_return, request);
    }
    ifaces_[i]->handleRequest(_return, request);
    return;
  }

};

// The 'concurrent' client is a thread safe client that correctly handles
// out of order responses.  It is slower than the regular client, so should
// only be used when you need to share a connection among multiple threads
template <class Protocol_>
class FtrlServiceConcurrentClientT : virtual public FtrlServiceIf {
 public:
  FtrlServiceConcurrentClientT(boost::shared_ptr< Protocol_> prot) {
    setProtocolT(prot);
  }
  FtrlServiceConcurrentClientT(boost::shared_ptr< Protocol_> iprot, boost::shared_ptr< Protocol_> oprot) {
    setProtocolT(iprot,oprot);
  }
 private:
  void setProtocolT(boost::shared_ptr< Protocol_> prot) {
  setProtocolT(prot,prot);
  }
  void setProtocolT(boost::shared_ptr< Protocol_> iprot, boost::shared_ptr< Protocol_> oprot) {
    piprot_=iprot;
    poprot_=oprot;
    iprot_ = iprot.get();
    oprot_ = oprot.get();
  }
 public:
  boost::shared_ptr< ::apache::thrift::protocol::TProtocol> getInputProtocol() {
    return this->piprot_;
  }
  boost::shared_ptr< ::apache::thrift::protocol::TProtocol> getOutputProtocol() {
    return this->poprot_;
  }
  double lrPredict(const std::string& id, const std::string& data);
  int32_t send_lrPredict(const std::string& id, const std::string& data);
  double recv_lrPredict(const int32_t seqid);
  bool setValue(const std::string& id, const double value);
  int32_t send_setValue(const std::string& id, const double value);
  bool recv_setValue(const int32_t seqid);
  void handleRequest(std::string& _return, const std::string& request);
  int32_t send_handleRequest(const std::string& request);
  void recv_handleRequest(std::string& _return, const int32_t seqid);
 protected:
  boost::shared_ptr< Protocol_> piprot_;
  boost::shared_ptr< Protocol_> poprot_;
  Protocol_* iprot_;
  Protocol_* oprot_;
  ::apache::thrift::async::TConcurrentClientSyncInfo sync_;
};

typedef FtrlServiceConcurrentClientT< ::apache::thrift::protocol::TProtocol> FtrlServiceConcurrentClient;

#ifdef _WIN32
  #pragma warning( pop )
#endif

} // namespace

#include "FtrlService.tcc"
#include "ftrl_types.tcc"

#endif
