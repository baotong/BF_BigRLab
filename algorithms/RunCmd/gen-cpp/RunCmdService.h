/**
 * Autogenerated by Thrift Compiler (0.9.3)
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */
#ifndef RunCmdService_H
#define RunCmdService_H

#include <thrift/TDispatchProcessor.h>
#include <thrift/async/TConcurrentClientSyncInfo.h>
#include "RunCmd_types.h"

namespace RunCmd {

#ifdef _WIN32
  #pragma warning( push )
  #pragma warning (disable : 4250 ) //inheriting methods via dominance 
#endif

class RunCmdServiceIf {
 public:
  virtual ~RunCmdServiceIf() {}
  virtual void readCmd(CmdResult& _return, const std::string& cmd) = 0;
  virtual int32_t runCmd(const std::string& cmd) = 0;
  virtual void getAlgMgr(std::string& _return) = 0;
};

class RunCmdServiceIfFactory {
 public:
  typedef RunCmdServiceIf Handler;

  virtual ~RunCmdServiceIfFactory() {}

  virtual RunCmdServiceIf* getHandler(const ::apache::thrift::TConnectionInfo& connInfo) = 0;
  virtual void releaseHandler(RunCmdServiceIf* /* handler */) = 0;
};

class RunCmdServiceIfSingletonFactory : virtual public RunCmdServiceIfFactory {
 public:
  RunCmdServiceIfSingletonFactory(const boost::shared_ptr<RunCmdServiceIf>& iface) : iface_(iface) {}
  virtual ~RunCmdServiceIfSingletonFactory() {}

  virtual RunCmdServiceIf* getHandler(const ::apache::thrift::TConnectionInfo&) {
    return iface_.get();
  }
  virtual void releaseHandler(RunCmdServiceIf* /* handler */) {}

 protected:
  boost::shared_ptr<RunCmdServiceIf> iface_;
};

class RunCmdServiceNull : virtual public RunCmdServiceIf {
 public:
  virtual ~RunCmdServiceNull() {}
  void readCmd(CmdResult& /* _return */, const std::string& /* cmd */) {
    return;
  }
  int32_t runCmd(const std::string& /* cmd */) {
    int32_t _return = 0;
    return _return;
  }
  void getAlgMgr(std::string& /* _return */) {
    return;
  }
};

typedef struct _RunCmdService_readCmd_args__isset {
  _RunCmdService_readCmd_args__isset() : cmd(false) {}
  bool cmd :1;
} _RunCmdService_readCmd_args__isset;

class RunCmdService_readCmd_args {
 public:

  RunCmdService_readCmd_args(const RunCmdService_readCmd_args&);
  RunCmdService_readCmd_args(RunCmdService_readCmd_args&&);
  RunCmdService_readCmd_args& operator=(const RunCmdService_readCmd_args&);
  RunCmdService_readCmd_args& operator=(RunCmdService_readCmd_args&&);
  RunCmdService_readCmd_args() : cmd() {
  }

  virtual ~RunCmdService_readCmd_args() throw();
  std::string cmd;

  _RunCmdService_readCmd_args__isset __isset;

  void __set_cmd(const std::string& val);

  template <class Protocol_>
  uint32_t read(Protocol_* iprot);
  template <class Protocol_>
  uint32_t write(Protocol_* oprot) const;

};


class RunCmdService_readCmd_pargs {
 public:


  virtual ~RunCmdService_readCmd_pargs() throw();
  const std::string* cmd;

  template <class Protocol_>
  uint32_t write(Protocol_* oprot) const;

};

typedef struct _RunCmdService_readCmd_result__isset {
  _RunCmdService_readCmd_result__isset() : success(false), err(false) {}
  bool success :1;
  bool err :1;
} _RunCmdService_readCmd_result__isset;

class RunCmdService_readCmd_result {
 public:

  RunCmdService_readCmd_result(const RunCmdService_readCmd_result&);
  RunCmdService_readCmd_result(RunCmdService_readCmd_result&&);
  RunCmdService_readCmd_result& operator=(const RunCmdService_readCmd_result&);
  RunCmdService_readCmd_result& operator=(RunCmdService_readCmd_result&&);
  RunCmdService_readCmd_result() {
  }

  virtual ~RunCmdService_readCmd_result() throw();
  CmdResult success;
   ::AlgCommon::InvalidRequest err;

  _RunCmdService_readCmd_result__isset __isset;

  void __set_success(const CmdResult& val);

  void __set_err(const  ::AlgCommon::InvalidRequest& val);

