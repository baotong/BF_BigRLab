/**
 * Autogenerated by Thrift Compiler (0.9.3)
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */
#ifndef ArticleService_H
#define ArticleService_H

#include <thrift/TDispatchProcessor.h>
#include <thrift/async/TConcurrentClientSyncInfo.h>
#include "article_types.h"

namespace Article {

#ifdef _WIN32
  #pragma warning( push )
  #pragma warning (disable : 4250 ) //inheriting methods via dominance 
#endif

class ArticleServiceIf {
 public:
  virtual ~ArticleServiceIf() {}
  virtual void setFilter(const std::string& filter) = 0;
  virtual void wordSegment(std::vector<std::string> & _return, const std::string& sentence) = 0;
  virtual void keyword(std::vector<KeywordResult> & _return, const std::string& sentence, const int32_t k) = 0;
  virtual void toVector(std::vector<double> & _return, const std::string& sentence, const bool wordseg) = 0;
  virtual void knn(std::vector<KnnResult> & _return, const std::string& sentence, const int32_t n, const int32_t searchK, const bool wordseg, const std::string& reqtype) = 0;
  virtual void handleRequest(std::string& _return, const std::string& request) = 0;
};

class ArticleServiceIfFactory {
 public:
  typedef ArticleServiceIf Handler;

  virtual ~ArticleServiceIfFactory() {}

  virtual ArticleServiceIf* getHandler(const ::apache::thrift::TConnectionInfo& connInfo) = 0;
  virtual void releaseHandler(ArticleServiceIf* /* handler */) = 0;
};

class ArticleServiceIfSingletonFactory : virtual public ArticleServiceIfFactory {
 public:
  ArticleServiceIfSingletonFactory(const boost::shared_ptr<ArticleServiceIf>& iface) : iface_(iface) {}
  virtual ~ArticleServiceIfSingletonFactory() {}

  virtual ArticleServiceIf* getHandler(const ::apache::thrift::TConnectionInfo&) {
    return iface_.get();
  }
  virtual void releaseHandler(ArticleServiceIf* /* handler */) {}

 protected:
  boost::shared_ptr<ArticleServiceIf> iface_;
};

class ArticleServiceNull : virtual public ArticleServiceIf {
 public:
  virtual ~ArticleServiceNull() {}
  void setFilter(const std::string& /* filter */) {
    return;
  }
  void wordSegment(std::vector<std::string> & /* _return */, const std::string& /* sentence */) {
    return;
  }
  void keyword(std::vector<KeywordResult> & /* _return */, const std::string& /* sentence */, const int32_t /* k */) {
    return;
  }
  void toVector(std::vector<double> & /* _return */, const std::string& /* sentence */, const bool /* wordseg */) {
    return;
  }
  void knn(std::vector<KnnResult> & /* _return */, const std::string& /* sentence */, const int32_t /* n */, const int32_t /* searchK */, const bool /* wordseg */, const std::string& /* reqtype */) {
    return;
  }
  void handleRequest(std::string& /* _return */, const std::string& /* request */) {
    return;
  }
};

typedef struct _ArticleService_setFilter_args__isset {
  _ArticleService_setFilter_args__isset() : filter(false) {}
  bool filter :1;
} _ArticleService_setFilter_args__isset;

class ArticleService_setFilter_args {
 public:

  ArticleService_setFilter_args(const ArticleService_setFilter_args&);
  ArticleService_setFilter_args(ArticleService_setFilter_args&&);
  ArticleService_setFilter_args& operator=(const ArticleService_setFilter_args&);
  ArticleService_setFilter_args& operator=(ArticleService_setFilter_args&&);
  ArticleService_setFilter_args() : filter() {
  }

  virtual ~ArticleService_setFilter_args() throw();
  std::string filter;

  _ArticleService_setFilter_args__isset __isset;

  void __set_filter(const std::string& val);

  template <class Protocol_>
  uint32_t read(Protocol_* iprot);
  template <class Protocol_>
  uint32_t write(Protocol_* oprot) const;

};


class ArticleService_setFilter_pargs {
 public:


  virtual ~ArticleService_setFilter_pargs() throw();
  const std::string* filter;

  template <class Protocol_>
  uint32_t write(Protocol_* oprot) const;

};


class ArticleService_setFilter_result {
 public:

