/**
 * Autogenerated by Thrift Compiler (0.9.3)
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */
#ifndef KnnService_H
#define KnnService_H

#include <thrift/TDispatchProcessor.h>
#include <thrift/async/TConcurrentClientSyncInfo.h>
#include "knn_types.h"

namespace KNN {

#ifdef _WIN32
  #pragma warning( push )
  #pragma warning (disable : 4250 ) //inheriting methods via dominance 
#endif

class KnnServiceIf {
 public:
  virtual ~KnnServiceIf() {}
  virtual void queryByItem(std::vector<Result> & _return, const std::string& item, const int32_t n) = 0;
  virtual void queryByVector(std::vector<Result> & _return, const std::vector<double> & values, const int32_t n) = 0;
  virtual void queryByVectorNoWeight(std::vector<std::string> & _return, const std::vector<double> & values, const int32_t n) = 0;
  virtual void queryByItemNoWeight(std::vector<std::string> & _return, const std::string& item, const int32_t n) = 0;
  virtual void handleRequest(std::string& _return, const std::string& request) = 0;
};

class KnnServiceIfFactory {
 public:
  typedef KnnServiceIf Handler;

  virtual ~KnnServiceIfFactory() {}

  virtual KnnServiceIf* getHandler(const ::apache::thrift::TConnectionInfo& connInfo) = 0;
  virtual void releaseHandler(KnnServiceIf* /* handler */) = 0;
};

class KnnServiceIfSingletonFactory : virtual public KnnServiceIfFactory {
 public:
  KnnServiceIfSingletonFactory(const boost::shared_ptr<KnnServiceIf>& iface) : iface_(iface) {}
  virtual ~KnnServiceIfSingletonFactory() {}

  virtual KnnServiceIf* getHandler(const ::apache::thrift::TConnectionInfo&) {
    return iface_.get();
  }
  virtual void releaseHandler(KnnServiceIf* /* handler */) {}

 protected:
  boost::shared_ptr<KnnServiceIf> iface_;
};

class KnnServiceNull : virtual public KnnServiceIf {
 public:
  virtual ~KnnServiceNull() {}
  void queryByItem(std::vector<Result> & /* _return */, const std::string& /* item */, const int32_t /* n */) {
    return;
  }
  void queryByVector(std::vector<Result> & /* _return */, const std::vector<double> & /* values */, const int32_t /* n */) {
    return;
  }
  void queryByVectorNoWeight(std::vector<std::string> & /* _return */, const std::vector<double> & /* values */, const int32_t /* n */) {
    return;
  }
  void queryByItemNoWeight(std::vector<std::string> & /* _return */, const std::string& /* item */, const int32_t /* n */) {
    return;
  }
  void handleRequest(std::string& /* _return */, const std::string& /* request */) {
    return;
  }
};

typedef struct _KnnService_queryByItem_args__isset {
  _KnnService_queryByItem_args__isset() : item(false), n(false) {}
  bool item :1;
  bool n :1;
} _KnnService_queryByItem_args__isset;

class KnnService_queryByItem_args {
 public:

  KnnService_queryByItem_args(const KnnService_queryByItem_args&);
  KnnService_queryByItem_args(KnnService_queryByItem_args&&);
  KnnService_queryByItem_args& operator=(const KnnService_queryByItem_args&);
  KnnService_queryByItem_args& operator=(KnnService_queryByItem_args&&);
  KnnService_queryByItem_args() : item(), n(0) {
  }

  virtual ~KnnService_queryByItem_args() throw();
  std::string item;
  int32_t n;

  _KnnService_queryByItem_args__isset __isset;

  void __set_item(const std::string& val);

  void __set_n(const int32_t val);

  template <class Protocol_>
  uint32_t read(Protocol_* iprot);
  template <class Protocol_>
  uint32_t write(Protocol_* oprot) const;

};


class KnnService_queryByItem_pargs {
 public:


  virtual ~KnnService_queryByItem_pargs() throw();
  const std::string* item;
  const int32_t* n;

