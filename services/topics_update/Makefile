all: topic_service_update.so

CC = c++

COMM_DIR = ../../common
API_SERVER_DIR = ../../api_server
ALG_DIR = ../../algorithms/topics_update/test

SRC = $(shell find . -type f -name '*.cpp')
SRC += $(shell find $(API_SERVER_DIR)/gen-cpp -type f -name '*.cpp')
SRC += $(shell find $(ALG_DIR)/gen-cpp -type f -name '*.cpp')
INCLUDE = -I$(COMM_DIR) -I$(API_SERVER_DIR) -I$(API_SERVER_DIR)/gen-cpp -I$(ALG_DIR)/gen-cpp
FLAGS = -std=c++11 -pthread -fPIC -shared -O3 

topic_service_update.so:
	$(CC) -o $@ $(SRC) $(INCLUDE) $(LIBS) $(FLAGS)

clean:
	rm -f *.so
