all: bigrcli

CC = c++
INCLUDE = -I../api_server
LIBS = -lrt -lgflags
FLAGS = -std=c++11 -pthread -O3 -g

SRC = bigrcli.cpp

bigrcli:
	$(CC) -o $@.bin $(SRC) $(INCLUDE) $(LIBS) $(FLAGS)
clean:
	rm -f *.bin