  ArticleService_setFilter_result(const ArticleService_setFilter_result&);
  ArticleService_setFilter_result(ArticleService_setFilter_result&&);
  ArticleService_setFilter_result& operator=(const ArticleService_setFilter_result&);
  ArticleService_setFilter_result& operator=(ArticleService_setFilter_result&&);
  ArticleService_setFilter_result() {
  }

  virtual ~ArticleService_setFilter_result() throw();

  template <class Protocol_>
  uint32_t read(Protocol_* iprot);
  template <class Protocol_>
  uint32_t write(Protocol_* oprot) const;

};


class ArticleService_setFilter_presult {
 public:


  virtual ~ArticleService_setFilter_presult() throw();

  template <class Protocol_>
  uint32_t read(Protocol_* iprot);

};

typedef struct _ArticleService_wordSegment_args__isset {
  _ArticleService_wordSegment_args__isset() : sentence(false) {}
  bool sentence :1;
} _ArticleService_wordSegment_args__isset;

class ArticleService_wordSegment_args {
 public:

  ArticleService_wordSegment_args(const ArticleService_wordSegment_args&);
  ArticleService_wordSegment_args(ArticleService_wordSegment_args&&);
  ArticleService_wordSegment_args& operator=(const ArticleService_wordSegment_args&);
  ArticleService_wordSegment_args& operator=(ArticleService_wordSegment_args&&);
  ArticleService_wordSegment_args() : sentence() {
  }

  virtual ~ArticleService_wordSegment_args() throw();
  std::string sentence;

  _ArticleService_wordSegment_args__isset __isset;

  void __set_sentence(const std::string& val);

  template <class Protocol_>
  uint32_t read(Protocol_* iprot);
  template <class Protocol_>
  uint32_t write(Protocol_* oprot) const;

};


class ArticleService_wordSegment_pargs {
 public:


  virtual ~ArticleService_wordSegment_pargs() throw();
  const std::string* sentence;

  template <class Protocol_>
  uint32_t write(Protocol_* oprot) const;

};

typedef struct _ArticleService_wordSegment_result__isset {
  _ArticleService_wordSegment_result__isset() : success(false), err(false) {}
  bool success :1;
  bool err :1;
} _ArticleService_wordSegment_result__isset;

class ArticleService_wordSegment_result {
 public:

  ArticleService_wordSegment_result(const ArticleService_wordSegment_result&);
  ArticleService_wordSegment_result(ArticleService_wordSegment_result&&);
  ArticleService_wordSegment_result& operator=(const ArticleService_wordSegment_result&);
  ArticleService_wordSegment_result& operator=(ArticleService_wordSegment_result&&);
  ArticleService_wordSegment_result() {
  }

  virtual ~ArticleService_wordSegment_result() throw();
  std::vector<std::string>  success;
  InvalidRequest err;

  _ArticleService_wordSegment_result__isset __isset;

  void __set_success(const std::vector<std::string> & val);

  void __set_err(const InvalidRequest& val);

  template <class Protocol_>
  uint32_t read(Protocol_* iprot);
  template <class Protocol_>
  uint32_t write(Protocol_* oprot) const;

};

typedef struct _ArticleService_wordSegment_presult__isset {
  _ArticleService_wordSegment_presult__isset() : success(false), err(false) {}
  bool success :1;
  bool err :1;
} _ArticleService_wordSegment_presult__isset;

class ArticleService_wordSegment_presult {
 public:


  virtual ~ArticleService_wordSegment_presult() throw();
  std::vector<std::string> * success;
  InvalidRequest err;

  _ArticleService_wordSegment_presult__isset __isset;

  template <class Protocol_>
  uint32_t read(Protocol_* iprot);

};

typedef struct _ArticleService_keyword_args__isset {
  _ArticleService_keyword_args__isset() : sentence(false), k(false) {}
  bool sentence :1;
  bool k :1;
} _ArticleService_keyword_args__isset;

class ArticleService_keyword_args {
 public:

  ArticleService_keyword_args(const ArticleService_keyword_args&);
  ArticleService_keyword_args(ArticleService_keyword_args&&);
  ArticleService_keyword_args& operator=(const ArticleService_keyword_args&);
  ArticleService_keyword_args& operator=(ArticleService_keyword_args&&);
  ArticleService_keyword_args() : sentence(), k(0) {
  }

  virtual ~ArticleService_keyword_args() throw();
  std::string sentence;
  int32_t k;

  _ArticleService_keyword_args__isset __isset;

