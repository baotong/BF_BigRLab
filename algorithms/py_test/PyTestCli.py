import sys, glob
# add path with Apache Thrift compiler generated files
sys.path.append('gen-py')
# add path where built Apache Thrift libraries are
sys.path.insert(0, glob.glob('thrift-0.9.3/lib/py/build/lib.*')[0])

from PyTest import PyService
from PyTest.ttypes import *
from PyTest.constants import *

from thrift import Thrift
from thrift.transport import TSocket
from thrift.transport import TTransport
from thrift.protocol import TBinaryProtocol

transport = TSocket.TSocket('localhost', 10080)
# transport = TTransport.TBufferedTransport(transport)
transport = TTransport.TFramedTransport(transport)
protocol = TBinaryProtocol.TBinaryProtocol(transport)
client = PyService.Client(protocol)

transport.open()

# client.log('logile.log')
# print client.multiply(2,21)
client.handleRequest("Hello world!")
client.handleRequest("Welcome to China")

transport.close()
