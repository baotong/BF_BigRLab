/**
 * Autogenerated by Thrift Compiler (0.9.3)
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */
#include "PyService.h"
#ifndef PyService_TCC
#define PyService_TCC


namespace PyTest {


template <class Protocol_>
uint32_t PyService_segment_args::read(Protocol_* iprot) {

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
          xfer += iprot->readString(this->text);
          this->__isset.text = true;
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
uint32_t PyService_segment_args::write(Protocol_* oprot) const {
  uint32_t xfer = 0;
  apache::thrift::protocol::TOutputRecursionTracker tracker(*oprot);
  xfer += oprot->writeStructBegin("PyService_segment_args");

  xfer += oprot->writeFieldBegin("text", ::apache::thrift::protocol::T_STRING, 1);
  xfer += oprot->writeString(this->text);
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldStop();
  xfer += oprot->writeStructEnd();
  return xfer;
}


template <class Protocol_>
uint32_t PyService_segment_pargs::write(Protocol_* oprot) const {
  uint32_t xfer = 0;
  apache::thrift::protocol::TOutputRecursionTracker tracker(*oprot);
  xfer += oprot->writeStructBegin("PyService_segment_pargs");

  xfer += oprot->writeFieldBegin("text", ::apache::thrift::protocol::T_STRING, 1);
  xfer += oprot->writeString((*(this->text)));
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldStop();
  xfer += oprot->writeStructEnd();
  return xfer;
}


template <class Protocol_>
uint32_t PyService_segment_result::read(Protocol_* iprot) {

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
uint32_t PyService_segment_result::write(Protocol_* oprot) const {

  uint32_t xfer = 0;

  xfer += oprot->writeStructBegin("PyService_segment_result");

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
uint32_t PyService_segment_presult::read(Protocol_* iprot) {

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
uint32_t PyService_handleRequest_args::read(Protocol_* iprot) {

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
          xfer += iprot->readString(this->request);
          this->__isset.request = true;
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
uint32_t PyService_handleRequest_args::write(Protocol_* oprot) const {
  uint32_t xfer = 0;
  apache::thrift::protocol::TOutputRecursionTracker tracker(*oprot);
  xfer += oprot->writeStructBegin("PyService_handleRequest_args");

  xfer += oprot->writeFieldBegin("request", ::apache::thrift::protocol::T_STRING, 1);
  xfer += oprot->writeString(this->request);
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldStop();
  xfer += oprot->writeStructEnd();
  return xfer;
}


template <class Protocol_>
uint32_t PyService_handleRequest_pargs::write(Protocol_* oprot) const {
  uint32_t xfer = 0;
  apache::thrift::protocol::TOutputRecursionTracker tracker(*oprot);
  xfer += oprot->writeStructBegin("PyService_handleRequest_pargs");

  xfer += oprot->writeFieldBegin("request", ::apache::thrift::protocol::T_STRING, 1);
  xfer += oprot->writeString((*(this->request)));
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldStop();
  xfer += oprot->writeStructEnd();
  return xfer;
}


template <class Protocol_>
uint32_t PyService_handleRequest_result::read(Protocol_* iprot) {

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
        if (ftype == ::apache::thrift::protocol::T_STRING) {
          xfer += iprot->readString(this->success);
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
uint32_t PyService_handleRequest_result::write(Protocol_* oprot) const {

  uint32_t xfer = 0;

  xfer += oprot->writeStructBegin("PyService_handleRequest_result");

  if (this->__isset.success) {
    xfer += oprot->writeFieldBegin("success", ::apache::thrift::protocol::T_STRING, 0);
    xfer += oprot->writeString(this->success);
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
uint32_t PyService_handleRequest_presult::read(Protocol_* iprot) {

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
        if (ftype == ::apache::thrift::protocol::T_STRING) {
          xfer += iprot->readString((*(this->success)));
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
void PyServiceClientT<Protocol_>::segment(std::vector<Result> & _return, const std::string& text)
{
  send_segment(text);
  recv_segment(_return);
}

template <class Protocol_>
void PyServiceClientT<Protocol_>::send_segment(const std::string& text)
{
  int32_t cseqid = 0;
  this->oprot_->writeMessageBegin("segment", ::apache::thrift::protocol::T_CALL, cseqid);

  PyService_segment_pargs args;
  args.text = &text;
  args.write(this->oprot_);

  this->oprot_->writeMessageEnd();
  this->oprot_->getTransport()->writeEnd();
  this->oprot_->getTransport()->flush();
}

template <class Protocol_>
void PyServiceClientT<Protocol_>::recv_segment(std::vector<Result> & _return)
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
  if (fname.compare("segment") != 0) {
    this->iprot_->skip(::apache::thrift::protocol::T_STRUCT);
    this->iprot_->readMessageEnd();
    this->iprot_->getTransport()->readEnd();
  }
  PyService_segment_presult result;
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
  throw ::apache::thrift::TApplicationException(::apache::thrift::TApplicationException::MISSING_RESULT, "segment failed: unknown result");
}

template <class Protocol_>
void PyServiceClientT<Protocol_>::handleRequest(std::string& _return, const std::string& request)
{
  send_handleRequest(request);
  recv_handleRequest(_return);
}

template <class Protocol_>
void PyServiceClientT<Protocol_>::send_handleRequest(const std::string& request)
{
  int32_t cseqid = 0;
  this->oprot_->writeMessageBegin("handleRequest", ::apache::thrift::protocol::T_CALL, cseqid);

  PyService_handleRequest_pargs args;
  args.request = &request;
  args.write(this->oprot_);

  this->oprot_->writeMessageEnd();
  this->oprot_->getTransport()->writeEnd();
  this->oprot_->getTransport()->flush();
}

template <class Protocol_>
void PyServiceClientT<Protocol_>::recv_handleRequest(std::string& _return)
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
  if (fname.compare("handleRequest") != 0) {
    this->iprot_->skip(::apache::thrift::protocol::T_STRUCT);
    this->iprot_->readMessageEnd();
    this->iprot_->getTransport()->readEnd();
  }
  PyService_handleRequest_presult result;
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
  throw ::apache::thrift::TApplicationException(::apache::thrift::TApplicationException::MISSING_RESULT, "handleRequest failed: unknown result");
}

template <class Protocol_>
bool PyServiceProcessorT<Protocol_>::dispatchCall(::apache::thrift::protocol::TProtocol* iprot, ::apache::thrift::protocol::TProtocol* oprot, const std::string& fname, int32_t seqid, void* callContext) {
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
bool PyServiceProcessorT<Protocol_>::dispatchCallTemplated(Protocol_* iprot, Protocol_* oprot, const std::string& fname, int32_t seqid, void* callContext) {
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
void PyServiceProcessorT<Protocol_>::process_segment(int32_t seqid, ::apache::thrift::protocol::TProtocol* iprot, ::apache::thrift::protocol::TProtocol* oprot, void* callContext)
{
  void* ctx = NULL;
  if (this->eventHandler_.get() != NULL) {
    ctx = this->eventHandler_->getContext("PyService.segment", callContext);
  }
  ::apache::thrift::TProcessorContextFreer freer(this->eventHandler_.get(), ctx, "PyService.segment");

  if (this->eventHandler_.get() != NULL) {
    this->eventHandler_->preRead(ctx, "PyService.segment");
  }

  PyService_segment_args args;
  args.read(iprot);
  iprot->readMessageEnd();
  uint32_t bytes = iprot->getTransport()->readEnd();

  if (this->eventHandler_.get() != NULL) {
    this->eventHandler_->postRead(ctx, "PyService.segment", bytes);
  }

  PyService_segment_result result;
  try {
    iface_->segment(result.success, args.text);
    result.__isset.success = true;
  } catch (InvalidRequest &err) {
    result.err = err;
    result.__isset.err = true;
  } catch (const std::exception& e) {
    if (this->eventHandler_.get() != NULL) {
      this->eventHandler_->handlerError(ctx, "PyService.segment");
    }

    ::apache::thrift::TApplicationException x(e.what());
    oprot->writeMessageBegin("segment", ::apache::thrift::protocol::T_EXCEPTION, seqid);
    x.write(oprot);
    oprot->writeMessageEnd();
    oprot->getTransport()->writeEnd();
    oprot->getTransport()->flush();
    return;
  }

  if (this->eventHandler_.get() != NULL) {
    this->eventHandler_->preWrite(ctx, "PyService.segment");
  }

  oprot->writeMessageBegin("segment", ::apache::thrift::protocol::T_REPLY, seqid);
  result.write(oprot);
  oprot->writeMessageEnd();
  bytes = oprot->getTransport()->writeEnd();
  oprot->getTransport()->flush();

  if (this->eventHandler_.get() != NULL) {
    this->eventHandler_->postWrite(ctx, "PyService.segment", bytes);
  }
}

template <class Protocol_>
void PyServiceProcessorT<Protocol_>::process_segment(int32_t seqid, Protocol_* iprot, Protocol_* oprot, void* callContext)
{
  void* ctx = NULL;
  if (this->eventHandler_.get() != NULL) {
    ctx = this->eventHandler_->getContext("PyService.segment", callContext);
  }
  ::apache::thrift::TProcessorContextFreer freer(this->eventHandler_.get(), ctx, "PyService.segment");

  if (this->eventHandler_.get() != NULL) {
    this->eventHandler_->preRead(ctx, "PyService.segment");
  }

  PyService_segment_args args;
  args.read(iprot);
  iprot->readMessageEnd();
  uint32_t bytes = iprot->getTransport()->readEnd();

  if (this->eventHandler_.get() != NULL) {
    this->eventHandler_->postRead(ctx, "PyService.segment", bytes);
  }

  PyService_segment_result result;
  try {
    iface_->segment(result.success, args.text);
    result.__isset.success = true;
  } catch (InvalidRequest &err) {
    result.err = err;
    result.__isset.err = true;
  } catch (const std::exception& e) {
    if (this->eventHandler_.get() != NULL) {
      this->eventHandler_->handlerError(ctx, "PyService.segment");
    }

    ::apache::thrift::TApplicationException x(e.what());
    oprot->writeMessageBegin("segment", ::apache::thrift::protocol::T_EXCEPTION, seqid);
    x.write(oprot);
    oprot->writeMessageEnd();
    oprot->getTransport()->writeEnd();
    oprot->getTransport()->flush();
    return;
  }

  if (this->eventHandler_.get() != NULL) {
    this->eventHandler_->preWrite(ctx, "PyService.segment");
  }

  oprot->writeMessageBegin("segment", ::apache::thrift::protocol::T_REPLY, seqid);
  result.write(oprot);
  oprot->writeMessageEnd();
  bytes = oprot->getTransport()->writeEnd();
  oprot->getTransport()->flush();

  if (this->eventHandler_.get() != NULL) {
    this->eventHandler_->postWrite(ctx, "PyService.segment", bytes);
  }
}

template <class Protocol_>
void PyServiceProcessorT<Protocol_>::process_handleRequest(int32_t seqid, ::apache::thrift::protocol::TProtocol* iprot, ::apache::thrift::protocol::TProtocol* oprot, void* callContext)
{
  void* ctx = NULL;
  if (this->eventHandler_.get() != NULL) {
    ctx = this->eventHandler_->getContext("PyService.handleRequest", callContext);
  }
  ::apache::thrift::TProcessorContextFreer freer(this->eventHandler_.get(), ctx, "PyService.handleRequest");

  if (this->eventHandler_.get() != NULL) {
    this->eventHandler_->preRead(ctx, "PyService.handleRequest");
  }

  PyService_handleRequest_args args;
  args.read(iprot);
  iprot->readMessageEnd();
  uint32_t bytes = iprot->getTransport()->readEnd();

  if (this->eventHandler_.get() != NULL) {
    this->eventHandler_->postRead(ctx, "PyService.handleRequest", bytes);
  }

  PyService_handleRequest_result result;
  try {
    iface_->handleRequest(result.success, args.request);
    result.__isset.success = true;
  } catch (InvalidRequest &err) {
    result.err = err;
    result.__isset.err = true;
  } catch (const std::exception& e) {
    if (this->eventHandler_.get() != NULL) {
      this->eventHandler_->handlerError(ctx, "PyService.handleRequest");
    }

    ::apache::thrift::TApplicationException x(e.what());
    oprot->writeMessageBegin("handleRequest", ::apache::thrift::protocol::T_EXCEPTION, seqid);
    x.write(oprot);
    oprot->writeMessageEnd();
    oprot->getTransport()->writeEnd();
    oprot->getTransport()->flush();
    return;
  }

  if (this->eventHandler_.get() != NULL) {
    this->eventHandler_->preWrite(ctx, "PyService.handleRequest");
  }

  oprot->writeMessageBegin("handleRequest", ::apache::thrift::protocol::T_REPLY, seqid);
  result.write(oprot);
  oprot->writeMessageEnd();
  bytes = oprot->getTransport()->writeEnd();
  oprot->getTransport()->flush();

  if (this->eventHandler_.get() != NULL) {
    this->eventHandler_->postWrite(ctx, "PyService.handleRequest", bytes);
  }
}

template <class Protocol_>
void PyServiceProcessorT<Protocol_>::process_handleRequest(int32_t seqid, Protocol_* iprot, Protocol_* oprot, void* callContext)
{
  void* ctx = NULL;
  if (this->eventHandler_.get() != NULL) {
    ctx = this->eventHandler_->getContext("PyService.handleRequest", callContext);
  }
  ::apache::thrift::TProcessorContextFreer freer(this->eventHandler_.get(), ctx, "PyService.handleRequest");

  if (this->eventHandler_.get() != NULL) {
    this->eventHandler_->preRead(ctx, "PyService.handleRequest");
  }

  PyService_handleRequest_args args;
  args.read(iprot);
  iprot->readMessageEnd();
  uint32_t bytes = iprot->getTransport()->readEnd();

  if (this->eventHandler_.get() != NULL) {
    this->eventHandler_->postRead(ctx, "PyService.handleRequest", bytes);
  }

  PyService_handleRequest_result result;
  try {
    iface_->handleRequest(result.success, args.request);
    result.__isset.success = true;
  } catch (InvalidRequest &err) {
    result.err = err;
    result.__isset.err = true;
  } catch (const std::exception& e) {
    if (this->eventHandler_.get() != NULL) {
      this->eventHandler_->handlerError(ctx, "PyService.handleRequest");
    }

    ::apache::thrift::TApplicationException x(e.what());
    oprot->writeMessageBegin("handleRequest", ::apache::thrift::protocol::T_EXCEPTION, seqid);
    x.write(oprot);
    oprot->writeMessageEnd();
    oprot->getTransport()->writeEnd();
    oprot->getTransport()->flush();
    return;
  }

  if (this->eventHandler_.get() != NULL) {
    this->eventHandler_->preWrite(ctx, "PyService.handleRequest");
  }

  oprot->writeMessageBegin("handleRequest", ::apache::thrift::protocol::T_REPLY, seqid);
  result.write(oprot);
  oprot->writeMessageEnd();
  bytes = oprot->getTransport()->writeEnd();
  oprot->getTransport()->flush();

  if (this->eventHandler_.get() != NULL) {
    this->eventHandler_->postWrite(ctx, "PyService.handleRequest", bytes);
  }
}

template <class Protocol_>
::boost::shared_ptr< ::apache::thrift::TProcessor > PyServiceProcessorFactoryT<Protocol_>::getProcessor(const ::apache::thrift::TConnectionInfo& connInfo) {
  ::apache::thrift::ReleaseHandler< PyServiceIfFactory > cleanup(handlerFactory_);
  ::boost::shared_ptr< PyServiceIf > handler(handlerFactory_->getHandler(connInfo), cleanup);
  ::boost::shared_ptr< ::apache::thrift::TProcessor > processor(new PyServiceProcessorT<Protocol_>(handler));
  return processor;
}

template <class Protocol_>
void PyServiceConcurrentClientT<Protocol_>::segment(std::vector<Result> & _return, const std::string& text)
{
  int32_t seqid = send_segment(text);
  recv_segment(_return, seqid);
}

template <class Protocol_>
int32_t PyServiceConcurrentClientT<Protocol_>::send_segment(const std::string& text)
{
  int32_t cseqid = this->sync_.generateSeqId();
  ::apache::thrift::async::TConcurrentSendSentry sentry(&this->sync_);
  this->oprot_->writeMessageBegin("segment", ::apache::thrift::protocol::T_CALL, cseqid);

  PyService_segment_pargs args;
  args.text = &text;
  args.write(this->oprot_);

  this->oprot_->writeMessageEnd();
  this->oprot_->getTransport()->writeEnd();
  this->oprot_->getTransport()->flush();

  sentry.commit();
  return cseqid;
}

template <class Protocol_>
void PyServiceConcurrentClientT<Protocol_>::recv_segment(std::vector<Result> & _return, const int32_t seqid)
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
      if (fname.compare("segment") != 0) {
        this->iprot_->skip(::apache::thrift::protocol::T_STRUCT);
        this->iprot_->readMessageEnd();
        this->iprot_->getTransport()->readEnd();

        // in a bad state, don't commit
        using ::apache::thrift::protocol::TProtocolException;
        throw TProtocolException(TProtocolException::INVALID_DATA);
      }
      PyService_segment_presult result;
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
      throw ::apache::thrift::TApplicationException(::apache::thrift::TApplicationException::MISSING_RESULT, "segment failed: unknown result");
    }
    // seqid != rseqid
    this->sync_.updatePending(fname, mtype, rseqid);

    // this will temporarily unlock the readMutex, and let other clients get work done
    this->sync_.waitForWork(seqid);
  } // end while(true)
}

template <class Protocol_>
void PyServiceConcurrentClientT<Protocol_>::handleRequest(std::string& _return, const std::string& request)
{
  int32_t seqid = send_handleRequest(request);
  recv_handleRequest(_return, seqid);
}

template <class Protocol_>
int32_t PyServiceConcurrentClientT<Protocol_>::send_handleRequest(const std::string& request)
{
  int32_t cseqid = this->sync_.generateSeqId();
  ::apache::thrift::async::TConcurrentSendSentry sentry(&this->sync_);
  this->oprot_->writeMessageBegin("handleRequest", ::apache::thrift::protocol::T_CALL, cseqid);

  PyService_handleRequest_pargs args;
  args.request = &request;
  args.write(this->oprot_);

  this->oprot_->writeMessageEnd();
  this->oprot_->getTransport()->writeEnd();
  this->oprot_->getTransport()->flush();

  sentry.commit();
  return cseqid;
}

template <class Protocol_>
void PyServiceConcurrentClientT<Protocol_>::recv_handleRequest(std::string& _return, const int32_t seqid)
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
      if (fname.compare("handleRequest") != 0) {
        this->iprot_->skip(::apache::thrift::protocol::T_STRUCT);
        this->iprot_->readMessageEnd();
        this->iprot_->getTransport()->readEnd();

        // in a bad state, don't commit
        using ::apache::thrift::protocol::TProtocolException;
        throw TProtocolException(TProtocolException::INVALID_DATA);
      }
      PyService_handleRequest_presult result;
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
      throw ::apache::thrift::TApplicationException(::apache::thrift::TApplicationException::MISSING_RESULT, "handleRequest failed: unknown result");
    }
    // seqid != rseqid
    this->sync_.updatePending(fname, mtype, rseqid);

    // this will temporarily unlock the readMutex, and let other clients get work done
    this->sync_.waitForWork(seqid);
  } // end while(true)
}

} // namespace

#endif