  void __set_sentence(const std::string& val);

  void __set_k(const int32_t val);

  template <class Protocol_>
  uint32_t read(Protocol_* iprot);
  template <class Protocol_>
  uint32_t write(Protocol_* oprot) const;

};


class ArticleService_keyword_pargs {
 public:


  virtual ~ArticleService_keyword_pargs() throw();
  const std::string* sentence;
  const int32_t* k;

  template <class Protocol_>
  uint32_t write(Protocol_* oprot) const;

};

typedef struct _ArticleService_keyword_result__isset {
  _ArticleService_keyword_result__isset() : success(false), err(false) {}
  bool success :1;
  bool err :1;
} _ArticleService_keyword_result__isset;

class ArticleService_keyword_result {
 public:

  ArticleService_keyword_result(const ArticleService_keyword_result&);
  ArticleService_keyword_result(ArticleService_keyword_result&&);
  ArticleService_keyword_result& operator=(const ArticleService_keyword_result&);
  ArticleService_keyword_result& operator=(ArticleService_keyword_result&&);
  ArticleService_keyword_result() {
  }

  virtual ~ArticleService_keyword_result() throw();
  std::vector<KeywordResult>  success;
  InvalidRequest err;

  _ArticleService_keyword_result__isset __isset;

  void __set_success(const std::vector<KeywordResult> & val);

  void __set_err(const InvalidRequest& val);

  template <class Protocol_>
  uint32_t read(Protocol_* iprot);
  template <class Protocol_>
  uint32_t write(Protocol_* oprot) const;

};

typedef struct _ArticleService_keyword_presult__isset {
  _ArticleService_keyword_presult__isset() : success(false), err(false) {}
  bool success :1;
  bool err :1;
} _ArticleService_keyword_presult__isset;

class ArticleService_keyword_presult {
 public:


  virtual ~ArticleService_keyword_presult() throw();
  std::vector<KeywordResult> * success;
  InvalidRequest err;

  _ArticleService_keyword_presult__isset __isset;

  template <class Protocol_>
  uint32_t read(Protocol_* iprot);

};

typedef struct _ArticleService_toVector_args__isset {
  _ArticleService_toVector_args__isset() : sentence(false), wordseg(false) {}
  bool sentence :1;
  bool wordseg :1;
} _ArticleService_toVector_args__isset;

class ArticleService_toVector_args {
 public:

  ArticleService_toVector_args(const ArticleService_toVector_args&);
  ArticleService_toVector_args(ArticleService_toVector_args&&);
  ArticleService_toVector_args& operator=(const ArticleService_toVector_args&);
  ArticleService_toVector_args& operator=(ArticleService_toVector_args&&);
  ArticleService_toVector_args() : sentence(), wordseg(0) {
  }

  virtual ~ArticleService_toVector_args() throw();
  std::string sentence;
  bool wordseg;

  _ArticleService_toVector_args__isset __isset;

  void __set_sentence(const std::string& val);

  void __set_wordseg(const bool val);

  template <class Protocol_>
  uint32_t read(Protocol_* iprot);
  template <class Protocol_>
  uint32_t write(Protocol_* oprot) const;

};


class ArticleService_toVector_pargs {
 public:


  virtual ~ArticleService_toVector_pargs() throw();
  const std::string* sentence;
  const bool* wordseg;

  template <class Protocol_>
  uint32_t write(Protocol_* oprot) const;

};

typedef struct _ArticleService_toVector_result__isset {
  _ArticleService_toVector_result__isset() : success(false), err(false) {}
  bool success :1;
  bool err :1;
} _ArticleService_toVector_result__isset;

class ArticleService_toVector_result {
 public:

  ArticleService_toVector_result(const ArticleService_toVector_result&);
  ArticleService_toVector_result(ArticleService_toVector_result&&);
  ArticleService_toVector_result& operator=(const ArticleService_toVector_result&);
  ArticleService_toVector_result& operator=(ArticleService_toVector_result&&);
  ArticleService_toVector_result() {
  }

  virtual ~ArticleService_toVector_result() throw();
  std::vector<double>  success;
  InvalidRequest err;

  _ArticleService_toVector_result__isset __isset;

  void __set_success(const std::vector<double> & val);

  void __set_err(const InvalidRequest& val);

