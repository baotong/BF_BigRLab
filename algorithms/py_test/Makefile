all: RegisterSvr

CC = g++

COMM_DIR = ../../common
ALG_COMM_DIR = ../alg_common
API_SERVER_DIR = ../../api_server
INCLUDE = -I$(COMM_DIR) -I$(ALG_COMM_DIR) -I$(API_SERVER_DIR)/gen-cpp
SRC = RegisterSvr.cpp
SRC += $(shell find $(API_SERVER_DIR)/gen-cpp -type f -name '*.cpp')
LIBS = -lthrift -lboost_thread -lboost_system -lboost_chrono -levent -lglog
FLAGS = -std=c++11 -pthread -O3 -g

RegisterSvr:
	$(CC) -o $@.bin $(SRC) $(INCLUDE) $(LIBS) $(FLAGS)
clean:
	rm -f *.bin *.bin.*
