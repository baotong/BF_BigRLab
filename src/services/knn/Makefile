all: knn_service.so

CC = c++
API_SERVER_DIR = ../../api_server
ALG_MGR_DIR = ../../alg_mgr
ALG_KNN_DIR = ../../algorithms/knn

SRC = $(shell find . -type f -name '*.cpp')
SRC += $(shell find $(ALG_KNN_DIR)/gen-cpp -type f -name '*.cpp')
SRC += $(API_SERVER_DIR)/work_mgr.cpp
INCLUDE = -I$(API_SERVER_DIR) -I$(ALG_MGR_DIR) -I$(ALG_MGR_DIR)/gen-cpp \
		  -I$(ALG_KNN_DIR) -I$(ALG_KNN_DIR)/gen-cpp
FLAGS = -std=c++11 -fPIC -shared -O3 

knn_service.so:
	$(CC) -o $@ $(SRC) $(INCLUDE) $(LIBS) $(FLAGS)

clean:
	rm -f *.so