  template <class Protocol_>
  uint32_t write(Protocol_* oprot) const;

};

typedef struct _KnnService_queryByItem_result__isset {
  _KnnService_queryByItem_result__isset() : success(false), err(false) {}
  bool success :1;
  bool err :1;
} _KnnService_queryByItem_result__isset;

class KnnService_queryByItem_result {
 public:

  KnnService_queryByItem_result(const KnnService_queryByItem_result&);
  KnnService_queryByItem_result(KnnService_queryByItem_result&&);
  KnnService_queryByItem_result& operator=(const KnnService_queryByItem_result&);
  KnnService_queryByItem_result& operator=(KnnService_queryByItem_result&&);
  KnnService_queryByItem_result() {
  }

  virtual ~KnnService_queryByItem_result() throw();
  std::vector<Result>  success;
  InvalidRequest err;

  _KnnService_queryByItem_result__isset __isset;

  void __set_success(const std::vector<Result> & val);

  void __set_err(const InvalidRequest& val);

  template <class Protocol_>
  uint32_t read(Protocol_* iprot);
  template <class Protocol_>
  uint32_t write(Protocol_* oprot) const;

};

typedef struct _KnnService_queryByItem_presult__isset {
  _KnnService_queryByItem_presult__isset() : success(false), err(false) {}
  bool success :1;
  bool err :1;
} _KnnService_queryByItem_presult__isset;

class KnnService_queryByItem_presult {
 public:


  virtual ~KnnService_queryByItem_presult() throw();
  std::vector<Result> * success;
  InvalidRequest err;

  _KnnService_queryByItem_presult__isset __isset;

  template <class Protocol_>
  uint32_t read(Protocol_* iprot);

};

typedef struct _KnnService_queryByVector_args__isset {
  _KnnService_queryByVector_args__isset() : values(false), n(false) {}
  bool values :1;
  bool n :1;
} _KnnService_queryByVector_args__isset;

class KnnService_queryByVector_args {
 public:

  KnnService_queryByVector_args(const KnnService_queryByVector_args&);
  KnnService_queryByVector_args(KnnService_queryByVector_args&&);
  KnnService_queryByVector_args& operator=(const KnnService_queryByVector_args&);
  KnnService_queryByVector_args& operator=(KnnService_queryByVector_args&&);
  KnnService_queryByVector_args() : n(0) {
  }

  virtual ~KnnService_queryByVector_args() throw();
  std::vector<double>  values;
  int32_t n;

  _KnnService_queryByVector_args__isset __isset;

  void __set_values(const std::vector<double> & val);

  void __set_n(const int32_t val);

  template <class Protocol_>
  uint32_t read(Protocol_* iprot);
  template <class Protocol_>
  uint32_t write(Protocol_* oprot) const;

};


class KnnService_queryByVector_pargs {
 public:


  virtual ~KnnService_queryByVector_pargs() throw();
  const std::vector<double> * values;
  const int32_t* n;

  template <class Protocol_>
  uint32_t write(Protocol_* oprot) const;

};

typedef struct _KnnService_queryByVector_result__isset {
  _KnnService_queryByVector_result__isset() : success(false), err(false) {}
  bool success :1;
  bool err :1;
} _KnnService_queryByVector_result__isset;

class KnnService_queryByVector_result {
 public:

  KnnService_queryByVector_result(const KnnService_queryByVector_result&);
  KnnService_queryByVector_result(KnnService_queryByVector_result&&);
  KnnService_queryByVector_result& operator=(const KnnService_queryByVector_result&);
  KnnService_queryByVector_result& operator=(KnnService_queryByVector_result&&);
  KnnService_queryByVector_result() {
  }

  virtual ~KnnService_queryByVector_result() throw();
  std::vector<Result>  success;
  InvalidRequest err;

  _KnnService_queryByVector_result__isset __isset;

  void __set_success(const std::vector<Result> & val);

  void __set_err(const InvalidRequest& val);