  template <class Protocol_>
  uint32_t read(Protocol_* iprot);
  template <class Protocol_>
  uint32_t write(Protocol_* oprot) const;

};

typedef struct _ArticleService_toVector_presult__isset {
  _ArticleService_toVector_presult__isset() : success(false), err(false) {}
  bool success :1;
  bool err :1;
} _ArticleService_toVector_presult__isset;

class ArticleService_toVector_presult {
 public:


  virtual ~ArticleService_toVector_presult() throw();
  std::vector<double> * success;
  InvalidRequest err;

  _ArticleService_toVector_presult__isset __isset;

  template <class Protocol_>
  uint32_t read(Protocol_* iprot);

};

typedef struct _ArticleService_knn_args__isset {
  _ArticleService_knn_args__isset() : sentence(false), n(false), searchK(false), wordseg(false), reqtype(false) {}
  bool sentence :1;
  bool n :1;
  bool searchK :1;
  bool wordseg :1;
  bool reqtype :1;
} _ArticleService_knn_args__isset;

class ArticleService_knn_args {
 public:

  ArticleService_knn_args(const ArticleService_knn_args&);
  ArticleService_knn_args(ArticleService_knn_args&&);
  ArticleService_knn_args& operator=(const ArticleService_knn_args&);
  ArticleService_knn_args& operator=(ArticleService_knn_args&&);
  ArticleService_knn_args() : sentence(), n(0), searchK(0), wordseg(0), reqtype() {
  }

  virtual ~ArticleService_knn_args() throw();
  std::string sentence;
  int32_t n;
  int32_t searchK;
  bool wordseg;
  std::string reqtype;

  _ArticleService_knn_args__isset __isset;

  void __set_sentence(const std::string& val);

  void __set_n(const int32_t val);

  void __set_searchK(const int32_t val);

  void __set_wordseg(const bool val);

  void __set_reqtype(const std::string& val);

  template <class Protocol_>
  uint32_t read(Protocol_* iprot);
  template <class Protocol_>
  uint32_t write(Protocol_* oprot) const;

};


class ArticleService_knn_pargs {
 public:


  virtual ~ArticleService_knn_pargs() throw();
  const std::string* sentence;
  const int32_t* n;
  const int32_t* searchK;
  const bool* wordseg;
  const std::string* reqtype;

  template <class Protocol_>
  uint32_t write(Protocol_* oprot) const;

};

typedef struct _ArticleService_knn_result__isset {
  _ArticleService_knn_result__isset() : success(false), err(false) {}
  bool success :1;
  bool err :1;
} _ArticleService_knn_result__isset;

class ArticleService_knn_result {
 public:

  ArticleService_knn_result(const ArticleService_knn_result&);
  ArticleService_knn_result(ArticleService_knn_result&&);
  ArticleService_knn_result& operator=(const ArticleService_knn_result&);
  ArticleService_knn_result& operator=(ArticleService_knn_result&&);
  ArticleService_knn_result() {
  }

  virtual ~ArticleService_knn_result() throw();
  std::vector<KnnResult>  success;
  InvalidRequest err;

  _ArticleService_knn_result__isset __isset;

  void __set_success(const std::vector<KnnResult> & val);

  void __set_err(const InvalidRequest& val);

  template <class Protocol_>
  uint32_t read(Protocol_* iprot);
  template <class Protocol_>
  uint32_t write(Protocol_* oprot) const;

};

typedef struct _ArticleService_knn_presult__isset {
  _ArticleService_knn_presult__isset() : success(false), err(false) {}
  bool success :1;
  bool err :1;
} _ArticleService_knn_presult__isset;

class ArticleService_knn_presult {
 public:


  virtual ~ArticleService_knn_presult() throw();
  std::vector<KnnResult> * success;
  InvalidRequest err;

  _ArticleService_knn_presult__isset __isset;

  template <class Protocol_>
  uint32_t read(Protocol_* iprot);

};

typedef struct _ArticleService_handleRequest_args__isset {
  _ArticleService_handleRequest_args__isset() : request(false) {}
  bool request :1;
} _ArticleService_handleRequest_args__isset;

class ArticleService_handleRequest_args {
 public:

  ArticleService_handleRequest_args(const ArticleService_handleRequest_args&);
  ArticleService_handleRequest_args(ArticleService_handleRequest_args&&);
  ArticleService_handleRequest_args& operator=(const ArticleService_handleRequest_args&);
  ArticleService_handleRequest_args& operator=(ArticleService_handleRequest_args&&);
  ArticleService_handleRequest_args() : request() {
  }