  template <class Protocol_>
  uint32_t read(Protocol_* iprot);
  template <class Protocol_>
  uint32_t write(Protocol_* oprot) const;

};

typedef struct _RunCmdService_readCmd_presult__isset {
  _RunCmdService_readCmd_presult__isset() : success(false), err(false) {}
  bool success :1;
  bool err :1;
} _RunCmdService_readCmd_presult__isset;

class RunCmdService_readCmd_presult {
 public:


  virtual ~RunCmdService_readCmd_presult() throw();
  CmdResult* success;
   ::AlgCommon::InvalidRequest err;

  _RunCmdService_readCmd_presult__isset __isset;

  template <class Protocol_>
  uint32_t read(Protocol_* iprot);

};

typedef struct _RunCmdService_runCmd_args__isset {
  _RunCmdService_runCmd_args__isset() : cmd(false) {}
  bool cmd :1;
} _RunCmdService_runCmd_args__isset;

class RunCmdService_runCmd_args {
 public:

  RunCmdService_runCmd_args(const RunCmdService_runCmd_args&);
  RunCmdService_runCmd_args(RunCmdService_runCmd_args&&);
  RunCmdService_runCmd_args& operator=(const RunCmdService_runCmd_args&);
  RunCmdService_runCmd_args& operator=(RunCmdService_runCmd_args&&);
  RunCmdService_runCmd_args() : cmd() {
  }

  virtual ~RunCmdService_runCmd_args() throw();
  std::string cmd;

  _RunCmdService_runCmd_args__isset __isset;

  void __set_cmd(const std::string& val);

  template <class Protocol_>
  uint32_t read(Protocol_* iprot);
  template <class Protocol_>
  uint32_t write(Protocol_* oprot) const;

};


class RunCmdService_runCmd_pargs {
 public:


  virtual ~RunCmdService_runCmd_pargs() throw();
  const std::string* cmd;

  template <class Protocol_>
  uint32_t write(Protocol_* oprot) const;

};

typedef struct _RunCmdService_runCmd_result__isset {
  _RunCmdService_runCmd_result__isset() : success(false), err(false) {}
  bool success :1;
  bool err :1;
} _RunCmdService_runCmd_result__isset;

class RunCmdService_runCmd_result {
 public:

  RunCmdService_runCmd_result(const RunCmdService_runCmd_result&);
  RunCmdService_runCmd_result(RunCmdService_runCmd_result&&);
  RunCmdService_runCmd_result& operator=(const RunCmdService_runCmd_result&);
  RunCmdService_runCmd_result& operator=(RunCmdService_runCmd_result&&);
  RunCmdService_runCmd_result() : success(0) {
  }

  virtual ~RunCmdService_runCmd_result() throw();
  int32_t success;
   ::AlgCommon::InvalidRequest err;

  _RunCmdService_runCmd_result__isset __isset;

  void __set_success(const int32_t val);

  void __set_err(const  ::AlgCommon::InvalidRequest& val);

  template <class Protocol_>
  uint32_t read(Protocol_* iprot);
  template <class Protocol_>
  uint32_t write(Protocol_* oprot) const;

};

typedef struct _RunCmdService_runCmd_presult__isset {
  _RunCmdService_runCmd_presult__isset() : success(false), err(false) {}
  bool success :1;
  bool err :1;
} _RunCmdService_runCmd_presult__isset;

class RunCmdService_runCmd_presult {
 public:


  virtual ~RunCmdService_runCmd_presult() throw();
  int32_t* success;
   ::AlgCommon::InvalidRequest err;

  _RunCmdService_runCmd_presult__isset __isset;

  template <class Protocol_>
  uint32_t read(Protocol_* iprot);

};


class RunCmdService_getAlgMgr_args {
 public:

  RunCmdService_getAlgMgr_args(const RunCmdService_getAlgMgr_args&);
  RunCmdService_getAlgMgr_args(RunCmdService_getAlgMgr_args&&);
  RunCmdService_getAlgMgr_args& operator=(const RunCmdService_getAlgMgr_args&);
  RunCmdService_getAlgMgr_args& operator=(RunCmdService_getAlgMgr_args&&);
  RunCmdService_getAlgMgr_args() {
  }

  virtual ~RunCmdService_getAlgMgr_args() throw();

  template <class Protocol_>
  uint32_t read(Protocol_* iprot);
  template <class Protocol_>
  uint32_t write(Protocol_* oprot) const;

};


class RunCmdService_getAlgMgr_pargs {
 public:


  virtual ~RunCmdService_getAlgMgr_pargs() throw();

