# -*- coding: utf-8 -*-

import sys, glob, logging
import os, signal, threading

logging.basicConfig(level=logging.INFO)

sys.path.append('gen-py')
# add path where built Apache Thrift libraries are
sys.path.insert(0, glob.glob('thrift-0.9.3/lib/py/build/lib.*')[0])


from PyTest import PyService
from PyTest.ttypes import *
from PyTest.constants import *

from thrift.transport import TSocket
from thrift.transport import TTransport
from thrift.protocol import TBinaryProtocol
from thrift.server import TNonblockingServer

LIB_NAME = "PyTest"

server = None

class PyTestHandler:
    def __init__(self):
        pass

    def segment(self, text):
        ret = []
        id = 0
        for w in text.split():
            res = Result()
            res.id = id
            res.word = w
            id = id + 1
        return ret

    def handleRequest(self, request):
        pass

def sig_handler(signum, frame):
    print "Received signal "
    if signum == signal.SIGINT:
        server.stop()

class RunSvrThread(threading.Thread):
    def run(self):
        server.serve()

if __name__ == '__main__':
    signal.signal(signal.SIGINT, sig_handler)

    handler = PyTestHandler()
    processor = PyService.Processor(handler)
    transport = TSocket.TServerSocket(port = 10080)
    tfactory = TTransport.TFramedTransportFactory()
    pfactory = TBinaryProtocol.TBinaryProtocolFactory()

    # 最后一个参数是线程数量，不分工作io
    server = TNonblockingServer.TNonblockingServer(processor, transport, tfactory, pfactory, 10)

    logging.info("Server starts...")
    server.serve()

    #  svrThr = RunSvrThread()
    #  svrThr.start()
    #  print "join thread"
    #  svrThr.join()