  template <class Protocol_>
  uint32_t read(Protocol_* iprot);
  template <class Protocol_>
  uint32_t write(Protocol_* oprot) const;

};

typedef struct _KnnService_queryByVector_presult__isset {
  _KnnService_queryByVector_presult__isset() : success(false), err(false) {}
  bool success :1;
  bool err :1;
} _KnnService_queryByVector_presult__isset;

class KnnService_queryByVector_presult {
 public:


  virtual ~KnnService_queryByVector_presult() throw();
  std::vector<Result> * success;
  InvalidRequest err;

  _KnnService_queryByVector_presult__isset __isset;

  template <class Protocol_>
  uint32_t read(Protocol_* iprot);

};

typedef struct _KnnService_queryByVectorNoWeight_args__isset {
  _KnnService_queryByVectorNoWeight_args__isset() : values(false), n(false) {}
  bool values :1;
  bool n :1;
} _KnnService_queryByVectorNoWeight_args__isset;

class KnnService_queryByVectorNoWeight_args {
 public:

  KnnService_queryByVectorNoWeight_args(const KnnService_queryByVectorNoWeight_args&);
  KnnService_queryByVectorNoWeight_args(KnnService_queryByVectorNoWeight_args&&);
  KnnService_queryByVectorNoWeight_args& operator=(const KnnService_queryByVectorNoWeight_args&);
  KnnService_queryByVectorNoWeight_args& operator=(KnnService_queryByVectorNoWeight_args&&);
  KnnService_queryByVectorNoWeight_args() : n(0) {
  }

  virtual ~KnnService_queryByVectorNoWeight_args() throw();
  std::vector<double>  values;
  int32_t n;

  _KnnService_queryByVectorNoWeight_args__isset __isset;

  void __set_values(const std::vector<double> & val);

  void __set_n(const int32_t val);

  template <class Protocol_>
  uint32_t read(Protocol_* iprot);
  template <class Protocol_>
  uint32_t write(Protocol_* oprot) const;

};


class KnnService_queryByVectorNoWeight_pargs {
 public:


  virtual ~KnnService_queryByVectorNoWeight_pargs() throw();
  const std::vector<double> * values;
  const int32_t* n;

  template <class Protocol_>
  uint32_t write(Protocol_* oprot) const;

};

typedef struct _KnnService_queryByVectorNoWeight_result__isset {
  _KnnService_queryByVectorNoWeight_result__isset() : success(false), err(false) {}
  bool success :1;
  bool err :1;
} _KnnService_queryByVectorNoWeight_result__isset;

class KnnService_queryByVectorNoWeight_result {
 public:

  KnnService_queryByVectorNoWeight_result(const KnnService_queryByVectorNoWeight_result&);
  KnnService_queryByVectorNoWeight_result(KnnService_queryByVectorNoWeight_result&&);
  KnnService_queryByVectorNoWeight_result& operator=(const KnnService_queryByVectorNoWeight_result&);
  KnnService_queryByVectorNoWeight_result& operator=(KnnService_queryByVectorNoWeight_result&&);
  KnnService_queryByVectorNoWeight_result() {
  }

  virtual ~KnnService_queryByVectorNoWeight_result() throw();
  std::vector<std::string>  success;
  InvalidRequest err;

  _KnnService_queryByVectorNoWeight_result__isset __isset;

  void __set_success(const std::vector<std::string> & val);

  void __set_err(const InvalidRequest& val);

  template <class Protocol_>
  uint32_t read(Protocol_* iprot);
  template <class Protocol_>
  uint32_t write(Protocol_* oprot) const;

};

typedef struct _KnnService_queryByVectorNoWeight_presult__isset {
  _KnnService_queryByVectorNoWeight_presult__isset() : success(false), err(false) {}
  bool success :1;
  bool err :1;
} _KnnService_queryByVectorNoWeight_presult__isset;

class KnnService_queryByVectorNoWeight_presult {
 public:


  virtual ~KnnService_queryByVectorNoWeight_presult() throw();
  std::vector<std::string> * success;
  InvalidRequest err;

  _KnnService_queryByVectorNoWeight_presult__isset __isset;

