# -*- coding: utf-8 -*-

"""
cd thrift-0.9.2/lib/py
python setup.py build
python PyTestSvr.py algmgr:127.0.0.1:9001 port:10080 algname:pytest nworkers:10

"""
import sys

reload(sys)
sys.setdefaultencoding('utf8')
from jieba import cut
import sys, glob
import os, threading, signal
import socket
print '|'.join(cut('initialization ..'))
sys.path.append('gen-py')
#  sys.path.append('../../api_server/gen-py')
sys.path.insert(0, glob.glob('thrift-0.9.3/lib/py/build/lib.*')[0])
import kenlm_seg as seg

from PyTest import PyService
from PyTest.ttypes import *
from PyTest.constants import *

from BigRLab import AlgMgrService
from BigRLab.ttypes import *
from BigRLab.constants import *

from thrift.transport import TSocket
from thrift.transport import TTransport
from thrift.protocol import TBinaryProtocol
from thrift.server import TProcessPoolServer
#  from thrift.server import TNonblockingServer
#  from thrift.server import TServer

# global vars
LIB_NAME = "PyTest"
argDict = dict()
algmgrIp = ""
algmgrPort = 0
algmgrCli = None
localIp = ""
localPort = 0
nworkers = 10
algSvrInfo = None
algmgrTransport = None
serviceName = ""
#import jieba
server = None

class PyTestHandler:
    def __init__(self):
        pass

    def segment(self, text):
        #print text
        ret = []
        id = 0
        for w in text.split():
	    #print w
            res = Result()
            res.id = id
            res.word = '|'.join(seg.mycut(w))
            ret.append(res)
            id = id + 1
        return res.word

    def handleRequest(self, request):
        #print request
        test=self.segment(request)
	#print test
        return test


class RegisterSvrThread(threading.Thread):
    def run(self):
        global algmgrCli
        global serviceName
        global algSvrInfo
        algmgrCli.rmSvr(serviceName, algSvrInfo)
        ret = algmgrCli.addSvr(serviceName, algSvrInfo)
        if ret != 0:
            raise Exception("Register server fail! retval = %d" % ret)
        print "Register server success!"


def start_algmgr_client():
    global algmgrIp
    global algmgrPort
    global algmgrCli
    global algmgrTransport
    algmgrTransport = TSocket.TSocket(algmgrIp, algmgrPort)
    algmgrTransport = TTransport.TFramedTransport(algmgrTransport)
    protocol = TBinaryProtocol.TBinaryProtocol(algmgrTransport)
    algmgrCli = AlgMgrService.Client(protocol)
    algmgrTransport.open()

def get_local_ip():
    global algmgrIp
    global algmgrPort
    global localIp
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.connect((algmgrIp, algmgrPort))
    localIp = s.getsockname()[0]
    s.close()


def parse_args():
    global argDict
    global algmgrIp
    global algmgrPort
    global localPort
    global nworkers
    global serviceName

    for (k,v) in argDict.items():
        #  print (k, v)
        if k == "algmgr":
            algmgr = v.split(":")
        elif k == "port":
            localPort = int(v)
        elif k == "algname":
            serviceName = v
        elif k == "nworkers":
            nworkers = int(v)

    if len(algmgr) != 2:
        print "invalid algmgr!"
        sys.exit(-1)

    if localPort <= 1024:
        print "invalid port!"
        sys.exit(-1)

    if serviceName == "":
        print "invalid algname"
        sys.exit(-1)

    algmgrIp = algmgr[0]
    algmgrPort = int(algmgr[1])


def sig_handler(signum, frame):
    global server
    global serviceName
    global algSvrInfo
    #  print "Received signal"
    if signum == signal.SIGINT:
        #  print "SIGINT"
        server.stop()
        algmgrCli.rmSvr(serviceName, algSvrInfo)
        sys.exit(0)


if __name__ == '__main__':
    #import jieba
    for arg in sys.argv[1:]:
        argKV = arg.split(":", 1)
        if len(argKV) != 2:
            print "wrong argument!"
            sys.exit(-1)
        argDict[argKV[0]] = argKV[1]

    parse_args()

    #  signal.signal(signal.SIGINT, sig_handler)

    get_local_ip()
    print "local ip is %s" % localIp

    algSvrInfo = AlgSvrInfo()
    algSvrInfo.addr = localIp
    algSvrInfo.port = localPort
    algSvrInfo.maxConcurrency = nworkers
    algSvrInfo.serviceName = LIB_NAME

    print algSvrInfo

    start_algmgr_client()
    print "connect to algmgr success."

    #  pid = os.fork()
    #  if pid == 0:
        #  cmd = "python RegisterSvr.py %s %d %s %d %s %s %d" % (algmgrIp, algmgrPort, localIp, localPort, LIB_NAME, serviceName, nworkers)
        #  ret = os.system(cmd)
        #  if ret != 0:
            #  print "Register server fail ret = %d" % ret
            #  sys.exit(-1)
        #  print "Register server success!"

    handler = PyTestHandler()
    processor = PyService.Processor(handler)
    transport = TSocket.TServerSocket(port = localPort)
    tfactory = TTransport.TFramedTransportFactory()
    #  tfactory = TTransport.TBufferedTransportFactory()
    pfactory = TBinaryProtocol.TBinaryProtocolFactory()

    #  server = TNonblockingServer.TNonblockingServer(processor, transport, tfactory, pfactory, nworkers)
    #  server = TServer.TSimpleServer(processor, transport, tfactory, pfactory)
    server = TProcessPoolServer.TProcessPoolServer(processor, transport, tfactory, pfactory)
    server.setNumWorkers(nworkers)

    registerSvr = RegisterSvrThread()
    registerSvr.start()

    # start server
    print "Starting server..."
    server.serve()

"""
    svrThr = RunSvrThread()
    svrThr.start()
    # register
    cmd = "./RegisterSvr.bin %s %d %s %d %s %s %d" % (algmgrIp, algmgrPort, localIp, localPort, LIB_NAME, serviceName, nworkers)
    #  ret = os.system("sleep 3; ls")
    ret = os.system(cmd)
    if ret != 0:
        print "Register server fail ret = %d" % ret
        sys.exit(-1)
    print "Register server success!"
    # end register
    svrThr.join()
"""