  template <class Protocol_>
  uint32_t write(Protocol_* oprot) const;

};

typedef struct _RunCmdService_getAlgMgr_result__isset {
  _RunCmdService_getAlgMgr_result__isset() : success(false) {}
  bool success :1;
} _RunCmdService_getAlgMgr_result__isset;

class RunCmdService_getAlgMgr_result {
 public:

  RunCmdService_getAlgMgr_result(const RunCmdService_getAlgMgr_result&);
  RunCmdService_getAlgMgr_result(RunCmdService_getAlgMgr_result&&);
  RunCmdService_getAlgMgr_result& operator=(const RunCmdService_getAlgMgr_result&);
  RunCmdService_getAlgMgr_result& operator=(RunCmdService_getAlgMgr_result&&);
  RunCmdService_getAlgMgr_result() : success() {
  }

  virtual ~RunCmdService_getAlgMgr_result() throw();
  std::string success;

  _RunCmdService_getAlgMgr_result__isset __isset;

  void __set_success(const std::string& val);

  template <class Protocol_>
  uint32_t read(Protocol_* iprot);
  template <class Protocol_>
  uint32_t write(Protocol_* oprot) const;

};

typedef struct _RunCmdService_getAlgMgr_presult__isset {
  _RunCmdService_getAlgMgr_presult__isset() : success(false) {}
  bool success :1;
} _RunCmdService_getAlgMgr_presult__isset;

class RunCmdService_getAlgMgr_presult {
 public:


  virtual ~RunCmdService_getAlgMgr_presult() throw();
  std::string* success;

  _RunCmdService_getAlgMgr_presult__isset __isset;

  template <class Protocol_>
  uint32_t read(Protocol_* iprot);

};

template <class Protocol_>
class RunCmdServiceClientT : virtual public RunCmdServiceIf {
 public:
  RunCmdServiceClientT(boost::shared_ptr< Protocol_> prot) {
    setProtocolT(prot);
  }
  RunCmdServiceClientT(boost::shared_ptr< Protocol_> iprot, boost::shared_ptr< Protocol_> oprot) {
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
  void readCmd(CmdResult& _return, const std::string& cmd);
  void send_readCmd(const std::string& cmd);
  void recv_readCmd(CmdResult& _return);
  int32_t runCmd(const std::string& cmd);
  void send_runCmd(const std::string& cmd);
  int32_t recv_runCmd();
  void getAlgMgr(std::string& _return);
  void send_getAlgMgr();
  void recv_getAlgMgr(std::string& _return);
 protected:
  boost::shared_ptr< Protocol_> piprot_;
  boost::shared_ptr< Protocol_> poprot_;
  Protocol_* iprot_;
  Protocol_* oprot_;
};

typedef RunCmdServiceClientT< ::apache::thrift::protocol::TProtocol> RunCmdServiceClient;

template <class Protocol_>
class RunCmdServiceProcessorT : public ::apache::thrift::TDispatchProcessorT<Protocol_> {
 protected:
  boost::shared_ptr<RunCmdServiceIf> iface_;
  virtual bool dispatchCall(::apache::thrift::protocol::TProtocol* iprot, ::apache::thrift::protocol::TProtocol* oprot, const std::string& fname, int32_t seqid, void* callContext);
  virtual bool dispatchCallTemplated(Protocol_* iprot, Protocol_* oprot, const std::string& fname, int32_t seqid, void* callContext);
 private:
  typedef  void (RunCmdServiceProcessorT::*ProcessFunction)(int32_t, ::apache::thrift::protocol::TProtocol*, ::apache::thrift::protocol::TProtocol*, void*);
  typedef void (RunCmdServiceProcessorT::*SpecializedProcessFunction)(int32_t, Protocol_*, Protocol_*, void*);
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
  void process_readCmd(int32_t seqid, ::apache::thrift::protocol::TProtocol* iprot, ::apache::thrift::protocol::TProtocol* oprot, void* callContext);
  void process_readCmd(int32_t seqid, Protocol_* iprot, Protocol_* oprot, void* callContext);
  void process_runCmd(int32_t seqid, ::apache::thrift::protocol::TProtocol* iprot, ::apache::thrift::protocol::TProtocol* oprot, void* callContext);
  void process_runCmd(int32_t seqid, Protocol_* iprot, Protocol_* oprot, void* callContext);
  void process_getAlgMgr(int32_t seqid, ::apache::thrift::protocol::TProtocol* iprot, ::apache::thrift::protocol::TProtocol* oprot, void* callContext);
  void process_getAlgMgr(int32_t seqid, Protocol_* iprot, Protocol_* oprot, void* callContext);
 public:
  RunCmdServiceProcessorT(boost::shared_ptr<RunCmdServiceIf> iface) :
    iface_(iface) {
    processMap_["readCmd"] = ProcessFunctions(
      &RunCmdServiceProcessorT::process_readCmd,
      &RunCmdServiceProcessorT::process_readCmd);
    processMap_["runCmd"] = ProcessFunctions(
      &RunCmdServiceProcessorT::process_runCmd,
      &RunCmdServiceProcessorT::process_runCmd);
    processMap_["getAlgMgr"] = ProcessFunctions(
      &RunCmdServiceProcessorT::process_getAlgMgr,
      &RunCmdServiceProcessorT::process_getAlgMgr);
  }

