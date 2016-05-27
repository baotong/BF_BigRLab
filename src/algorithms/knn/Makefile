all: wordknn
wordknn: wordknn.bin

CC = c++
COMM_DIR = ../../common
API_SERVER_DIR = ../../api_server

SRC = $(shell find . -type f -name '*.cpp')
SRC += $(shell find $(API_SERVER_DIR)/gen-cpp -type f -name '*.cpp')
INCLUDE = -I./gen-cpp -I$(COMM_DIR) -I$(API_SERVER_DIR)/gen-cpp
LIBS = -lthriftnb -lthrift -lboost_thread -lboost_system -levent -lglog -lgflags
FLAGS = -std=c++11 -pthread -O3 -g

wordknn.bin:
	$(CC) -o $@ $(SRC) $(INCLUDE) $(LIBS) $(FLAGS)

clean:
	rm -rf *.bin *.bin.*
