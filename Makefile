all: server.bin

CC = c++

INCLUDE = -I./include/cppnetlib

LIBS = -lcppnetlib-uri -lcppnetlib-client-connections -lcppnetlib-server-parsers -lssl -lcrypto \
	   -lboost_thread -lboost_system -lrt -ldl -lglog

FLAGS = -std=c++11 -pthread -g -O3

SRC = $(shell find src -type f -name '*.cpp')

server.bin:
	$(CC) -o $@ $(SRC) $(INCLUDE) $(LIBS) $(FLAGS)

clean:
	rm -rf *.bin *.bin.*
