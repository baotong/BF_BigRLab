/**
 * Autogenerated by Thrift Compiler (0.9.3)
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */
#ifndef PyService_H
#define PyService_H

#include <thrift/TDispatchProcessor.h>
#include <thrift/async/TConcurrentClientSyncInfo.h>
#include "PyTest_types.h"

namespace PyTest {

#ifdef _WIN32
  #pragma warning( push )
  #pragma warning (disable : 4250 ) //inheriting methods via dominance 
#endif

class PyServiceIf {
 public:
  virtual ~PyServiceIf() {}
  virtual void segment(std::vector<Result> & _return, const std::string& text) = 0;
  virtual void handleRequest(std::string& _return, const std::string& request) = 0;
};

class PyServiceIfFactory {
 public:
  typedef PyServiceIf Handler;

  virtual ~PyServiceIfFactory() {}

  virtual PyServiceIf* getHandler(const ::apache::thrift::TConnectionInfo& connInfo) = 0;
  virtual void releaseHandler(PyServiceIf* /* handler */) = 0;
};

class PyServiceIfSingletonFactory : virtual public PyServiceIfFactory {
 public:
  PyServiceIfSingletonFactory(const boost::shared_ptr<PyServiceIf>& iface) : iface_(iface) {}
  virtual ~PyServiceIfSingletonFactory() {}

  virtual PyServiceIf* getHandler(const ::apache::thrift::TConnectionInfo&) {
    return iface_.get();
  }
  virtual void releaseHandler(PyServiceIf* /* handler */) {}

 protected:
  boost::shared_ptr<PyServiceIf> iface_;
};

class PyServiceNull : virtual public PyServiceIf {
 public:
  virtual ~PyServiceNull() {}
  void segment(std::vector<Result> & /* _return */, const std::string& /* text */) {
    return;
  }
  void handleRequest(std::string& /* _return */, const std::string& /* request */) {
    return;
  }
};

typedef struct _PyService_segment_args__isset {
  _PyService_segment_args__isset() : text(false) {}
  bool text :1;
} _PyService_segment_args__isset;

class PyService_segment_args {
 public:

  PyService_segment_args(const PyService_segment_args&);
  PyService_segment_args(PyService_segment_args&&);
  PyService_segment_args& operator=(const PyService_segment_args&);
  PyService_segment_args& operator=(PyService_segment_args&&);
  PyService_segment_args() : text() {
  }

  virtual ~PyService_segment_args() throw();
  std::string text;

  _PyService_segment_args__isset __isset;

  void __set_text(const std::string& val);

  template <class Protocol_>
  uint32_t read(Protocol_* iprot);
  template <class Protocol_>
  uint32_t write(Protocol_* oprot) const;

};


class PyService_segment_pargs {
 public:


  virtual ~PyService_segment_pargs() throw();
  const std::string* text;

  template <class Protocol_>
  uint32_t write(Protocol_* oprot) const;

};

typedef struct _PyService_segment_result__isset {
  _PyService_segment_result__isset() : success(false), err(false) {}
  bool success :1;
  bool err :1;
} _PyService_segment_result__isset;

class PyService_segment_result {
 public:

  PyService_segment_result(const PyService_segment_result&);
  PyService_segment_result(PyService_segment_result&&);
  PyService_segment_result& operator=(const PyService_segment_result&);
  PyService_segment_result& operator=(PyService_segment_result&&);
  PyService_segment_result() {
  }

  virtual ~PyService_segment_result() throw();
  std::vector<Result>  success;
   ::AlgCommon::InvalidRequest err;

  _PyService_segment_result__isset __isset;

  void __set_success(const std::vector<Result> & val);

  void __set_err(const  ::AlgCommon::InvalidRequest& val);

  template <class Protocol_>
  uint32_t read(Protocol_* iprot);
  template <class Protocol_>
  uint32_t write(Protocol_* oprot) const;

};

typedef struct _PyService_segment_presult__isset {
  _PyService_segment_presult__isset() : success(false), err(false) {}
  bool success :1;
  bool err :1;
} _PyService_segment_presult__isset;

class PyService_segment_presult {
 public:


