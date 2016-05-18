all: server

CC = c++

COMM_SRC = $(shell find gen-cpp -type f -name '*.cpp')
INCLUDE = -I./src/gen-cpp 
COMM_LIBS = -lglog -lthrift -lboost_thread -lboost_system
COMM_FLAGS = -std=c++11 -O3 -g

server: alg_mgr_svr.bin

alg_mgr_svr.bin:
	$(CC) -o $@ alg_mgr.cpp $(COMM_SRC) $(INCLUDE) -lthriftnb -levent $(COMM_LIBS) $(COMM_FLAGS)

