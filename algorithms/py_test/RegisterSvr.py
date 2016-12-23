import sys, glob
import os, time

sys.path.append('../../api_server/gen-py')
sys.path.insert(0, glob.glob('thrift-0.9.3/lib/py/build/lib.*')[0])

from thrift import Thrift
from thrift.transport import TSocket
from thrift.transport import TTransport
from thrift.protocol import TBinaryProtocol

from BigRLab import AlgMgrService
from BigRLab.ttypes import *
from BigRLab.constants import *


# RegisterSvr.py algmgrIp algmgrPort localIp localPort libname servicename nthreads
if __name__ == '__main__':
    #  for arg in sys.argv:
        #  print arg

    algmgrIp = sys.argv[1]
    algmgrPort = int(sys.argv[2])

    algSvrInfo = AlgSvrInfo()
    algSvrInfo.addr = sys.argv[3]
    algSvrInfo.port = int(sys.argv[4])
    algSvrInfo.serviceName = sys.argv[5]
    serviceName = sys.argv[6]
    algSvrInfo.maxConcurrency = int(sys.argv[7])

    # register svr
    algmgrTransport = TSocket.TSocket(algmgrIp, algmgrPort)
    algmgrTransport = TTransport.TFramedTransport(algmgrTransport)
    algmgrProtocol = TBinaryProtocol.TBinaryProtocol(algmgrTransport)
    algmgrCli = AlgMgrService.Client(algmgrProtocol)
    algmgrTransport.open()
    algmgrCli.rmSvr(serviceName, algSvrInfo)
    ret = algmgrCli.addSvr(serviceName, algSvrInfo)
    if ret != 0:
        #  print "Register server fail! retval = %d" % ret
        sys.exit(-1)
    #  print "Register server success!"
    sys.exit(0)