  virtual ~ArticleService_handleRequest_args() throw();
  std::string request;

  _ArticleService_handleRequest_args__isset __isset;

  void __set_request(const std::string& val);

  template <class Protocol_>
  uint32_t read(Protocol_* iprot);
  template <class Protocol_>
  uint32_t write(Protocol_* oprot) const;

};


class ArticleService_handleRequest_pargs {
 public:


  virtual ~ArticleService_handleRequest_pargs() throw();
  const std::string* request;

  template <class Protocol_>
  uint32_t write(Protocol_* oprot) const;

};

typedef struct _ArticleService_handleRequest_result__isset {
  _ArticleService_handleRequest_result__isset() : success(false), err(false) {}
  bool success :1;
  bool err :1;
} _ArticleService_handleRequest_result__isset;

class ArticleService_handleRequest_result {
 public:

  ArticleService_handleRequest_result(const ArticleService_handleRequest_result&);
  ArticleService_handleRequest_result(ArticleService_handleRequest_result&&);
  ArticleService_handleRequest_result& operator=(const ArticleService_handleRequest_result&);
  ArticleService_handleRequest_result& operator=(ArticleService_handleRequest_result&&);
  ArticleService_handleRequest_result() : success() {
  }

  virtual ~ArticleService_handleRequest_result() throw();
  std::string success;
  InvalidRequest err;

  _ArticleService_handleRequest_result__isset __isset;

  void __set_success(const std::string& val);

  void __set_err(const InvalidRequest& val);

  template <class Protocol_>
  uint32_t read(Protocol_* iprot);
  template <class Protocol_>
  uint32_t write(Protocol_* oprot) const;

};

typedef struct _ArticleService_handleRequest_presult__isset {
  _ArticleService_handleRequest_presult__isset() : success(false), err(false) {}
  bool success :1;
  bool err :1;
} _ArticleService_handleRequest_presult__isset;

class ArticleService_handleRequest_presult {
 public:


  virtual ~ArticleService_handleRequest_presult() throw();
  std::string* success;
  InvalidRequest err;

  _ArticleService_handleRequest_presult__isset __isset;