  template <class Protocol_>
  uint32_t read(Protocol_* iprot);

};

typedef struct _KnnService_queryByItemNoWeight_args__isset {
  _KnnService_queryByItemNoWeight_args__isset() : item(false), n(false) {}
  bool item :1;
  bool n :1;
} _KnnService_queryByItemNoWeight_args__isset;

class KnnService_queryByItemNoWeight_args {
 public:

  KnnService_queryByItemNoWeight_args(const KnnService_queryByItemNoWeight_args&);
  KnnService_queryByItemNoWeight_args(KnnService_queryByItemNoWeight_args&&);
  KnnService_queryByItemNoWeight_args& operator=(const KnnService_queryByItemNoWeight_args&);
  KnnService_queryByItemNoWeight_args& operator=(KnnService_queryByItemNoWeight_args&&);
  KnnService_queryByItemNoWeight_args() : item(), n(0) {
  }

  virtual ~KnnService_queryByItemNoWeight_args() throw();
  std::string item;
  int32_t n;

  _KnnService_queryByItemNoWeight_args__isset __isset;

  void __set_item(const std::string& val);

  void __set_n(const int32_t val);

  template <class Protocol_>
  uint32_t read(Protocol_* iprot);
  template <class Protocol_>
  uint32_t write(Protocol_* oprot) const;

};


class KnnService_queryByItemNoWeight_pargs {
 public:


  virtual ~KnnService_queryByItemNoWeight_pargs() throw();
  const std::string* item;
  const int32_t* n;

  template <class Protocol_>
  uint32_t write(Protocol_* oprot) const;

};

typedef struct _KnnService_queryByItemNoWeight_result__isset {
  _KnnService_queryByItemNoWeight_result__isset() : success(false), err(false) {}
  bool success :1;
  bool err :1;
} _KnnService_queryByItemNoWeight_result__isset;

class KnnService_queryByItemNoWeight_result {
 public:

  KnnService_queryByItemNoWeight_result(const KnnService_queryByItemNoWeight_result&);
  KnnService_queryByItemNoWeight_result(KnnService_queryByItemNoWeight_result&&);
  KnnService_queryByItemNoWeight_result& operator=(const KnnService_queryByItemNoWeight_result&);
  KnnService_queryByItemNoWeight_result& operator=(KnnService_queryByItemNoWeight_result&&);
  KnnService_queryByItemNoWeight_result() {
  }

  virtual ~KnnService_queryByItemNoWeight_result() throw();
  std::vector<std::string>  success;
  InvalidRequest err;

  _KnnService_queryByItemNoWeight_result__isset __isset;

  void __set_success(const std::vector<std::string> & val);

  void __set_err(const InvalidRequest& val);

  template <class Protocol_>
  uint32_t read(Protocol_* iprot);
  template <class Protocol_>
  uint32_t write(Protocol_* oprot) const;

};

typedef struct _KnnService_queryByItemNoWeight_presult__isset {
  _KnnService_queryByItemNoWeight_presult__isset() : success(false), err(false) {}
  bool success :1;
  bool err :1;
} _KnnService_queryByItemNoWeight_presult__isset;

class KnnService_queryByItemNoWeight_presult {
 public:


  virtual ~KnnService_queryByItemNoWeight_presult() throw();
  std::vector<std::string> * success;
  InvalidRequest err;

  _KnnService_queryByItemNoWeight_presult__isset __isset;

  template <class Protocol_>
  uint32_t read(Protocol_* iprot);

};

typedef struct _KnnService_handleRequest_args__isset {
  _KnnService_handleRequest_args__isset() : request(false) {}
  bool request :1;
} _KnnService_handleRequest_args__isset;

class KnnService_handleRequest_args {
 public:

  KnnService_handleRequest_args(const KnnService_handleRequest_args&);
  KnnService_handleRequest_args(KnnService_handleRequest_args&&);
  KnnService_handleRequest_args& operator=(const KnnService_handleRequest_args&);
  KnnService_handleRequest_args& operator=(KnnService_handleRequest_args&&);
  KnnService_handleRequest_args() : request() {
  }

