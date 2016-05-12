all: knn_service.so

CC = c++

INCLUDE = -I../src

# LIBS = -lcppnetlib-uri -lcppnetlib-client-connections -lcppnetlib-server-parsers -lssl -lcrypto

# FLAGS = -std=c++11 -pthread -fPIC -shared -O3 
FLAGS = -std=c++11 -fPIC -shared -O3 

SRC = $(shell find . -type f -name '*.cpp')

knn_service.so:
	$(CC) -o $@ $(SRC) $(INCLUDE) $(LIBS) $(FLAGS)

clean:
	rm -f *.so