  template <class Protocol_>
  uint32_t read(Protocol_* iprot);

};

template <class Protocol_>
class ArticleServiceClientT : virtual public ArticleServiceIf {
 public:
  ArticleServiceClientT(boost::shared_ptr< Protocol_> prot) {
    setProtocolT(prot);
  }
  ArticleServiceClientT(boost::shared_ptr< Protocol_> iprot, boost::shared_ptr< Protocol_> oprot) {
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
  void setFilter(const std::string& filter);
  void send_setFilter(const std::string& filter);
  void recv_setFilter();
  void wordSegment(std::vector<std::string> & _return, const std::string& sentence);
  void send_wordSegment(const std::string& sentence);
  void recv_wordSegment(std::vector<std::string> & _return);
  void keyword(std::vector<KeywordResult> & _return, const std::string& sentence, const int32_t k);
  void send_keyword(const std::string& sentence, const int32_t k);
  void recv_keyword(std::vector<KeywordResult> & _return);
  void toVector(std::vector<double> & _return, const std::string& sentence, const bool wordseg);
  void send_toVector(const std::string& sentence, const bool wordseg);
  void recv_toVector(std::vector<double> & _return);
  void knn(std::vector<KnnResult> & _return, const std::string& sentence, const int32_t n, const int32_t searchK, const bool wordseg, const std::string& reqtype);
  void send_knn(const std::string& sentence, const int32_t n, const int32_t searchK, const bool wordseg, const std::string& reqtype);
  void recv_knn(std::vector<KnnResult> & _return);
  void handleRequest(std::string& _return, const std::string& request);
  void send_handleRequest(const std::string& request);
  void recv_handleRequest(std::string& _return);
 protected:
  boost::shared_ptr< Protocol_> piprot_;
  boost::shared_ptr< Protocol_> poprot_;
  Protocol_* iprot_;
  Protocol_* oprot_;
};

typedef ArticleServiceClientT< ::apache::thrift::protocol::TProtocol> ArticleServiceClient;

template <class Protocol_>
class ArticleServiceProcessorT : public ::apache::thrift::TDispatchProcessorT<Protocol_> {
 protected:
  boost::shared_ptr<ArticleServiceIf> iface_;
  virtual bool dispatchCall(::apache::thrift::protocol::TProtocol* iprot, ::apache::thrift::protocol::TProtocol* oprot, const std::string& fname, int32_t seqid, void* callContext);
  virtual bool dispatchCallTemplated(Protocol_* iprot, Protocol_* oprot, const std::string& fname, int32_t seqid, void* callContext);
 private:
  typedef  void (ArticleServiceProcessorT::*ProcessFunction)(int32_t, ::apache::thrift::protocol::TProtocol*, ::apache::thrift::protocol::TProtocol*, void*);
  typedef void (ArticleServiceProcessorT::*SpecializedProcessFunction)(int32_t, Protocol_*, Protocol_*, void*);
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
  void process_setFilter(int32_t seqid, ::apache::thrift::protocol::TProtocol* iprot, ::apache::thrift::protocol::TProtocol* oprot, void* callContext);
  void process_setFilter(int32_t seqid, Protocol_* iprot, Protocol_* oprot, void* callContext);
  void process_wordSegment(int32_t seqid, ::apache::thrift::protocol::TProtocol* iprot, ::apache::thrift::protocol::TProtocol* oprot, void* callContext);
  void process_wordSegment(int32_t seqid, Protocol_* iprot, Protocol_* oprot, void* callContext);
  void process_keyword(int32_t seqid, ::apache::thrift::protocol::TProtocol* iprot, ::apache::thrift::protocol::TProtocol* oprot, void* callContext);
  void process_keyword(int32_t seqid, Protocol_* iprot, Protocol_* oprot, void* callContext);
  void process_toVector(int32_t seqid, ::apache::thrift::protocol::TProtocol* iprot, ::apache::thrift::protocol::TProtocol* oprot, void* callContext);
  void process_toVector(int32_t seqid, Protocol_* iprot, Protocol_* oprot, void* callContext);
  void process_knn(int32_t seqid, ::apache::thrift::protocol::TProtocol* iprot, ::apache::thrift::protocol::TProtocol* oprot, void* callContext);
  void process_knn(int32_t seqid, Protocol_* iprot, Protocol_* oprot, void* callContext);
  void process_handleRequest(int32_t seqid, ::apache::thrift::protocol::TProtocol* iprot, ::apache::thrift::protocol::TProtocol* oprot, void* callContext);
  void process_handleRequest(int32_t seqid, Protocol_* iprot, Protocol_* oprot, void* callContext);
 public:
  ArticleServiceProcessorT(boost::shared_ptr<ArticleServiceIf> iface) :
    iface_(iface) {
    processMap_["setFilter"] = ProcessFunctions(
      &ArticleServiceProcessorT::process_setFilter,
      &ArticleServiceProcessorT::process_setFilter);
    processMap_["wordSegment"] = ProcessFunctions(
      &ArticleServiceProcessorT::process_wordSegment,
      &ArticleServiceProcessorT::process_wordSegment);
    processMap_["keyword"] = ProcessFunctions(
      &ArticleServiceProcessorT::process_keyword,
      &ArticleServiceProcessorT::process_keyword);
    processMap_["toVector"] = ProcessFunctions(
      &ArticleServiceProcessorT::process_toVector,
      &ArticleServiceProcessorT::process_toVector);
    processMap_["knn"] = ProcessFunctions(
      &ArticleServiceProcessorT::process_knn,
      &ArticleServiceProcessorT::process_knn);
    processMap_["handleRequest"] = ProcessFunctions(
      &ArticleServiceProcessorT::process_handleRequest,
      &ArticleServiceProcessorT::process_handleRequest);
  }

  virtual ~ArticleServiceProcessorT() {}
};

typedef ArticleServiceProcessorT< ::apache::thrift::protocol::TDummyProtocol > ArticleServiceProcessor;

template <class Protocol_>
class ArticleServiceProcessorFactoryT : public ::apache::thrift::TProcessorFactory {
 public:
  ArticleServiceProcessorFactoryT(const ::boost::shared_ptr< ArticleServiceIfFactory >& handlerFactory) :
      handlerFactory_(handlerFactory) {}

  ::boost::shared_ptr< ::apache::thrift::TProcessor > getProcessor(const ::apache::thrift::TConnectionInfo& connInfo);