  virtual ~KnnService_handleRequest_args() throw();
  std::string request;

  _KnnService_handleRequest_args__isset __isset;

  void __set_request(const std::string& val);

  template <class Protocol_>
  uint32_t read(Protocol_* iprot);
  template <class Protocol_>
  uint32_t write(Protocol_* oprot) const;

};


class KnnService_handleRequest_pargs {
 public:


  virtual ~KnnService_handleRequest_pargs() throw();
  const std::string* request;

  template <class Protocol_>
  uint32_t write(Protocol_* oprot) const;

};

typedef struct _KnnService_handleRequest_result__isset {
  _KnnService_handleRequest_result__isset() : success(false), err(false) {}
  bool success :1;
  bool err :1;
} _KnnService_handleRequest_result__isset;

class KnnService_handleRequest_result {
 public:

  KnnService_handleRequest_result(const KnnService_handleRequest_result&);
  KnnService_handleRequest_result(KnnService_handleRequest_result&&);
  KnnService_handleRequest_result& operator=(const KnnService_handleRequest_result&);
  KnnService_handleRequest_result& operator=(KnnService_handleRequest_result&&);
  KnnService_handleRequest_result() : success() {
  }

  virtual ~KnnService_handleRequest_result() throw();
  std::string success;
  InvalidRequest err;

  _KnnService_handleRequest_result__isset __isset;

  void __set_success(const std::string& val);

  void __set_err(const InvalidRequest& val);

  template <class Protocol_>
  uint32_t read(Protocol_* iprot);
  template <class Protocol_>
  uint32_t write(Protocol_* oprot) const;

};

typedef struct _KnnService_handleRequest_presult__isset {
  _KnnService_handleRequest_presult__isset() : success(false), err(false) {}
  bool success :1;
  bool err :1;
} _KnnService_handleRequest_presult__isset;

class KnnService_handleRequest_presult {
 public:


  virtual ~KnnService_handleRequest_presult() throw();
  std::string* success;
  InvalidRequest err;

  _KnnService_handleRequest_presult__isset __isset;