  virtual ~PyService_segment_presult() throw();
  std::vector<Result> * success;
   ::AlgCommon::InvalidRequest err;

  _PyService_segment_presult__isset __isset;

  template <class Protocol_>
  uint32_t read(Protocol_* iprot);

};

typedef struct _PyService_handleRequest_args__isset {
  _PyService_handleRequest_args__isset() : request(false) {}
  bool request :1;
} _PyService_handleRequest_args__isset;

class PyService_handleRequest_args {
 public:

  PyService_handleRequest_args(const PyService_handleRequest_args&);
  PyService_handleRequest_args(PyService_handleRequest_args&&);
  PyService_handleRequest_args& operator=(const PyService_handleRequest_args&);
  PyService_handleRequest_args& operator=(PyService_handleRequest_args&&);
  PyService_handleRequest_args() : request() {
  }

  virtual ~PyService_handleRequest_args() throw();
  std::string request;

  _PyService_handleRequest_args__isset __isset;

  void __set_request(const std::string& val);

  template <class Protocol_>
  uint32_t read(Protocol_* iprot);
  template <class Protocol_>
  uint32_t write(Protocol_* oprot) const;

};


class PyService_handleRequest_pargs {
 public:


  virtual ~PyService_handleRequest_pargs() throw();
  const std::string* request;

  template <class Protocol_>
  uint32_t write(Protocol_* oprot) const;

};

typedef struct _PyService_handleRequest_result__isset {
  _PyService_handleRequest_result__isset() : success(false), err(false) {}
  bool success :1;
  bool err :1;
} _PyService_handleRequest_result__isset;

class PyService_handleRequest_result {
 public:

  PyService_handleRequest_result(const PyService_handleRequest_result&);
  PyService_handleRequest_result(PyService_handleRequest_result&&);
  PyService_handleRequest_result& operator=(const PyService_handleRequest_result&);
  PyService_handleRequest_result& operator=(PyService_handleRequest_result&&);
  PyService_handleRequest_result() : success() {
  }

  virtual ~PyService_handleRequest_result() throw();
  std::string success;
   ::AlgCommon::InvalidRequest err;

  _PyService_handleRequest_result__isset __isset;

  void __set_success(const std::string& val);

  void __set_err(const  ::AlgCommon::InvalidRequest& val);

  template <class Protocol_>
  uint32_t read(Protocol_* iprot);
  template <class Protocol_>
  uint32_t write(Protocol_* oprot) const;

};

typedef struct _PyService_handleRequest_presult__isset {
  _PyService_handleRequest_presult__isset() : success(false), err(false) {}
  bool success :1;
  bool err :1;
} _PyService_handleRequest_presult__isset;

class PyService_handleRequest_presult {
 public:


  virtual ~PyService_handleRequest_presult() throw();
  std::string* success;
   ::AlgCommon::InvalidRequest err;

  _PyService_handleRequest_presult__isset __isset;