 protected:
  ::boost::shared_ptr< ArticleServiceIfFactory > handlerFactory_;
};

typedef ArticleServiceProcessorFactoryT< ::apache::thrift::protocol::TDummyProtocol > ArticleServiceProcessorFactory;

class ArticleServiceMultiface : virtual public ArticleServiceIf {
 public:
  ArticleServiceMultiface(std::vector<boost::shared_ptr<ArticleServiceIf> >& ifaces) : ifaces_(ifaces) {
  }
  virtual ~ArticleServiceMultiface() {}
 protected:
  std::vector<boost::shared_ptr<ArticleServiceIf> > ifaces_;
  ArticleServiceMultiface() {}
  void add(boost::shared_ptr<ArticleServiceIf> iface) {
    ifaces_.push_back(iface);
  }
 public:
  void setFilter(const std::string& filter) {
    size_t sz = ifaces_.size();
    size_t i = 0;
    for (; i < (sz - 1); ++i) {
      ifaces_[i]->setFilter(filter);
    }
    ifaces_[i]->setFilter(filter);
  }

  void wordSegment(std::vector<std::string> & _return, const std::string& sentence) {
    size_t sz = ifaces_.size();
    size_t i = 0;
    for (; i < (sz - 1); ++i) {
      ifaces_[i]->wordSegment(_return, sentence);
    }
    ifaces_[i]->wordSegment(_return, sentence);
    return;
  }

  void keyword(std::vector<KeywordResult> & _return, const std::string& sentence, const int32_t k) {
    size_t sz = ifaces_.size();
    size_t i = 0;
    for (; i < (sz - 1); ++i) {
      ifaces_[i]->keyword(_return, sentence, k);
    }
    ifaces_[i]->keyword(_return, sentence, k);
    return;
  }

  void toVector(std::vector<double> & _return, const std::string& sentence, const bool wordseg) {
    size_t sz = ifaces_.size();
    size_t i = 0;
    for (; i < (sz - 1); ++i) {
      ifaces_[i]->toVector(_return, sentence, wordseg);
    }
    ifaces_[i]->toVector(_return, sentence, wordseg);
    return;
  }

  void knn(std::vector<KnnResult> & _return, const std::string& sentence, const int32_t n, const int32_t searchK, const bool wordseg, const std::string& reqtype) {
    size_t sz = ifaces_.size();
    size_t i = 0;
    for (; i < (sz - 1); ++i) {
      ifaces_[i]->knn(_return, sentence, n, searchK, wordseg, reqtype);
    }
    ifaces_[i]->knn(_return, sentence, n, searchK, wordseg, reqtype);
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
class ArticleServiceConcurrentClientT : virtual public ArticleServiceIf {
 public:
  ArticleServiceConcurrentClientT(boost::shared_ptr< Protocol_> prot) {
    setProtocolT(prot);
  }
  ArticleServiceConcurrentClientT(boost::shared_ptr< Protocol_> iprot, boost::shared_ptr< Protocol_> oprot) {
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
  void setFilter(const std::string& filter);
  int32_t send_setFilter(const std::string& filter);
  void recv_setFilter(const int32_t seqid);
  void wordSegment(std::vector<std::string> & _return, const std::string& sentence);
  int32_t send_wordSegment(const std::string& sentence);
  void recv_wordSegment(std::vector<std::string> & _return, const int32_t seqid);
  void keyword(std::vector<KeywordResult> & _return, const std::string& sentence, const int32_t k);
  int32_t send_keyword(const std::string& sentence, const int32_t k);
  void recv_keyword(std::vector<KeywordResult> & _return, const int32_t seqid);
  void toVector(std::vector<double> & _return, const std::string& sentence, const bool wordseg);
  int32_t send_toVector(const std::string& sentence, const bool wordseg);
  void recv_toVector(std::vector<double> & _return, const int32_t seqid);
  void knn(std::vector<KnnResult> & _return, const std::string& sentence, const int32_t n, const int32_t searchK, const bool wordseg, const std::string& reqtype);
  int32_t send_knn(const std::string& sentence, const int32_t n, const int32_t searchK, const bool wordseg, const std::string& reqtype);
  void recv_knn(std::vector<KnnResult> & _return, const int32_t seqid);
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

typedef ArticleServiceConcurrentClientT< ::apache::thrift::protocol::TProtocol> ArticleServiceConcurrentClient;

#ifdef _WIN32
  #pragma warning( pop )
#endif

} // namespace

#include "ArticleService.tcc"
#include "article_types.tcc"

#endif
