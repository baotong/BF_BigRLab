all: wordknn
wordknn: wordknn.bin

CC = c++

SRC = $(shell find . -type f -name '*.cpp')
INCLUDE = -I./gen-cpp -I../../alg_mgr -I../../alg_mgr/gen-cpp
LIBS = -lthriftnb -lthrift -lboost_thread -lboost_system -levent -lglog -lgflags
FLAGS = -std=c++11 -pthread -O3 -g

wordknn.bin:
	$(CC) -o $@ $(SRC) $(INCLUDE) $(LIBS) $(FLAGS)

clean:
	rm -rf *.bin *.bin.*
