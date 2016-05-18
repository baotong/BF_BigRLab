all: server client

CC = c++

COMM_SRC = $(shell find gen-cpp -type f -name '*.cpp')
INCLUDE = -I./gen-cpp 
COMM_LIBS = -lglog -lthrift -lboost_thread -lboost_system
COMM_FLAGS = -std=c++11 -O3 -g

server: alg_mgr_svr.bin
client: test_client.bin

alg_mgr_svr.bin:
	$(CC) -o $@ alg_mgr.cpp $(COMM_SRC) $(INCLUDE) -lthriftnb -levent $(COMM_LIBS) $(COMM_FLAGS)
test_client.bin:
	$(CC) -o $@ test_client.cpp $(COMM_SRC) $(INCLUDE) $(COMM_LIBS) $(COMM_FLAGS)

clean:
	rm -rf *.bin *.bin.*

