#
# Autogenerated by Thrift Compiler (0.9.3)
#
# DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
#
#  options string: py:new_style,utf8strings
#

from thrift.Thrift import TType, TMessageType, TException, TApplicationException

from thrift.transport import TTransport
from thrift.protocol import TBinaryProtocol, TProtocol
try:
  from thrift.protocol import fastbinary
except:
  fastbinary = None


class ErrCodeType(object):
  SUCCESS = 0
  ALREADY_EXIST = 1
  SERVER_UNREACHABLE = 2
  NO_SERVICE = 3
  INTERNAL_FAIL = 4

  _VALUES_TO_NAMES = {
    0: "SUCCESS",
    1: "ALREADY_EXIST",
    2: "SERVER_UNREACHABLE",
    3: "NO_SERVICE",
    4: "INTERNAL_FAIL",
  }

  _NAMES_TO_VALUES = {
    "SUCCESS": 0,
    "ALREADY_EXIST": 1,
    "SERVER_UNREACHABLE": 2,
    "NO_SERVICE": 3,
    "INTERNAL_FAIL": 4,
  }


class AlgSvrInfo(object):
  """
  Attributes:
   - addr
   - port
   - maxConcurrency
   - serviceName
  """

  thrift_spec = (
    None, # 0
    (1, TType.STRING, 'addr', None, None, ), # 1
    (2, TType.I16, 'port', None, None, ), # 2
    (3, TType.I32, 'maxConcurrency', None, None, ), # 3
    (4, TType.STRING, 'serviceName', None, None, ), # 4
  )

  def __init__(self, addr=None, port=None, maxConcurrency=None, serviceName=None,):
    self.addr = addr
    self.port = port
    self.maxConcurrency = maxConcurrency
    self.serviceName = serviceName

  def read(self, iprot):
    if iprot.__class__ == TBinaryProtocol.TBinaryProtocolAccelerated and isinstance(iprot.trans, TTransport.CReadableTransport) and self.thrift_spec is not None and fastbinary is not None:
      fastbinary.decode_binary(self, iprot.trans, (self.__class__, self.thrift_spec))
      return
    iprot.readStructBegin()
    while True:
      (fname, ftype, fid) = iprot.readFieldBegin()
      if ftype == TType.STOP:
        break
      if fid == 1:
        if ftype == TType.STRING:
          self.addr = iprot.readString().decode('utf-8')
        else:
          iprot.skip(ftype)
      elif fid == 2:
        if ftype == TType.I16:
          self.port = iprot.readI16()
        else:
          iprot.skip(ftype)
      elif fid == 3:
        if ftype == TType.I32:
          self.maxConcurrency = iprot.readI32()
        else:
          iprot.skip(ftype)
      elif fid == 4:
        if ftype == TType.STRING:
          self.serviceName = iprot.readString().decode('utf-8')
        else:
          iprot.skip(ftype)
      else:
        iprot.skip(ftype)
      iprot.readFieldEnd()
    iprot.readStructEnd()

  def write(self, oprot):
    if oprot.__class__ == TBinaryProtocol.TBinaryProtocolAccelerated and self.thrift_spec is not None and fastbinary is not None:
      oprot.trans.write(fastbinary.encode_binary(self, (self.__class__, self.thrift_spec)))
      return
    oprot.writeStructBegin('AlgSvrInfo')
    if self.addr is not None:
      oprot.writeFieldBegin('addr', TType.STRING, 1)
      oprot.writeString(self.addr.encode('utf-8'))
      oprot.writeFieldEnd()
    if self.port is not None:
      oprot.writeFieldBegin('port', TType.I16, 2)
      oprot.writeI16(self.port)
      oprot.writeFieldEnd()
    if self.maxConcurrency is not None:
      oprot.writeFieldBegin('maxConcurrency', TType.I32, 3)
      oprot.writeI32(self.maxConcurrency)
      oprot.writeFieldEnd()
    if self.serviceName is not None:
      oprot.writeFieldBegin('serviceName', TType.STRING, 4)
      oprot.writeString(self.serviceName.encode('utf-8'))
      oprot.writeFieldEnd()
    oprot.writeFieldStop()
    oprot.writeStructEnd()

  def validate(self):
    return


  def __hash__(self):
    value = 17
    value = (value * 31) ^ hash(self.addr)
    value = (value * 31) ^ hash(self.port)
    value = (value * 31) ^ hash(self.maxConcurrency)
    value = (value * 31) ^ hash(self.serviceName)
    return value

  def __repr__(self):
    L = ['%s=%r' % (key, value)
      for key, value in self.__dict__.iteritems()]
    return '%s(%s)' % (self.__class__.__name__, ', '.join(L))

  def __eq__(self, other):
    return isinstance(other, self.__class__) and self.__dict__ == other.__dict__

  def __ne__(self, other):
    return not (self == other)
