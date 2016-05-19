/**
 * Autogenerated by Thrift Compiler (0.9.3)
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */
#include "KnnService.h"
#ifndef KnnService_TCC
#define KnnService_TCC


namespace KNN {


template <class Protocol_>
uint32_t KnnService_queryByItem_args::read(Protocol_* iprot) {

  apache::thrift::protocol::TInputRecursionTracker tracker(*iprot);
  uint32_t xfer = 0;
  std::string fname;
  ::apache::thrift::protocol::TType ftype;
  int16_t fid;

  xfer += iprot->readStructBegin(fname);

  using ::apache::thrift::protocol::TProtocolException;


  while (true)
  {
    xfer += iprot->readFieldBegin(fname, ftype, fid);
    if (ftype == ::apache::thrift::protocol::T_STOP) {
      break;
    }
    switch (fid)
    {
      case 1:
        if (ftype == ::apache::thrift::protocol::T_STRING) {
          xfer += iprot->readString(this->item);
          this->__isset.item = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      case 2:
        if (ftype == ::apache::thrift::protocol::T_I32) {
          xfer += iprot->readI32(this->n);
          this->__isset.n = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      default:
        xfer += iprot->skip(ftype);
        break;
    }
    xfer += iprot->readFieldEnd();
  }

  xfer += iprot->readStructEnd();

  return xfer;
}

template <class Protocol_>
uint32_t KnnService_queryByItem_args::write(Protocol_* oprot) const {
  uint32_t xfer = 0;
  apache::thrift::protocol::TOutputRecursionTracker tracker(*oprot);
  xfer += oprot->writeStructBegin("KnnService_queryByItem_args");

  xfer += oprot->writeFieldBegin("item", ::apache::thrift::protocol::T_STRING, 1);
  xfer += oprot->writeString(this->item);
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldBegin("n", ::apache::thrift::protocol::T_I32, 2);
  xfer += oprot->writeI32(this->n);
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldStop();
  xfer += oprot->writeStructEnd();
  return xfer;
}


template <class Protocol_>
uint32_t KnnService_queryByItem_pargs::write(Protocol_* oprot) const {
  uint32_t xfer = 0;
  apache::thrift::protocol::TOutputRecursionTracker tracker(*oprot);
  xfer += oprot->writeStructBegin("KnnService_queryByItem_pargs");

  xfer += oprot->writeFieldBegin("item", ::apache::thrift::protocol::T_STRING, 1);
  xfer += oprot->writeString((*(this->item)));
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldBegin("n", ::apache::thrift::protocol::T_I32, 2);
  xfer += oprot->writeI32((*(this->n)));
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldStop();
  xfer += oprot->writeStructEnd();
  return xfer;
}


template <class Protocol_>
uint32_t KnnService_queryByItem_result::read(Protocol_* iprot) {

  apache::thrift::protocol::TInputRecursionTracker tracker(*iprot);
  uint32_t xfer = 0;
  std::string fname;
  ::apache::thrift::protocol::TType ftype;
  int16_t fid;

  xfer += iprot->readStructBegin(fname);

  using ::apache::thrift::protocol::TProtocolException;


  while (true)
  {
    xfer += iprot->readFieldBegin(fname, ftype, fid);
    if (ftype == ::apache::thrift::protocol::T_STOP) {
      break;
    }
    switch (fid)
    {
      case 0:
        if (ftype == ::apache::thrift::protocol::T_LIST) {
          {
            this->success.clear();
            uint32_t _size8;
            ::apache::thrift::protocol::TType _etype11;
            xfer += iprot->readListBegin(_etype11, _size8);
            this->success.resize(_size8);
            uint32_t _i12;
            for (_i12 = 0; _i12 < _size8; ++_i12)
            {
              xfer += this->success[_i12].read(iprot);
            }
            xfer += iprot->readListEnd();
          }
          this->__isset.success = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      case 1:
        if (ftype == ::apache::thrift::protocol::T_STRUCT) {
          xfer += this->err.read(iprot);
          this->__isset.err = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      default:
        xfer += iprot->skip(ftype);
        break;
    }
    xfer += iprot->readFieldEnd();
  }

  xfer += iprot->readStructEnd();

  return xfer;
}

template <class Protocol_>
uint32_t KnnService_queryByItem_result::write(Protocol_* oprot) const {

  uint32_t xfer = 0;

  xfer += oprot->writeStructBegin("KnnService_queryByItem_result");

  if (this->__isset.success) {
    xfer += oprot->writeFieldBegin("success", ::apache::thrift::protocol::T_LIST, 0);
    {
      xfer += oprot->writeListBegin(::apache::thrift::protocol::T_STRUCT, static_cast<uint32_t>(this->success.size()));
      std::vector<Result> ::const_iterator _iter13;
      for (_iter13 = this->success.begin(); _iter13 != this->success.end(); ++_iter13)
      {
        xfer += (*_iter13).write(oprot);
      }
      xfer += oprot->writeListEnd();
    }
    xfer += oprot->writeFieldEnd();
  } else if (this->__isset.err) {
    xfer += oprot->writeFieldBegin("err", ::apache::thrift::protocol::T_STRUCT, 1);
    xfer += this->err.write(oprot);
    xfer += oprot->writeFieldEnd();
  }
  xfer += oprot->writeFieldStop();
  xfer += oprot->writeStructEnd();
  return xfer;
}


template <class Protocol_>
uint32_t KnnService_queryByItem_presult::read(Protocol_* iprot) {

  apache::thrift::protocol::TInputRecursionTracker tracker(*iprot);
  uint32_t xfer = 0;
  std::string fname;
  ::apache::thrift::protocol::TType ftype;
  int16_t fid;

  xfer += iprot->readStructBegin(fname);

  using ::apache::thrift::protocol::TProtocolException;


  while (true)
  {
    xfer += iprot->readFieldBegin(fname, ftype, fid);
    if (ftype == ::apache::thrift::protocol::T_STOP) {
      break;
    }
    switch (fid)
    {
      case 0:
        if (ftype == ::apache::thrift::protocol::T_LIST) {
          {
            (*(this->success)).clear();
            uint32_t _size14;
            ::apache::thrift::protocol::TType _etype17;
            xfer += iprot->readListBegin(_etype17, _size14);
            (*(this->success)).resize(_size14);
            uint32_t _i18;
            for (_i18 = 0; _i18 < _size14; ++_i18)
            {
              xfer += (*(this->success))[_i18].read(iprot);
            }
            xfer += iprot->readListEnd();
          }
          this->__isset.success = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      case 1:
        if (ftype == ::apache::thrift::protocol::T_STRUCT) {
          xfer += this->err.read(iprot);
          this->__isset.err = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      default:
        xfer += iprot->skip(ftype);
        break;
    }
    xfer += iprot->readFieldEnd();
  }

  xfer += iprot->readStructEnd();

  return xfer;
}


template <class Protocol_>
uint32_t KnnService_queryByVector_args::read(Protocol_* iprot) {

  apache::thrift::protocol::TInputRecursionTracker tracker(*iprot);
  uint32_t xfer = 0;
  std::string fname;
  ::apache::thrift::protocol::TType ftype;
  int16_t fid;

  xfer += iprot->readStructBegin(fname);

  using ::apache::thrift::protocol::TProtocolException;


  while (true)
  {
    xfer += iprot->readFieldBegin(fname, ftype, fid);
    if (ftype == ::apache::thrift::protocol::T_STOP) {
      break;
    }
    switch (fid)
    {
      case 1:
        if (ftype == ::apache::thrift::protocol::T_LIST) {
          {
            this->values.clear();
            uint32_t _size19;
            ::apache::thrift::protocol::TType _etype22;
            xfer += iprot->readListBegin(_etype22, _size19);
            this->values.resize(_size19);
            uint32_t _i23;
            for (_i23 = 0; _i23 < _size19; ++_i23)
            {
              xfer += iprot->readDouble(this->values[_i23]);
            }
            xfer += iprot->readListEnd();
          }
          this->__isset.values = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      case 2:
        if (ftype == ::apache::thrift::protocol::T_I32) {
          xfer += iprot->readI32(this->n);
          this->__isset.n = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      default:
        xfer += iprot->skip(ftype);
        break;
    }
    xfer += iprot->readFieldEnd();
  }

  xfer += iprot->readStructEnd();

  return xfer;
}

template <class Protocol_>
uint32_t KnnService_queryByVector_args::write(Protocol_* oprot) const {
  uint32_t xfer = 0;
  apache::thrift::protocol::TOutputRecursionTracker tracker(*oprot);
  xfer += oprot->writeStructBegin("KnnService_queryByVector_args");

  xfer += oprot->writeFieldBegin("values", ::apache::thrift::protocol::T_LIST, 1);
  {
    xfer += oprot->writeListBegin(::apache::thrift::protocol::T_DOUBLE, static_cast<uint32_t>(this->values.size()));
    std::vector<double> ::const_iterator _iter24;
    for (_iter24 = this->values.begin(); _iter24 != this->values.end(); ++_iter24)
    {
      xfer += oprot->writeDouble((*_iter24));
    }
    xfer += oprot->writeListEnd();
  }
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldBegin("n", ::apache::thrift::protocol::T_I32, 2);
  xfer += oprot->writeI32(this->n);
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldStop();
  xfer += oprot->writeStructEnd();
  return xfer;
}


template <class Protocol_>
uint32_t KnnService_queryByVector_pargs::write(Protocol_* oprot) const {
  uint32_t xfer = 0;
  apache::thrift::protocol::TOutputRecursionTracker tracker(*oprot);
  xfer += oprot->writeStructBegin("KnnService_queryByVector_pargs");

  xfer += oprot->writeFieldBegin("values", ::apache::thrift::protocol::T_LIST, 1);
  {
    xfer += oprot->writeListBegin(::apache::thrift::protocol::T_DOUBLE, static_cast<uint32_t>((*(this->values)).size()));
    std::vector<double> ::const_iterator _iter25;
    for (_iter25 = (*(this->values)).begin(); _iter25 != (*(this->values)).end(); ++_iter25)
    {
      xfer += oprot->writeDouble((*_iter25));
    }
    xfer += oprot->writeListEnd();
  }
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldBegin("n", ::apache::thrift::protocol::T_I32, 2);
  xfer += oprot->writeI32((*(this->n)));
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldStop();
  xfer += oprot->writeStructEnd();
  return xfer;
}


template <class Protocol_>
uint32_t KnnService_queryByVector_result::read(Protocol_* iprot) {

  apache::thrift::protocol::TInputRecursionTracker tracker(*iprot);
  uint32_t xfer = 0;
  std::string fname;
  ::apache::thrift::protocol::TType ftype;
  int16_t fid;

  xfer += iprot->readStructBegin(fname);

  using ::apache::thrift::protocol::TProtocolException;


  while (true)
  {
    xfer += iprot->readFieldBegin(fname, ftype, fid);
    if (ftype == ::apache::thrift::protocol::T_STOP) {
      break;
    }
    switch (fid)
    {
      case 0:
        if (ftype == ::apache::thrift::protocol::T_LIST) {
          {
            this->success.clear();
            uint32_t _size26;
            ::apache::thrift::protocol::TType _etype29;
            xfer += iprot->readListBegin(_etype29, _size26);
            this->success.resize(_size26);
            uint32_t _i30;
            for (_i30 = 0; _i30 < _size26; ++_i30)
            {
              xfer += this->success[_i30].read(iprot);
            }
            xfer += iprot->readListEnd();
          }
          this->__isset.success = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      case 1:
        if (ftype == ::apache::thrift::protocol::T_STRUCT) {
          xfer += this->err.read(iprot);
          this->__isset.err = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      default:
        xfer += iprot->skip(ftype);
        break;
    }
    xfer += iprot->readFieldEnd();
  }

  xfer += iprot->readStructEnd();

  return xfer;
}

template <class Protocol_>
uint32_t KnnService_queryByVector_result::write(Protocol_* oprot) const {

  uint32_t xfer = 0;

  xfer += oprot->writeStructBegin("KnnService_queryByVector_result");

  if (this->__isset.success) {
    xfer += oprot->writeFieldBegin("success", ::apache::thrift::protocol::T_LIST, 0);
    {
      xfer += oprot->writeListBegin(::apache::thrift::protocol::T_STRUCT, static_cast<uint32_t>(this->success.size()));
      std::vector<Result> ::const_iterator _iter31;
      for (_iter31 = this->success.begin(); _iter31 != this->success.end(); ++_iter31)
      {
        xfer += (*_iter31).write(oprot);
      }
      xfer += oprot->writeListEnd();
    }
    xfer += oprot->writeFieldEnd();
  } else if (this->__isset.err) {
    xfer += oprot->writeFieldBegin("err", ::apache::thrift::protocol::T_STRUCT, 1);
    xfer += this->err.write(oprot);
    xfer += oprot->writeFieldEnd();
  }
  xfer += oprot->writeFieldStop();
  xfer += oprot->writeStructEnd();
  return xfer;
}


template <class Protocol_>
uint32_t KnnService_queryByVector_presult::read(Protocol_* iprot) {

  apache::thrift::protocol::TInputRecursionTracker tracker(*iprot);
  uint32_t xfer = 0;
  std::string fname;
  ::apache::thrift::protocol::TType ftype;
  int16_t fid;

  xfer += iprot->readStructBegin(fname);

  using ::apache::thrift::protocol::TProtocolException;


  while (true)
  {
    xfer += iprot->readFieldBegin(fname, ftype, fid);
    if (ftype == ::apache::thrift::protocol::T_STOP) {
      break;
    }
    switch (fid)
    {
      case 0:
        if (ftype == ::apache::thrift::protocol::T_LIST) {
          {
            (*(this->success)).clear();
            uint32_t _size32;
            ::apache::thrift::protocol::TType _etype35;
            xfer += iprot->readListBegin(_etype35, _size32);
            (*(this->success)).resize(_size32);
            uint32_t _i36;
            for (_i36 = 0; _i36 < _size32; ++_i36)
            {
              xfer += (*(this->success))[_i36].read(iprot);
            }
            xfer += iprot->readListEnd();
          }
          this->__isset.success = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      case 1:
        if (ftype == ::apache::thrift::protocol::T_STRUCT) {
          xfer += this->err.read(iprot);
          this->__isset.err = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      default:
        xfer += iprot->skip(ftype);
        break;
    }
    xfer += iprot->readFieldEnd();
  }

  xfer += iprot->readStructEnd();

  return xfer;
}

template <class Protocol_>
void KnnServiceClientT<Protocol_>::queryByItem(std::vector<Result> & _return, const std::string& item, const int32_t n)
{
  send_queryByItem(item, n);
  recv_queryByItem(_return);
}

template <class Protocol_>
void KnnServiceClientT<Protocol_>::send_queryByItem(const std::string& item, const int32_t n)
{
  int32_t cseqid = 0;
  this->oprot_->writeMessageBegin("queryByItem", ::apache::thrift::protocol::T_CALL, cseqid);

  KnnService_queryByItem_pargs args;
  args.item = &item;
  args.n = &n;
  args.write(this->oprot_);

  this->oprot_->writeMessageEnd();
  this->oprot_->getTransport()->writeEnd();
  this->oprot_->getTransport()->flush();
}

template <class Protocol_>
void KnnServiceClientT<Protocol_>::recv_queryByItem(std::vector<Result> & _return)
{

  int32_t rseqid = 0;
  std::string fname;
  ::apache::thrift::protocol::TMessageType mtype;

  this->iprot_->readMessageBegin(fname, mtype, rseqid);
  if (mtype == ::apache::thrift::protocol::T_EXCEPTION) {
    ::apache::thrift::TApplicationException x;
    x.read(this->iprot_);
    this->iprot_->readMessageEnd();
    this->iprot_->getTransport()->readEnd();
    throw x;
  }
  if (mtype != ::apache::thrift::protocol::T_REPLY) {
    this->iprot_->skip(::apache::thrift::protocol::T_STRUCT);
    this->iprot_->readMessageEnd();
    this->iprot_->getTransport()->readEnd();
  }
  if (fname.compare("queryByItem") != 0) {
    this->iprot_->skip(::apache::thrift::protocol::T_STRUCT);
    this->iprot_->readMessageEnd();
    this->iprot_->getTransport()->readEnd();
  }
  KnnService_queryByItem_presult result;
  result.success = &_return;
  result.read(this->iprot_);
  this->iprot_->readMessageEnd();
  this->iprot_->getTransport()->readEnd();

  if (result.__isset.success) {
    // _return pointer has now been filled
    return;
  }
  if (result.__isset.err) {
    throw result.err;
  }
  throw ::apache::thrift::TApplicationException(::apache::thrift::TApplicationException::MISSING_RESULT, "queryByItem failed: unknown result");
}

template <class Protocol_>
void KnnServiceClientT<Protocol_>::queryByVector(std::vector<Result> & _return, const std::vector<double> & values, const int32_t n)
{
  send_queryByVector(values, n);
  recv_queryByVector(_return);
}

template <class Protocol_>
void KnnServiceClientT<Protocol_>::send_queryByVector(const std::vector<double> & values, const int32_t n)
{
  int32_t cseqid = 0;
  this->oprot_->writeMessageBegin("queryByVector", ::apache::thrift::protocol::T_CALL, cseqid);

  KnnService_queryByVector_pargs args;
  args.values = &values;
  args.n = &n;
  args.write(this->oprot_);

  this->oprot_->writeMessageEnd();
  this->oprot_->getTransport()->writeEnd();
  this->oprot_->getTransport()->flush();
}

template <class Protocol_>
void KnnServiceClientT<Protocol_>::recv_queryByVector(std::vector<Result> & _return)
{

  int32_t rseqid = 0;
  std::string fname;
  ::apache::thrift::protocol::TMessageType mtype;

  this->iprot_->readMessageBegin(fname, mtype, rseqid);
  if (mtype == ::apache::thrift::protocol::T_EXCEPTION) {
    ::apache::thrift::TApplicationException x;
    x.read(this->iprot_);
    this->iprot_->readMessageEnd();
    this->iprot_->getTransport()->readEnd();
    throw x;
  }
  if (mtype != ::apache::thrift::protocol::T_REPLY) {
    this->iprot_->skip(::apache::thrift::protocol::T_STRUCT);
    this->iprot_->readMessageEnd();
    this->iprot_->getTransport()->readEnd();
  }
  if (fname.compare("queryByVector") != 0) {
    this->iprot_->skip(::apache::thrift::protocol::T_STRUCT);
    this->iprot_->readMessageEnd();
    this->iprot_->getTransport()->readEnd();
  }
  KnnService_queryByVector_presult result;
  result.success = &_return;
  result.read(this->iprot_);
  this->iprot_->readMessageEnd();
  this->iprot_->getTransport()->readEnd();

  if (result.__isset.success) {
    // _return pointer has now been filled
    return;
  }
  if (result.__isset.err) {
    throw result.err;
  }
  throw ::apache::thrift::TApplicationException(::apache::thrift::TApplicationException::MISSING_RESULT, "queryByVector failed: unknown result");
}

template <class Protocol_>
bool KnnServiceProcessorT<Protocol_>::dispatchCall(::apache::thrift::protocol::TProtocol* iprot, ::apache::thrift::protocol::TProtocol* oprot, const std::string& fname, int32_t seqid, void* callContext) {
  typename ProcessMap::iterator pfn;
  pfn = processMap_.find(fname);
  if (pfn == processMap_.end()) {
    iprot->skip(::apache::thrift::protocol::T_STRUCT);
    iprot->readMessageEnd();
    iprot->getTransport()->readEnd();
    ::apache::thrift::TApplicationException x(::apache::thrift::TApplicationException::UNKNOWN_METHOD, "Invalid method name: '"+fname+"'");
    oprot->writeMessageBegin(fname, ::apache::thrift::protocol::T_EXCEPTION, seqid);
    x.write(oprot);
    oprot->writeMessageEnd();
    oprot->getTransport()->writeEnd();
    oprot->getTransport()->flush();
    return true;
  }
  (this->*(pfn->second.generic))(seqid, iprot, oprot, callContext);
  return true;
}

template <class Protocol_>
bool KnnServiceProcessorT<Protocol_>::dispatchCallTemplated(Protocol_* iprot, Protocol_* oprot, const std::string& fname, int32_t seqid, void* callContext) {
  typename ProcessMap::iterator pfn;
  pfn = processMap_.find(fname);
  if (pfn == processMap_.end()) {
    iprot->skip(::apache::thrift::protocol::T_STRUCT);
    iprot->readMessageEnd();
    iprot->getTransport()->readEnd();
    ::apache::thrift::TApplicationException x(::apache::thrift::TApplicationException::UNKNOWN_METHOD, "Invalid method name: '"+fname+"'");
    oprot->writeMessageBegin(fname, ::apache::thrift::protocol::T_EXCEPTION, seqid);
    x.write(oprot);
    oprot->writeMessageEnd();
    oprot->getTransport()->writeEnd();
    oprot->getTransport()->flush();
    return true;
  }
  (this->*(pfn->second.specialized))(seqid, iprot, oprot, callContext);
  return true;
}

template <class Protocol_>
void KnnServiceProcessorT<Protocol_>::process_queryByItem(int32_t seqid, ::apache::thrift::protocol::TProtocol* iprot, ::apache::thrift::protocol::TProtocol* oprot, void* callContext)
{
  void* ctx = NULL;
  if (this->eventHandler_.get() != NULL) {
    ctx = this->eventHandler_->getContext("KnnService.queryByItem", callContext);
  }
  ::apache::thrift::TProcessorContextFreer freer(this->eventHandler_.get(), ctx, "KnnService.queryByItem");

  if (this->eventHandler_.get() != NULL) {
    this->eventHandler_->preRead(ctx, "KnnService.queryByItem");
  }

  KnnService_queryByItem_args args;
  args.read(iprot);
  iprot->readMessageEnd();
  uint32_t bytes = iprot->getTransport()->readEnd();

  if (this->eventHandler_.get() != NULL) {
    this->eventHandler_->postRead(ctx, "KnnService.queryByItem", bytes);
  }

  KnnService_queryByItem_result result;
  try {
    iface_->queryByItem(result.success, args.item, args.n);
    result.__isset.success = true;
  } catch (InvalidRequest &err) {
    result.err = err;
    result.__isset.err = true;
  } catch (const std::exception& e) {
    if (this->eventHandler_.get() != NULL) {
      this->eventHandler_->handlerError(ctx, "KnnService.queryByItem");
    }

    ::apache::thrift::TApplicationException x(e.what());
    oprot->writeMessageBegin("queryByItem", ::apache::thrift::protocol::T_EXCEPTION, seqid);
    x.write(oprot);
    oprot->writeMessageEnd();
    oprot->getTransport()->writeEnd();
    oprot->getTransport()->flush();
    return;
  }

  if (this->eventHandler_.get() != NULL) {
    this->eventHandler_->preWrite(ctx, "KnnService.queryByItem");
  }

  oprot->writeMessageBegin("queryByItem", ::apache::thrift::protocol::T_REPLY, seqid);
  result.write(oprot);
  oprot->writeMessageEnd();
  bytes = oprot->getTransport()->writeEnd();
  oprot->getTransport()->flush();

  if (this->eventHandler_.get() != NULL) {
    this->eventHandler_->postWrite(ctx, "KnnService.queryByItem", bytes);
  }
}

template <class Protocol_>
void KnnServiceProcessorT<Protocol_>::process_queryByItem(int32_t seqid, Protocol_* iprot, Protocol_* oprot, void* callContext)
{
  void* ctx = NULL;
  if (this->eventHandler_.get() != NULL) {
    ctx = this->eventHandler_->getContext("KnnService.queryByItem", callContext);
  }
  ::apache::thrift::TProcessorContextFreer freer(this->eventHandler_.get(), ctx, "KnnService.queryByItem");

  if (this->eventHandler_.get() != NULL) {
    this->eventHandler_->preRead(ctx, "KnnService.queryByItem");
  }

  KnnService_queryByItem_args args;
  args.read(iprot);
  iprot->readMessageEnd();
  uint32_t bytes = iprot->getTransport()->readEnd();

  if (this->eventHandler_.get() != NULL) {
    this->eventHandler_->postRead(ctx, "KnnService.queryByItem", bytes);
  }

  KnnService_queryByItem_result result;
  try {
    iface_->queryByItem(result.success, args.item, args.n);
    result.__isset.success = true;
  } catch (InvalidRequest &err) {
    result.err = err;
    result.__isset.err = true;
  } catch (const std::exception& e) {
    if (this->eventHandler_.get() != NULL) {
      this->eventHandler_->handlerError(ctx, "KnnService.queryByItem");
    }

    ::apache::thrift::TApplicationException x(e.what());
    oprot->writeMessageBegin("queryByItem", ::apache::thrift::protocol::T_EXCEPTION, seqid);
    x.write(oprot);
    oprot->writeMessageEnd();
    oprot->getTransport()->writeEnd();
    oprot->getTransport()->flush();
    return;
  }

  if (this->eventHandler_.get() != NULL) {
    this->eventHandler_->preWrite(ctx, "KnnService.queryByItem");
  }

  oprot->writeMessageBegin("queryByItem", ::apache::thrift::protocol::T_REPLY, seqid);
  result.write(oprot);
  oprot->writeMessageEnd();
  bytes = oprot->getTransport()->writeEnd();
  oprot->getTransport()->flush();

  if (this->eventHandler_.get() != NULL) {
    this->eventHandler_->postWrite(ctx, "KnnService.queryByItem", bytes);
  }
}

template <class Protocol_>
void KnnServiceProcessorT<Protocol_>::process_queryByVector(int32_t seqid, ::apache::thrift::protocol::TProtocol* iprot, ::apache::thrift::protocol::TProtocol* oprot, void* callContext)
{
  void* ctx = NULL;
  if (this->eventHandler_.get() != NULL) {
    ctx = this->eventHandler_->getContext("KnnService.queryByVector", callContext);
  }
  ::apache::thrift::TProcessorContextFreer freer(this->eventHandler_.get(), ctx, "KnnService.queryByVector");

  if (this->eventHandler_.get() != NULL) {
    this->eventHandler_->preRead(ctx, "KnnService.queryByVector");
  }

  KnnService_queryByVector_args args;
  args.read(iprot);
  iprot->readMessageEnd();
  uint32_t bytes = iprot->getTransport()->readEnd();

  if (this->eventHandler_.get() != NULL) {
    this->eventHandler_->postRead(ctx, "KnnService.queryByVector", bytes);
  }

  KnnService_queryByVector_result result;
  try {
    iface_->queryByVector(result.success, args.values, args.n);
    result.__isset.success = true;
  } catch (InvalidRequest &err) {
    result.err = err;
    result.__isset.err = true;
  } catch (const std::exception& e) {
    if (this->eventHandler_.get() != NULL) {
      this->eventHandler_->handlerError(ctx, "KnnService.queryByVector");
    }

    ::apache::thrift::TApplicationException x(e.what());
    oprot->writeMessageBegin("queryByVector", ::apache::thrift::protocol::T_EXCEPTION, seqid);
    x.write(oprot);
    oprot->writeMessageEnd();
    oprot->getTransport()->writeEnd();
    oprot->getTransport()->flush();
    return;
  }

  if (this->eventHandler_.get() != NULL) {
    this->eventHandler_->preWrite(ctx, "KnnService.queryByVector");
  }

  oprot->writeMessageBegin("queryByVector", ::apache::thrift::protocol::T_REPLY, seqid);
  result.write(oprot);
  oprot->writeMessageEnd();
  bytes = oprot->getTransport()->writeEnd();
  oprot->getTransport()->flush();

  if (this->eventHandler_.get() != NULL) {
    this->eventHandler_->postWrite(ctx, "KnnService.queryByVector", bytes);
  }
}

template <class Protocol_>
void KnnServiceProcessorT<Protocol_>::process_queryByVector(int32_t seqid, Protocol_* iprot, Protocol_* oprot, void* callContext)
{
  void* ctx = NULL;
  if (this->eventHandler_.get() != NULL) {
    ctx = this->eventHandler_->getContext("KnnService.queryByVector", callContext);
  }
  ::apache::thrift::TProcessorContextFreer freer(this->eventHandler_.get(), ctx, "KnnService.queryByVector");

  if (this->eventHandler_.get() != NULL) {
    this->eventHandler_->preRead(ctx, "KnnService.queryByVector");
  }

  KnnService_queryByVector_args args;
  args.read(iprot);
  iprot->readMessageEnd();
  uint32_t bytes = iprot->getTransport()->readEnd();

  if (this->eventHandler_.get() != NULL) {
    this->eventHandler_->postRead(ctx, "KnnService.queryByVector", bytes);
  }

  KnnService_queryByVector_result result;
  try {
    iface_->queryByVector(result.success, args.values, args.n);
    result.__isset.success = true;
  } catch (InvalidRequest &err) {
    result.err = err;
    result.__isset.err = true;
  } catch (const std::exception& e) {
    if (this->eventHandler_.get() != NULL) {
      this->eventHandler_->handlerError(ctx, "KnnService.queryByVector");
    }

    ::apache::thrift::TApplicationException x(e.what());
    oprot->writeMessageBegin("queryByVector", ::apache::thrift::protocol::T_EXCEPTION, seqid);
    x.write(oprot);
    oprot->writeMessageEnd();
    oprot->getTransport()->writeEnd();
    oprot->getTransport()->flush();
    return;
  }

  if (this->eventHandler_.get() != NULL) {
    this->eventHandler_->preWrite(ctx, "KnnService.queryByVector");
  }

  oprot->writeMessageBegin("queryByVector", ::apache::thrift::protocol::T_REPLY, seqid);
  result.write(oprot);
  oprot->writeMessageEnd();
  bytes = oprot->getTransport()->writeEnd();
  oprot->getTransport()->flush();

  if (this->eventHandler_.get() != NULL) {
    this->eventHandler_->postWrite(ctx, "KnnService.queryByVector", bytes);
  }
}

template <class Protocol_>
::boost::shared_ptr< ::apache::thrift::TProcessor > KnnServiceProcessorFactoryT<Protocol_>::getProcessor(const ::apache::thrift::TConnectionInfo& connInfo) {
  ::apache::thrift::ReleaseHandler< KnnServiceIfFactory > cleanup(handlerFactory_);
  ::boost::shared_ptr< KnnServiceIf > handler(handlerFactory_->getHandler(connInfo), cleanup);
  ::boost::shared_ptr< ::apache::thrift::TProcessor > processor(new KnnServiceProcessorT<Protocol_>(handler));
  return processor;
}

template <class Protocol_>
void KnnServiceConcurrentClientT<Protocol_>::queryByItem(std::vector<Result> & _return, const std::string& item, const int32_t n)
{
  int32_t seqid = send_queryByItem(item, n);
  recv_queryByItem(_return, seqid);
}

template <class Protocol_>
int32_t KnnServiceConcurrentClientT<Protocol_>::send_queryByItem(const std::string& item, const int32_t n)
{
  int32_t cseqid = this->sync_.generateSeqId();
  ::apache::thrift::async::TConcurrentSendSentry sentry(&this->sync_);
  this->oprot_->writeMessageBegin("queryByItem", ::apache::thrift::protocol::T_CALL, cseqid);

  KnnService_queryByItem_pargs args;
  args.item = &item;
  args.n = &n;
  args.write(this->oprot_);

  this->oprot_->writeMessageEnd();
  this->oprot_->getTransport()->writeEnd();
  this->oprot_->getTransport()->flush();

  sentry.commit();
  return cseqid;
}

template <class Protocol_>
void KnnServiceConcurrentClientT<Protocol_>::recv_queryByItem(std::vector<Result> & _return, const int32_t seqid)
{

  int32_t rseqid = 0;
  std::string fname;
  ::apache::thrift::protocol::TMessageType mtype;

  // the read mutex gets dropped and reacquired as part of waitForWork()
  // The destructor of this sentry wakes up other clients
  ::apache::thrift::async::TConcurrentRecvSentry sentry(&this->sync_, seqid);

  while(true) {
    if(!this->sync_.getPending(fname, mtype, rseqid)) {
      this->iprot_->readMessageBegin(fname, mtype, rseqid);
    }
    if(seqid == rseqid) {
      if (mtype == ::apache::thrift::protocol::T_EXCEPTION) {
        ::apache::thrift::TApplicationException x;
        x.read(this->iprot_);
        this->iprot_->readMessageEnd();
        this->iprot_->getTransport()->readEnd();
        sentry.commit();
        throw x;
      }
      if (mtype != ::apache::thrift::protocol::T_REPLY) {
        this->iprot_->skip(::apache::thrift::protocol::T_STRUCT);
        this->iprot_->readMessageEnd();
        this->iprot_->getTransport()->readEnd();
      }
      if (fname.compare("queryByItem") != 0) {
        this->iprot_->skip(::apache::thrift::protocol::T_STRUCT);
        this->iprot_->readMessageEnd();
        this->iprot_->getTransport()->readEnd();

        // in a bad state, don't commit
        using ::apache::thrift::protocol::TProtocolException;
        throw TProtocolException(TProtocolException::INVALID_DATA);
      }
      KnnService_queryByItem_presult result;
      result.success = &_return;
      result.read(this->iprot_);
      this->iprot_->readMessageEnd();
      this->iprot_->getTransport()->readEnd();

      if (result.__isset.success) {
        // _return pointer has now been filled
        sentry.commit();
        return;
      }
      if (result.__isset.err) {
        sentry.commit();
        throw result.err;
      }
      // in a bad state, don't commit
      throw ::apache::thrift::TApplicationException(::apache::thrift::TApplicationException::MISSING_RESULT, "queryByItem failed: unknown result");
    }
    // seqid != rseqid
    this->sync_.updatePending(fname, mtype, rseqid);

    // this will temporarily unlock the readMutex, and let other clients get work done
    this->sync_.waitForWork(seqid);
  } // end while(true)
}

template <class Protocol_>
void KnnServiceConcurrentClientT<Protocol_>::queryByVector(std::vector<Result> & _return, const std::vector<double> & values, const int32_t n)
{
  int32_t seqid = send_queryByVector(values, n);
  recv_queryByVector(_return, seqid);
}

template <class Protocol_>
int32_t KnnServiceConcurrentClientT<Protocol_>::send_queryByVector(const std::vector<double> & values, const int32_t n)
{
  int32_t cseqid = this->sync_.generateSeqId();
  ::apache::thrift::async::TConcurrentSendSentry sentry(&this->sync_);
  this->oprot_->writeMessageBegin("queryByVector", ::apache::thrift::protocol::T_CALL, cseqid);

  KnnService_queryByVector_pargs args;
  args.values = &values;
  args.n = &n;
  args.write(this->oprot_);

  this->oprot_->writeMessageEnd();
  this->oprot_->getTransport()->writeEnd();
  this->oprot_->getTransport()->flush();

  sentry.commit();
  return cseqid;
}

template <class Protocol_>
void KnnServiceConcurrentClientT<Protocol_>::recv_queryByVector(std::vector<Result> & _return, const int32_t seqid)
{

  int32_t rseqid = 0;
  std::string fname;
  ::apache::thrift::protocol::TMessageType mtype;

  // the read mutex gets dropped and reacquired as part of waitForWork()
  // The destructor of this sentry wakes up other clients
  ::apache::thrift::async::TConcurrentRecvSentry sentry(&this->sync_, seqid);

  while(true) {
    if(!this->sync_.getPending(fname, mtype, rseqid)) {
      this->iprot_->readMessageBegin(fname, mtype, rseqid);
    }
    if(seqid == rseqid) {
      if (mtype == ::apache::thrift::protocol::T_EXCEPTION) {
        ::apache::thrift::TApplicationException x;
        x.read(this->iprot_);
        this->iprot_->readMessageEnd();
        this->iprot_->getTransport()->readEnd();
        sentry.commit();
        throw x;
      }
      if (mtype != ::apache::thrift::protocol::T_REPLY) {
        this->iprot_->skip(::apache::thrift::protocol::T_STRUCT);
        this->iprot_->readMessageEnd();
        this->iprot_->getTransport()->readEnd();
      }
      if (fname.compare("queryByVector") != 0) {
        this->iprot_->skip(::apache::thrift::protocol::T_STRUCT);
        this->iprot_->readMessageEnd();
        this->iprot_->getTransport()->readEnd();

        // in a bad state, don't commit
        using ::apache::thrift::protocol::TProtocolException;
        throw TProtocolException(TProtocolException::INVALID_DATA);
      }
      KnnService_queryByVector_presult result;
      result.success = &_return;
      result.read(this->iprot_);
      this->iprot_->readMessageEnd();
      this->iprot_->getTransport()->readEnd();

      if (result.__isset.success) {
        // _return pointer has now been filled
        sentry.commit();
        return;
      }
      if (result.__isset.err) {
        sentry.commit();
        throw result.err;
      }
      // in a bad state, don't commit
      throw ::apache::thrift::TApplicationException(::apache::thrift::TApplicationException::MISSING_RESULT, "queryByVector failed: unknown result");
    }
    // seqid != rseqid
    this->sync_.updatePending(fname, mtype, rseqid);

    // this will temporarily unlock the readMutex, and let other clients get work done
    this->sync_.waitForWork(seqid);
  } // end while(true)
}

} // namespace

#endif