  template <class Protocol_>
  uint32_t read(Protocol_* iprot);

};

template <class Protocol_>
class KnnServiceClientT : virtual public KnnServiceIf {
 public:
  KnnServiceClientT(boost::shared_ptr< Protocol_> prot) {
    setProtocolT(prot);
  }
  KnnServiceClientT(boost::shared_ptr< Protocol_> iprot, boost::shared_ptr< Protocol_> oprot) {
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
  void queryByItem(std::vector<Result> & _return, const std::string& item, const int32_t n);
  void send_queryByItem(const std::string& item, const int32_t n);
  void recv_queryByItem(std::vector<Result> & _return);
  void queryByVector(std::vector<Result> & _return, const std::vector<double> & values, const int32_t n);
  void send_queryByVector(const std::vector<double> & values, const int32_t n);
  void recv_queryByVector(std::vector<Result> & _return);
  void queryByVectorNoWeight(std::vector<std::string> & _return, const std::vector<double> & values, const int32_t n);
  void send_queryByVectorNoWeight(const std::vector<double> & values, const int32_t n);
  void recv_queryByVectorNoWeight(std::vector<std::string> & _return);
  void queryByItemNoWeight(std::vector<std::string> & _return, const std::string& item, const int32_t n);
  void send_queryByItemNoWeight(const std::string& item, const int32_t n);
  void recv_queryByItemNoWeight(std::vector<std::string> & _return);
  void handleRequest(std::string& _return, const std::string& request);
  void send_handleRequest(const std::string& request);
  void recv_handleRequest(std::string& _return);
 protected:
  boost::shared_ptr< Protocol_> piprot_;
  boost::shared_ptr< Protocol_> poprot_;
  Protocol_* iprot_;
  Protocol_* oprot_;
};

typedef KnnServiceClientT< ::apache::thrift::protocol::TProtocol> KnnServiceClient;

template <class Protocol_>
class KnnServiceProcessorT : public ::apache::thrift::TDispatchProcessorT<Protocol_> {
 protected:
  boost::shared_ptr<KnnServiceIf> iface_;
  virtual bool dispatchCall(::apache::thrift::protocol::TProtocol* iprot, ::apache::thrift::protocol::TProtocol* oprot, const std::string& fname, int32_t seqid, void* callContext);
  virtual bool dispatchCallTemplated(Protocol_* iprot, Protocol_* oprot, const std::string& fname, int32_t seqid, void* callContext);
 private:
  typedef  void (KnnServiceProcessorT::*ProcessFunction)(int32_t, ::apache::thrift::protocol::TProtocol*, ::apache::thrift::protocol::TProtocol*, void*);
  typedef void (KnnServiceProcessorT::*SpecializedProcessFunction)(int32_t, Protocol_*, Protocol_*, void*);
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
  void process_queryByItem(int32_t seqid, ::apache::thrift::protocol::TProtocol* iprot, ::apache::thrift::protocol::TProtocol* oprot, void* callContext);
  void process_queryByItem(int32_t seqid, Protocol_* iprot, Protocol_* oprot, void* callContext);
  void process_queryByVector(int32_t seqid, ::apache::thrift::protocol::TProtocol* iprot, ::apache::thrift::protocol::TProtocol* oprot, void* callContext);
  void process_queryByVector(int32_t seqid, Protocol_* iprot, Protocol_* oprot, void* callContext);
  void process_queryByVectorNoWeight(int32_t seqid, ::apache::thrift::protocol::TProtocol* iprot, ::apache::thrift::protocol::TProtocol* oprot, void* callContext);
  void process_queryByVectorNoWeight(int32_t seqid, Protocol_* iprot, Protocol_* oprot, void* callContext);
  void process_queryByItemNoWeight(int32_t seqid, ::apache::thrift::protocol::TProtocol* iprot, ::apache::thrift::protocol::TProtocol* oprot, void* callContext);
  void process_queryByItemNoWeight(int32_t seqid, Protocol_* iprot, Protocol_* oprot, void* callContext);
  void process_handleRequest(int32_t seqid, ::apache::thrift::protocol::TProtocol* iprot, ::apache::thrift::protocol::TProtocol* oprot, void* callContext);
  void process_handleRequest(int32_t seqid, Protocol_* iprot, Protocol_* oprot, void* callContext);
 public:
  KnnServiceProcessorT(boost::shared_ptr<KnnServiceIf> iface) :
    iface_(iface) {
    processMap_["queryByItem"] = ProcessFunctions(
      &KnnServiceProcessorT::process_queryByItem,
      &KnnServiceProcessorT::process_queryByItem);
    processMap_["queryByVector"] = ProcessFunctions(
      &KnnServiceProcessorT::process_queryByVector,
      &KnnServiceProcessorT::process_queryByVector);
    processMap_["queryByVectorNoWeight"] = ProcessFunctions(
      &KnnServiceProcessorT::process_queryByVectorNoWeight,
      &KnnServiceProcessorT::process_queryByVectorNoWeight);
    processMap_["queryByItemNoWeight"] = ProcessFunctions(
      &KnnServiceProcessorT::process_queryByItemNoWeight,
      &KnnServiceProcessorT::process_queryByItemNoWeight);
    processMap_["handleRequest"] = ProcessFunctions(
      &KnnServiceProcessorT::process_handleRequest,
      &KnnServiceProcessorT::process_handleRequest);
  }

  virtual ~KnnServiceProcessorT() {}
};

typedef KnnServiceProcessorT< ::apache::thrift::protocol::TDummyProtocol > KnnServiceProcessor;

template <class Protocol_>
class KnnServiceProcessorFactoryT : public ::apache::thrift::TProcessorFactory {
 public:
  KnnServiceProcessorFactoryT(const ::boost::shared_ptr< KnnServiceIfFactory >& handlerFactory) :
      handlerFactory_(handlerFactory) {}

  ::boost::shared_ptr< ::apache::thrift::TProcessor > getProcessor(const ::apache::thrift::TConnectionInfo& connInfo);

