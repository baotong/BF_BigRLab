#! /bin/bash

for i in `seq 10`; do
    curl -o test$i.out -i -X POST -H "Content-Type: application/json" -d '{"username":"xyz","password":"xyz"}' http://localhost:9000/knn &
    # echo $i
done

