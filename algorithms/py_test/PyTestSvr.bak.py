# -*- coding: utf-8 -*-

"""
python PyTestSvr.py algmgr:127.0.0.1:9001 port:10080 algname:pytest nthreads:25
"""

import sys, glob, logging
import os, signal, threading, time
import socket

logging.basicConfig(level=logging.INFO)

sys.path.append('gen-py')
sys.path.append('../../api_server/gen-py')
# add path where built Apache Thrift libraries are
sys.path.insert(0, glob.glob('thrift-0.9.3/lib/py/build/lib.*')[0])


from PyTest import PyService
from PyTest.ttypes import *
from PyTest.constants import *

from BigRLab import AlgMgrService
from BigRLab.ttypes import *
from BigRLab.constants import *

from thrift.transport import TSocket
from thrift.transport import TTransport
from thrift.protocol import TBinaryProtocol
from thrift.server import TNonblockingServer
from thrift.server import TServer

# global vars
LIB_NAME = "PyTest"
argDict = dict()
algmgrIp = ""
algmgrPort = 0
algmgrCli = None
localIp = ""
localPort = 0
nthreads = 10
algSvrInfo = None
algmgrTransport = None
serviceName = ""

server = None

class PyTestHandler:
    def __init__(self):
        pass

    def segment(self, text):
        print text
        ret = []
        id = 0
        for w in text.split():
            res = Result()
            res.id = id
            res.word = w
            ret.append(res)
            id = id + 1
        return ret

    def handleRequest(self, request):
        print request
        return request

def sig_handler(signum, frame):
    print "Received signal "
    if signum == signal.SIGINT:
        server.stop()

class RunSvrThread(threading.Thread):
    def run(self):
        global server
        server.serve()

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
    global nthreads
    global serviceName

    for (k,v) in argDict.items():
        #  print (k, v)
        if k == "algmgr":
            algmgr = v.split(":")
        elif k == "port":
            localPort = int(v)
        elif k == "algname":
            serviceName = v
        elif k == "nthreads":
            nthreads = int(v)

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


if __name__ == '__main__':
    for arg in sys.argv[1:]:
        argKV = arg.split(":", 1)
        if len(argKV) != 2:
            print "wrong argument!"
            sys.exit(-1)
        argDict[argKV[0]] = argKV[1]

    parse_args()

    #  signal.signal(signal.SIGINT, sig_handler)

    get_local_ip()
    #  print "local ip is %s" % localIp

    algSvrInfo = AlgSvrInfo()
    algSvrInfo.addr = localIp
    algSvrInfo.port = localPort
    algSvrInfo.maxConcurrency = nthreads
    algSvrInfo.serviceName = LIB_NAME

    print algSvrInfo

    start_algmgr_client()
    print "connect to algmgr success."

    handler = PyTestHandler()
    processor = PyService.Processor(handler)
    transport = TSocket.TServerSocket(port = 10080)
    tfactory = TTransport.TFramedTransportFactory()
    #  tfactory = TTransport.TBufferedTransportFactory()
    pfactory = TBinaryProtocol.TBinaryProtocolFactory()

    # 最后一个参数是线程数量，不分工作io
    #  server = TNonblockingServer.TNonblockingServer(processor, transport, tfactory, pfactory, nthreads)
    server = TServer.TSimpleServer(processor, transport, tfactory, pfactory)

    #  logging.info("Server starts...")
    #  server.serve()

    # start server
    #  print "Starting server..."
    #  svrThr = RunSvrThread()
    #  svrThr.start()

    #  time.sleep(3);

    # register svr
    #  algmgrCli.rmSvr(serviceName, algSvrInfo)
    #  ret = algmgrCli.addSvr(serviceName, algSvrInfo)
    #  if ret != 0:
        #  raise Exception("Register server fail! retval = %d" % ret)
    #  print "Register server success!"
    registerSvr = RegisterSvrThread()
    registerSvr.start()
    print "AlgSvr %s running..." % serviceName
    server.serve()
    sys.exit(0)


    while 1:
        cmd = sys.stdin.readline()
        cmd = cmd.strip()
        if cmd == "quit":
            server.stop()
            break

    algmgrCli.rmSvr(serviceName, algSvrInfo)
    svrThr.join();
    print "%s done!" % sys.argv[0]