 protected:
  ::boost::shared_ptr< KnnServiceIfFactory > handlerFactory_;
};

typedef KnnServiceProcessorFactoryT< ::apache::thrift::protocol::TDummyProtocol > KnnServiceProcessorFactory;

class KnnServiceMultiface : virtual public KnnServiceIf {
 public:
  KnnServiceMultiface(std::vector<boost::shared_ptr<KnnServiceIf> >& ifaces) : ifaces_(ifaces) {
  }
  virtual ~KnnServiceMultiface() {}
 protected:
  std::vector<boost::shared_ptr<KnnServiceIf> > ifaces_;
  KnnServiceMultiface() {}
  void add(boost::shared_ptr<KnnServiceIf> iface) {
    ifaces_.push_back(iface);
  }
 public:
  void queryByItem(std::vector<Result> & _return, const std::string& item, const int32_t n) {
    size_t sz = ifaces_.size();
    size_t i = 0;
    for (; i < (sz - 1); ++i) {
      ifaces_[i]->queryByItem(_return, item, n);
    }
    ifaces_[i]->queryByItem(_return, item, n);
    return;
  }

  void queryByVector(std::vector<Result> & _return, const std::vector<double> & values, const int32_t n) {
    size_t sz = ifaces_.size();
    size_t i = 0;
    for (; i < (sz - 1); ++i) {
      ifaces_[i]->queryByVector(_return, values, n);
    }
    ifaces_[i]->queryByVector(_return, values, n);
    return;
  }

  void queryByVectorNoWeight(std::vector<std::string> & _return, const std::vector<double> & values, const int32_t n) {
    size_t sz = ifaces_.size();
    size_t i = 0;
    for (; i < (sz - 1); ++i) {
      ifaces_[i]->queryByVectorNoWeight(_return, values, n);
    }
    ifaces_[i]->queryByVectorNoWeight(_return, values, n);
    return;
  }

  void queryByItemNoWeight(std::vector<std::string> & _return, const std::string& item, const int32_t n) {
    size_t sz = ifaces_.size();
    size_t i = 0;
    for (; i < (sz - 1); ++i) {
      ifaces_[i]->queryByItemNoWeight(_return, item, n);
    }
    ifaces_[i]->queryByItemNoWeight(_return, item, n);
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
class KnnServiceConcurrentClientT : virtual public KnnServiceIf {
 public:
  KnnServiceConcurrentClientT(boost::shared_ptr< Protocol_> prot) {
    setProtocolT(prot);
  }
  KnnServiceConcurrentClientT(boost::shared_ptr< Protocol_> iprot, boost::shared_ptr< Protocol_> oprot) {
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
  void queryByItem(std::vector<Result> & _return, const std::string& item, const int32_t n);
  int32_t send_queryByItem(const std::string& item, const int32_t n);
  void recv_queryByItem(std::vector<Result> & _return, const int32_t seqid);
  void queryByVector(std::vector<Result> & _return, const std::vector<double> & values, const int32_t n);
  int32_t send_queryByVector(const std::vector<double> & values, const int32_t n);
  void recv_queryByVector(std::vector<Result> & _return, const int32_t seqid);
  void queryByVectorNoWeight(std::vector<std::string> & _return, const std::vector<double> & values, const int32_t n);
  int32_t send_queryByVectorNoWeight(const std::vector<double> & values, const int32_t n);
  void recv_queryByVectorNoWeight(std::vector<std::string> & _return, const int32_t seqid);
  void queryByItemNoWeight(std::vector<std::string> & _return, const std::string& item, const int32_t n);
  int32_t send_queryByItemNoWeight(const std::string& item, const int32_t n);
  void recv_queryByItemNoWeight(std::vector<std::string> & _return, const int32_t seqid);
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

typedef KnnServiceConcurrentClientT< ::apache::thrift::protocol::TProtocol> KnnServiceConcurrentClient;

#ifdef _WIN32
  #pragma warning( pop )
#endif

} // namespace

#include "KnnService.tcc"
#include "knn_types.tcc"

#endif