  template <class Protocol_>
  uint32_t read(Protocol_* iprot);

};

template <class Protocol_>
class PyServiceClientT : virtual public PyServiceIf {
 public:
  PyServiceClientT(boost::shared_ptr< Protocol_> prot) {
    setProtocolT(prot);
  }
  PyServiceClientT(boost::shared_ptr< Protocol_> iprot, boost::shared_ptr< Protocol_> oprot) {
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
  void segment(std::vector<Result> & _return, const std::string& text);
  void send_segment(const std::string& text);
  void recv_segment(std::vector<Result> & _return);
  void handleRequest(std::string& _return, const std::string& request);
  void send_handleRequest(const std::string& request);
  void recv_handleRequest(std::string& _return);
 protected:
  boost::shared_ptr< Protocol_> piprot_;
  boost::shared_ptr< Protocol_> poprot_;
  Protocol_* iprot_;
  Protocol_* oprot_;
};

typedef PyServiceClientT< ::apache::thrift::protocol::TProtocol> PyServiceClient;

template <class Protocol_>
class PyServiceProcessorT : public ::apache::thrift::TDispatchProcessorT<Protocol_> {
 protected:
  boost::shared_ptr<PyServiceIf> iface_;
  virtual bool dispatchCall(::apache::thrift::protocol::TProtocol* iprot, ::apache::thrift::protocol::TProtocol* oprot, const std::string& fname, int32_t seqid, void* callContext);
  virtual bool dispatchCallTemplated(Protocol_* iprot, Protocol_* oprot, const std::string& fname, int32_t seqid, void* callContext);
 private:
  typedef  void (PyServiceProcessorT::*ProcessFunction)(int32_t, ::apache::thrift::protocol::TProtocol*, ::apache::thrift::protocol::TProtocol*, void*);
  typedef void (PyServiceProcessorT::*SpecializedProcessFunction)(int32_t, Protocol_*, Protocol_*, void*);
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
  void process_segment(int32_t seqid, ::apache::thrift::protocol::TProtocol* iprot, ::apache::thrift::protocol::TProtocol* oprot, void* callContext);
  void process_segment(int32_t seqid, Protocol_* iprot, Protocol_* oprot, void* callContext);
  void process_handleRequest(int32_t seqid, ::apache::thrift::protocol::TProtocol* iprot, ::apache::thrift::protocol::TProtocol* oprot, void* callContext);
  void process_handleRequest(int32_t seqid, Protocol_* iprot, Protocol_* oprot, void* callContext);
 public:
  PyServiceProcessorT(boost::shared_ptr<PyServiceIf> iface) :
    iface_(iface) {
    processMap_["segment"] = ProcessFunctions(
      &PyServiceProcessorT::process_segment,
      &PyServiceProcessorT::process_segment);
    processMap_["handleRequest"] = ProcessFunctions(
      &PyServiceProcessorT::process_handleRequest,
      &PyServiceProcessorT::process_handleRequest);
  }

  virtual ~PyServiceProcessorT() {}
};

typedef PyServiceProcessorT< ::apache::thrift::protocol::TDummyProtocol > PyServiceProcessor;

template <class Protocol_>
class PyServiceProcessorFactoryT : public ::apache::thrift::TProcessorFactory {
 public:
  PyServiceProcessorFactoryT(const ::boost::shared_ptr< PyServiceIfFactory >& handlerFactory) :
      handlerFactory_(handlerFactory) {}

  ::boost::shared_ptr< ::apache::thrift::TProcessor > getProcessor(const ::apache::thrift::TConnectionInfo& connInfo);

 protected:
  ::boost::shared_ptr< PyServiceIfFactory > handlerFactory_;
};

typedef PyServiceProcessorFactoryT< ::apache::thrift::protocol::TDummyProtocol > PyServiceProcessorFactory;

class PyServiceMultiface : virtual public PyServiceIf {
 public:
  PyServiceMultiface(std::vector<boost::shared_ptr<PyServiceIf> >& ifaces) : ifaces_(ifaces) {
  }
  virtual ~PyServiceMultiface() {}
 protected:
  std::vector<boost::shared_ptr<PyServiceIf> > ifaces_;
  PyServiceMultiface() {}
  void add(boost::shared_ptr<PyServiceIf> iface) {
    ifaces_.push_back(iface);
  }
 public:
  void segment(std::vector<Result> & _return, const std::string& text) {
    size_t sz = ifaces_.size();
    size_t i = 0;
    for (; i < (sz - 1); ++i) {
      ifaces_[i]->segment(_return, text);
    }
    ifaces_[i]->segment(_return, text);
    return;
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
class PyServiceConcurrentClientT : virtual public PyServiceIf {
 public:
  PyServiceConcurrentClientT(boost::shared_ptr< Protocol_> prot) {
    setProtocolT(prot);
  }
  PyServiceConcurrentClientT(boost::shared_ptr< Protocol_> iprot, boost::shared_ptr< Protocol_> oprot) {
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
  void segment(std::vector<Result> & _return, const std::string& text);
  int32_t send_segment(const std::string& text);
  void recv_segment(std::vector<Result> & _return, const int32_t seqid);
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

typedef PyServiceConcurrentClientT< ::apache::thrift::protocol::TProtocol> PyServiceConcurrentClient;

#ifdef _WIN32
  #pragma warning( pop )
#endif

} // namespace

#include "PyService.tcc"
#include "PyTest_types.tcc"

#endif
