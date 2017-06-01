#! /bin/bash

for i in `seq 12605`
do
    printf "48-60\t1\n"
done

# cat test.data | awk '{if ($1 == "21-30" && $2 == "0") print $0}' | wc -l