  virtual ~RunCmdServiceProcessorT() {}
};

typedef RunCmdServiceProcessorT< ::apache::thrift::protocol::TDummyProtocol > RunCmdServiceProcessor;

template <class Protocol_>
class RunCmdServiceProcessorFactoryT : public ::apache::thrift::TProcessorFactory {
 public:
  RunCmdServiceProcessorFactoryT(const ::boost::shared_ptr< RunCmdServiceIfFactory >& handlerFactory) :
      handlerFactory_(handlerFactory) {}

  ::boost::shared_ptr< ::apache::thrift::TProcessor > getProcessor(const ::apache::thrift::TConnectionInfo& connInfo);

 protected:
  ::boost::shared_ptr< RunCmdServiceIfFactory > handlerFactory_;
};

typedef RunCmdServiceProcessorFactoryT< ::apache::thrift::protocol::TDummyProtocol > RunCmdServiceProcessorFactory;

class RunCmdServiceMultiface : virtual public RunCmdServiceIf {
 public:
  RunCmdServiceMultiface(std::vector<boost::shared_ptr<RunCmdServiceIf> >& ifaces) : ifaces_(ifaces) {
  }
  virtual ~RunCmdServiceMultiface() {}
 protected:
  std::vector<boost::shared_ptr<RunCmdServiceIf> > ifaces_;
  RunCmdServiceMultiface() {}
  void add(boost::shared_ptr<RunCmdServiceIf> iface) {
    ifaces_.push_back(iface);
  }
 public:
  void readCmd(CmdResult& _return, const std::string& cmd) {
    size_t sz = ifaces_.size();
    size_t i = 0;
    for (; i < (sz - 1); ++i) {
      ifaces_[i]->readCmd(_return, cmd);
    }
    ifaces_[i]->readCmd(_return, cmd);
    return;
  }

  int32_t runCmd(const std::string& cmd) {
    size_t sz = ifaces_.size();
    size_t i = 0;
    for (; i < (sz - 1); ++i) {
      ifaces_[i]->runCmd(cmd);
    }
    return ifaces_[i]->runCmd(cmd);
  }

  void getAlgMgr(std::string& _return) {
    size_t sz = ifaces_.size();
    size_t i = 0;
    for (; i < (sz - 1); ++i) {
      ifaces_[i]->getAlgMgr(_return);
    }
    ifaces_[i]->getAlgMgr(_return);
    return;
  }

};

// The 'concurrent' client is a thread safe client that correctly handles
// out of order responses.  It is slower than the regular client, so should
// only be used when you need to share a connection among multiple threads
template <class Protocol_>
class RunCmdServiceConcurrentClientT : virtual public RunCmdServiceIf {
 public:
  RunCmdServiceConcurrentClientT(boost::shared_ptr< Protocol_> prot) {
    setProtocolT(prot);
  }
  RunCmdServiceConcurrentClientT(boost::shared_ptr< Protocol_> iprot, boost::shared_ptr< Protocol_> oprot) {
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
  void readCmd(CmdResult& _return, const std::string& cmd);
  int32_t send_readCmd(const std::string& cmd);
  void recv_readCmd(CmdResult& _return, const int32_t seqid);
  int32_t runCmd(const std::string& cmd);
  int32_t send_runCmd(const std::string& cmd);
  int32_t recv_runCmd(const int32_t seqid);
  void getAlgMgr(std::string& _return);
  int32_t send_getAlgMgr();
  void recv_getAlgMgr(std::string& _return, const int32_t seqid);
 protected:
  boost::shared_ptr< Protocol_> piprot_;
  boost::shared_ptr< Protocol_> poprot_;
  Protocol_* iprot_;
  Protocol_* oprot_;
  ::apache::thrift::async::TConcurrentClientSyncInfo sync_;
};

typedef RunCmdServiceConcurrentClientT< ::apache::thrift::protocol::TProtocol> RunCmdServiceConcurrentClient;

#ifdef _WIN32
  #pragma warning( pop )
#endif

} // namespace

#include "RunCmdService.tcc"
#include "RunCmd_types.tcc"

#endif
