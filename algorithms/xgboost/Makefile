all: xgboost_svr

CC = g++

COMM_DIR = ../../common
ALG_COMM_DIR = ../alg_common
API_SERVER_DIR = ../../api_server
INCLUDE = -I./gen-cpp -I$(COMM_DIR) -I$(ALG_COMM_DIR) -I$(API_SERVER_DIR)/gen-cpp
SRC = $(shell find . -type f -name '*.cpp')
SRC += $(shell find $(API_SERVER_DIR)/gen-cpp -type f -name '*.cpp')
LIBS = -lthriftnb -lthrift -lboost_thread -lboost_system -lboost_chrono -levent -lxgboost -ldmlc -lrabit -lm -lglog -lgflags -ljsoncpp
FLAGS = -std=c++11 -pthread -fopenmp -O3 -g

xgboost_svr:
	$(CC) -o $@.bin $(SRC) $(INCLUDE) $(LIBS) $(FLAGS)
clean:
	rm -f *.bin *.bin.*
