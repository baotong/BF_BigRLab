all: apiserver
apiserver: apiserver.bin

CC = c++

SRC = $(shell find . -type f -name '*.cpp')
INCLUDE = -I../common -I./gen-cpp
LIBS = -lcppnetlib-uri -lcppnetlib-client-connections -lcppnetlib-server-parsers -lssl -lcrypto \
	   -lthriftnb -lthrift -lboost_chrono -lboost_thread -lboost_system -levent -lrt -ldl \
	   -ljsoncpp -lglog -lgflags
FLAGS = -std=c++11 -pthread -O3 -g

apiserver.bin:
	$(CC) -o $@ $(SRC) $(INCLUDE) $(LIBS) $(FLAGS)

clean:
	rm -rf *.bin *.bin.*
