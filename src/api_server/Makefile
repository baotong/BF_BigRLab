all: apiserver
apiserver: apiserver.bin

CC = c++
ALG_MGR_DIR = ../alg_mgr

SRC = $(shell find . -type f -name '*.cpp')
SRC += $(shell find $(ALG_MGR_DIR)/gen-cpp -type f -name '*.cpp')
INCLUDE = -I./gen-cpp -I$(ALG_MGR_DIR) -I$(ALG_MGR_DIR)/gen-cpp
LIBS = -lcppnetlib-uri -lcppnetlib-client-connections -lcppnetlib-server-parsers -lssl -lcrypto \
	   -lthriftnb -lthrift -lboost_thread -lboost_system -levent -lrt -ldl -lglog -lgflags
FLAGS = -std=c++11 -pthread -O3 -g

apiserver.bin:
	$(CC) -o $@ $(SRC) $(INCLUDE) $(LIBS) $(FLAGS)

clean:
	rm -rf *.bin *.bin.*
